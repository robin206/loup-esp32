// Loup Garou LED controller (ESP32 + WS2812B + BLE)
//
// Historique
// V1.2 : Ajout routeur Wifi pour contrôle en http. Suppression du BLE
// V1.3 : Ajout lumières du sampler
// V1.4 : Suppression WiFi + ajout BLE pour contrôle des LEDs
// V1.7b : Compilation corrigée (core ESP32 2.x et 3.x), relance de l'annonce BLE,
//        commandes exécutées dans loop(), commandes lum:N et leds:N,
//        retour d'état par notification BLE, limite de courant,
//        fondu depuis l'état réel du bandeau, loup en respiration (7 s),
//        violon en serpent vert (2,3 s, queue de 60 LEDs),
//        clocher et horloge: flashs chronométrés sur l'audio, ambiance conservée entre les flashs

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <FastLED.h>
#include "Flash.h"   // type Flash (fichier dans le dossier du sketch)

// ---------------------------------------------------------------- Matériel
#define LED_PIN        5
#define MAX_LEDS       300
#define LED_TYPE       WS2812B
#define COLOR_ORDER    GRB

#define SUPPLY_VOLTS   5
#define SUPPLY_MAX_MA  15000   // alimentation 5 V 20 A, on garde 25 % de marge

#define DEFAULT_BRIGHTNESS 150
#define MIN_BRIGHTNESS     10

int numLedsUsed = MAX_LEDS;
int currentBrightness = DEFAULT_BRIGHTNESS;
CRGB* leds = nullptr;

// ---------------------------------------------------------------- Ambiances
enum AmbientMode { OFF, DAY, NIGHT, VOTE };
AmbientMode currentAmbient = OFF;

const CRGB sunWhite   = CRGB(255, 220, 150);
const CRGB nightColor = CRGB(50, 50, 120);
const CRGB voteColor  = CRGB(255, 0, 0);
const CRGB offColor   = CRGB(0, 0, 0);

// Couleur réellement affichée sur le bandeau (sert de point de départ aux fondus)
CRGB currentColor = CRGB::Black;

#define FADE_MS        1000    // fondu entre ambiances
#define LOUP_MS        7000    // respiration du loup (aller et retour compris)
#define VIOLON_MS      2300    // passage du serpent sur tout le bandeau
#define VIOLON_TAIL    60      // longueur de la queue du serpent, en LEDs

const CRGB violonColor = CRGB(0, 255, 0);
const CRGB loupColor   = CRGB(255, 0, 0);

// ---------------------------------------------------------------- BLE
// UUID personnalisés (ce ne sont pas ceux du Nordic UART Service)
#define SERVICE_UUID      "d752c5fb-1380-4cd5-b0ef-cac7d72cff20"
#define CHARACTERISTIC_RX "2d30c082-f39f-4ce6-923f-3484ea480596"   // app -> ESP32 (écriture)
#define CHARACTERISTIC_TX "9b2f4d3a-5c1e-4a7b-8f6d-1e2c3b4a5d60"   // ESP32 -> app (notification)

BLECharacteristic* txChar = nullptr;
volatile bool clientConnected = false;

// Une seule commande en attente (la dernière reçue remplace la précédente)
#define CMD_MAX 32
char pendingCmd[CMD_MAX] = "";
volatile bool hasPendingCmd = false;
portMUX_TYPE cmdMux = portMUX_INITIALIZER_UNLOCKED;

void notifyApp(const String& msg) {
  Serial.print("Retour : ");
  Serial.println(msg);
  if (clientConnected && txChar != nullptr) {
    txChar->setValue((uint8_t*)msg.c_str(), msg.length());
    txChar->notify();
  }
}

// ---------------------------------------------------------------- Outils LED
CRGB getAmbientColor(AmbientMode mode) {
  switch (mode) {
    case DAY:   return sunWhite;
    case NIGHT: return nightColor;
    case VOTE:  return voteColor;
    case OFF:
    default:    return offColor;
  }
}

