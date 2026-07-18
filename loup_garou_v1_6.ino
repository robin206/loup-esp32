// V1.2 : Ajout routeur Wifi pour controle en http. Suppression du BLE
// V1.3 : Ajout lumières du sampler
// V1.6 : Retrait Wifi, ajout BLE, modification des animations de sampler
// V1.4 : Suppression WiFi + ajout BLE pour contrôle des LEDs
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <FastLED.h>

#define LED_PIN     5
#define MAX_LEDS    300
int numLedsUsed = 300;
int currentBrightness = 150;

#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
CRGB* leds = nullptr;

enum AmbientMode { OFF, DAY, NIGHT, VOTE };
AmbientMode currentAmbient = OFF;
AmbientMode targetAmbient = OFF;
unsigned long samplerEndTime = 0;
bool isSamplerActive = false;

CRGB sunWhite = CRGB(255, 220, 150);
CRGB nightColor = CRGB(50, 50, 120);
CRGB voteColor = CRGB::Red;
CRGB offColor = CRGB::Black;
CRGB currentColor = CRGB::Black;

// UUID Nordic UART Service (NUS)
#define SERVICE_UUID        "d752c5fb-1380-4cd5-b0ef-cac7d72cff20"
#define CHARACTERISTIC_RX   "2d30c082-f39f-4ce6-923f-3484ea480596"

BLECharacteristic* rxChar;

CRGB getAmbientColor(AmbientMode mode) {
  switch (mode) {
    case DAY: return sunWhite;
    case NIGHT: return nightColor;
    case VOTE: return voteColor;
    case OFF:
    default: return offColor;
  }
}

void transitionToColor(CRGB targetColor, uint16_t duration) {
  for (uint8_t s = 0; s <= 30; s++) {
    float p = (float)s / 30;
    CRGB i = currentColor.lerp8(targetColor, p * 255);
    fill_solid(leds, numLedsUsed, i);
    FastLED.show();
    delay(duration / 30);
  }
  currentColor = targetColor;
}

void applyAmbient() {
  CRGB color = getAmbientColor(currentAmbient);
  transitionToColor(color, 1000);
}

void animationLoup() {
  for (int i = 0; i < numLedsUsed; i++)
    leds[i] = (i % 2 == 0) ? CRGB::Red : CRGB::Black;
  FastLED.show();
  delay(10000);
}

void animationClocher() {
  for (int i = 0; i < 12; i++) {
    fill_solid(leds, numLedsUsed, CRGB::White);
    FastLED.show();
    delay(300);
    fill_solid(leds, numLedsUsed, CRGB::Black);
    FastLED.show();
    delay(1800);
  }
}

void animationHorloge() {
  // Temps entre chaque pic en ms (calculé depuis le fichier audio)
  int flashDelays[] = {90, 50, 80, 60, 50, 50, 70, 60, 90, 90, 70, 70};
  int n = sizeof(flashDelays) / sizeof(flashDelays[0]);

  for (int i = 0; i < n; i++) {
    delay(flashDelays[i]); // Attente avant le prochain pic

    fill_solid(leds, numLeds, CRGB::White); // Flash blanc
    FastLED.show();
    delay(100); // Durée du flash
    fill_solid(leds, numLeds, CRGB::Black); // Retour au noir
    FastLED.show();
  }
}

void animationTonnerre() {
  CRGB color = getAmbientColor(currentAmbient);
  for (int i = 0; i < 3; i++) {
    fill_solid(leds, numLeds, CRGB::White);  // Flash blanc
    FastLED.show();
    delay(120);
    fill_solid(leds, numLeds, color);        // Retour ambiance
    FastLED.show();
    delay(200 + random(100));                // Variation naturelle
  }
  currentColor = color;
  delay(1000);
}

void animationViolon() {
  for (int i = 0; i < 7; i++) {
    fill_solid(leds, numLedsUsed, CRGB::Green);
    FastLED.show();
    delay(250);
    fill_solid(leds, numLedsUsed, CRGB::Black);
    FastLED.show();
    delay(250);
  }
}

void startSampler(String val) {
  isSamplerActive = true;

  if (val == "loup") {
    animationLoup();
  } else if (val == "clocher") {
    animationClocher();
  } else if (val == "horloge") {
    animationHorloge();
  } else if (val == "tonnerre") {
    animationTonnerre();
  } else if (val == "violon") {
    animationViolon();
  }

  isSamplerActive = false;
  applyAmbient();
}

void handleCommand(String val) {
  val.toLowerCase();

  if (val == "jour") currentAmbient = DAY;
  else if (val == "nuit") currentAmbient = NIGHT;
  else if (val == "vote") currentAmbient = VOTE;
  else if (val == "off") currentAmbient = OFF;
  else startSampler(val);

  if (!isSamplerActive) applyAmbient();
}

class RXCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) {
    std::string rxValue = characteristic->getValue();
    if (rxValue.length() > 0) {
      String cmd = String(rxValue.c_str());
      Serial.print("Commande BLE reçue : ");
      Serial.println(cmd);
      handleCommand(cmd);
    }
  }
};

void animationDemarrage() {
  for (int i = 0; i < numLedsUsed; i++) {
    leds[i] = CRGB::White;
    FastLED.show();
    delay(5);
  }
  delay(500);
  fill_solid(leds, numLedsUsed, CRGB::Black);
  FastLED.show();
}

void setup() {
  Serial.begin(115200);

  leds = new CRGB[MAX_LEDS];
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, MAX_LEDS);
  FastLED.setBrightness(currentBrightness);
  fill_solid(leds, numLedsUsed, CRGB::Black);
  FastLED.show();

  animationDemarrage();

  // BLE INIT
  BLEDevice::init("LoupGarou_LED");
  BLEServer* server = BLEDevice::createServer();
  BLEService* service = server->createService(SERVICE_UUID);

  rxChar = service->createCharacteristic(
    CHARACTERISTIC_RX,
    BLECharacteristic::PROPERTY_WRITE
  );

  rxChar->setCallbacks(new RXCallback());
  service->start();

  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->start();

  Serial.println("BLE prêt. En attente de commandes...");
}

void loop() {
  // Rien à faire, BLE callbacks gèrent tout
}