// Affiche une couleur unie et mémorise l'état réel du bandeau
void showSolid(const CRGB& color) {
  fill_solid(leds, numLedsUsed, color);
  FastLED.show();
  currentColor = color;
}

// Fondu depuis ce qui est réellement affiché vers la couleur cible
void fadeTo(const CRGB& target, uint16_t durationMs) {
  CRGB from = currentColor;
  if (from == target) return;

  uint32_t t0 = millis();
  while (true) {
    uint32_t elapsed = millis() - t0;
    if (elapsed >= durationMs) break;
    uint8_t p = ease8InOutQuad((uint8_t)((elapsed * 255UL) / durationMs));
    fill_solid(leds, numLedsUsed, blend(from, target, p));
    FastLED.show();
    delay(1);
  }
  showSolid(target);
}

void applyAmbient() {
  fadeTo(getAmbientColor(currentAmbient), FADE_MS);
}

// ---------------------------------------------------------------- Animations
// Toutes finissent en affichant la couleur d'ambiance, sauf le clocher
// (fin noire) que applyAmbient() ramène ensuite en fondu.

// Respiration rouge : ambiance -> rouge -> ambiance, en LOUP_MS au total
void animationLoup() {
  CRGB base = getAmbientColor(currentAmbient);
  uint32_t t0 = millis();
  while (true) {
    uint32_t elapsed = millis() - t0;
    if (elapsed >= LOUP_MS) break;
    float k = 0.5f * (1.0f - cosf(TWO_PI * (float)elapsed / (float)LOUP_MS));
    fill_solid(leds, numLedsUsed, blend(base, loupColor, (uint8_t)(k * 255.0f)));
    FastLED.show();
    delay(1);
  }
  showSolid(base);
}

// ---- Flashs chronométrés depuis le départ de l'animation (pas de dérive cumulée)
// Intensité d'un flash à l'instant e (en ms depuis le départ)
uint8_t flashLevel(const Flash& f, int32_t e) {
  int32_t dt = e - (int32_t)f.atMs;
  if (dt < 0) return 0;
  if (dt < f.attackMs) return (uint8_t)((uint32_t)f.level * dt / f.attackMs);
  dt -= f.attackMs;
  if (dt < f.holdMs) return f.level;
  dt -= f.holdMs;
  if (dt < f.decayMs) {
    uint8_t r = 255 - (uint8_t)((uint32_t)255 * dt / f.decayMs);
    return scale8(f.level, scale8(r, r));   // retombée douce
  }
  return 0;
}

// Joue une liste de flashs par-dessus la couleur d'ambiance pendant totalMs.
// offsetMs décale tous les flashs (négatif = plus tôt, pour compenser la latence BLE).
void playFlashes(const Flash* flashes, int n, uint16_t totalMs, int offsetMs, const CRGB& flashColor) {
  CRGB base = getAmbientColor(currentAmbient);
  uint32_t t0 = millis();
  while (true) {
    uint32_t elapsed = millis() - t0;
    if (elapsed >= totalMs) break;
    uint8_t level = 0;
    for (int i = 0; i < n; i++) {
      uint8_t l = flashLevel(flashes[i], (int32_t)elapsed - offsetMs);
      if (l > level) level = l;
    }
    fill_solid(leds, numLedsUsed, blend(base, flashColor, level));
    FastLED.show();
    delay(1);
  }
  showSolid(base);
}

// Clocher : 12 dongs, un flash blanc de 300 ms sur chacun, ambiance conservée entre les dongs.
// Instants mesurés dans sampler_clocher.ogg (période 2,083 s, premier dong à 0,08 s).
#define CLOCHER_OFFSET_MS 0
#define CLOCHER_TOTAL_MS  25000
const Flash clocherFlashes[] = {
  {  80, 255, 0, 300, 0}, { 2150, 255, 0, 300, 0}, { 4220, 255, 0, 300, 0}, { 6310, 255, 0, 300, 0},
  { 8390, 255, 0, 300, 0}, {10480, 255, 0, 300, 0}, {12560, 255, 0, 300, 0}, {14640, 255, 0, 300, 0},
  {16730, 255, 0, 300, 0}, {18820, 255, 0, 300, 0}, {20900, 255, 0, 300, 0}, {22980, 255, 0, 300, 0}
};

void animationClocher() {
  playFlashes(clocherFlashes, sizeof(clocherFlashes) / sizeof(clocherFlashes[0]),
              CLOCHER_TOTAL_MS, CLOCHER_OFFSET_MS, CRGB::White);
}

// Horloge : 3 grands coups (jaune plein, longue retombée) et 5 tics (jaune 40 %, retombée courte).
// Instants mesurés dans sampler_clock.ogg. L'ambiance reste affichée entre les flashs.
#define HORLOGE_OFFSET_MS 0
#define HORLOGE_TOTAL_MS  14500
const CRGB horlogeColor = CRGB(255, 190, 0);
const Flash horlogeFlashes[] = {
  {  670, 255, 25, 0, 1500},   // grand coup
  { 3740, 102, 10, 0,  250},   // tic
  { 4970, 102, 10, 0,  250},   // tic
  { 6670, 255, 25, 0, 1500},   // grand coup
  { 8880, 102, 10, 0,  250},   // tic
  {10050, 102, 10, 0,  250},   // tic
  {11280, 102, 10, 0,  250},   // tic
  {12680, 255, 25, 0, 1500}    // grand coup
};

void animationHorloge() {
  playFlashes(horlogeFlashes, sizeof(horlogeFlashes) / sizeof(horlogeFlashes[0]),
              HORLOGE_TOTAL_MS, HORLOGE_OFFSET_MS, horlogeColor);
}

void animationTonnerre() {
  CRGB base = getAmbientColor(currentAmbient);
  for (int i = 0; i < 3; i++) {
    showSolid(CRGB::White);
    delay(120);
    showSolid(base);
    delay(200 + random(100));   // variation naturelle
  }
  delay(1000);
}

// Serpent vert qui traverse tout le bandeau en VIOLON_MS
void animationViolon() {
  CRGB base = getAmbientColor(currentAmbient);
  const int span = numLedsUsed + VIOLON_TAIL;
  uint32_t t0 = millis();
  while (true) {
    uint32_t elapsed = millis() - t0;
    if (elapsed >= VIOLON_MS) break;
    int head = (int)(((uint32_t)span * elapsed) / VIOLON_MS);

    fill_solid(leds, numLedsUsed, base);
    for (int k = 0; k < VIOLON_TAIL; k++) {
      int idx = head - k;
      if (idx < 0 || idx >= numLedsUsed) continue;
      uint8_t a = 255 - (uint8_t)(((uint16_t)k * 255) / VIOLON_TAIL);
      leds[idx] = blend(base, violonColor, scale8(a, a));   // queue qui s'estompe
    }
    FastLED.show();
    delay(1);
  }
  showSolid(base);
}

void animationDemarrage() {
  for (int i = 0; i < numLedsUsed; i++) {
    leds[i] = CRGB::White;
    FastLED.show();
    delay(5);
  }
  delay(500);
  showSolid(CRGB::Black);
}

// ---------------------------------------------------------------- Commandes
// Retourne true si la commande est un sampler connu
bool runSampler(const String& name) {
  if (name == "loup")          animationLoup();
  else if (name == "clocher")  animationClocher();
  else if (name == "horloge")  animationHorloge();
  else if (name == "tonnerre") animationTonnerre();
  else if (name == "violon")   animationViolon();
  else return false;
  return true;
}

void setBrightnessCmd(const String& cmd) {
  long n = cmd.substring(4).toInt();
  if (n <= 0) { notifyApp("err:" + cmd); return; }
  currentBrightness = constrain(n, MIN_BRIGHTNESS, 255);
  FastLED.setBrightness(currentBrightness);
  showSolid(currentColor);
  notifyApp("lum:" + String(currentBrightness));
}

void setLedCountCmd(const String& cmd) {
  long n = cmd.substring(5).toInt();
  if (n <= 0) { notifyApp("err:" + cmd); return; }
  int count = constrain(n, 1, MAX_LEDS);
  fill_solid(leds, MAX_LEDS, CRGB::Black);   // éteint les LEDs qui sortent de la zone utilisée
  FastLED.show();
  numLedsUsed = count;
  showSolid(currentColor);
  notifyApp("leds:" + String(numLedsUsed));
}

void handleCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();

  bool isAmbient = true;
  if      (cmd == "jour") currentAmbient = DAY;
  else if (cmd == "nuit") currentAmbient = NIGHT;
  else if (cmd == "vote") currentAmbient = VOTE;
  else if (cmd == "off")  currentAmbient = OFF;
  else isAmbient = false;

  if (isAmbient) {
    applyAmbient();
    notifyApp("ok:" + cmd);
  } else if (cmd.startsWith("lum:")) {
    setBrightnessCmd(cmd);
  } else if (cmd.startsWith("leds:")) {
    setLedCountCmd(cmd);
  } else if (runSampler(cmd)) {
    applyAmbient();             // un seul retour à l'ambiance
    notifyApp("done:" + cmd);
  } else {
    notifyApp("err:" + cmd);
  }
}

// ---------------------------------------------------------------- Callbacks BLE
class RXCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) {
    // Compatible core ESP32 2.x (std::string) et 3.x (String)
    String cmd = String(characteristic->getValue().c_str());
    cmd.trim();
    if (cmd.length() == 0) return;

    Serial.print("Commande BLE reçue : ");
    Serial.println(cmd);

    // On ne fait que mémoriser : les animations tournent dans loop()
    portENTER_CRITICAL(&cmdMux);
    strncpy(pendingCmd, cmd.c_str(), CMD_MAX - 1);
    pendingCmd[CMD_MAX - 1] = '\0';
    hasPendingCmd = true;
    portEXIT_CRITICAL(&cmdMux);
  }
};

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) {
    clientConnected = true;
    Serial.println("Client BLE connecté");
  }
  void onDisconnect(BLEServer* server) {
    clientConnected = false;
    Serial.println("Client BLE déconnecté, relance de l'annonce");
    BLEDevice::startAdvertising();
  }
};

// ---------------------------------------------------------------- setup / loop
void setup() {
  Serial.begin(115200);

  leds = new CRGB[MAX_LEDS];
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, MAX_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(SUPPLY_VOLTS, SUPPLY_MAX_MA);
  FastLED.setBrightness(currentBrightness);
  fill_solid(leds, MAX_LEDS, CRGB::Black);
  FastLED.show();

  animationDemarrage();

  // BLE
  BLEDevice::init("LoupGarou_LED");
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  BLEService* service = server->createService(SERVICE_UUID);

  BLECharacteristic* rxChar = service->createCharacteristic(
    CHARACTERISTIC_RX,
    BLECharacteristic::PROPERTY_WRITE
  );
  rxChar->setCallbacks(new RXCallback());

  txChar = service->createCharacteristic(
    CHARACTERISTIC_TX,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  txChar->addDescriptor(new BLE2902());

  service->start();

  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);   // le nom va dans la réponse de scan
  advertising->setMinPreferred(0x06);
  advertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE prêt. En attente de commandes...");
}

void loop() {
  if (hasPendingCmd) {
    char buf[CMD_MAX];
    portENTER_CRITICAL(&cmdMux);
    memcpy(buf, pendingCmd, CMD_MAX);
    hasPendingCmd = false;
    portEXIT_CRITICAL(&cmdMux);
    handleCommand(String(buf));
  }
  delay(10);
}
