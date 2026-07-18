🐺 Loupgarou LED Controller – ESP32 + BLE
Contrôleur de LEDs pour l’application Loup Garou, basé sur un ESP32 WROOM et un bandeau WS2812B.
Communication via Bluetooth Low Energy (BLE) et animations synchronisées avec les phases du jeu.

✨ Fonctionnalités
🎨 Ambiances lumineuses :

Jour

Nuit

Vote

Off

🔊 Animations “Sampler” synchronisées avec les sons du jeu :

Loup

Clocher

Horloge

Tonnerre

Violon

📡 Communication BLE via le protocole Nordic UART Service (NUS)

💡 Transitions douces entre ambiances

🚀 Animation de démarrage

🔌 Compatible WS2812B (FastLED)

🧩 Matériel requis
ESP32 WROOM

Bandeau LED WS2812B (jusqu’à 300 LEDs)

Alimentation 5V adaptée au nombre de LEDs

Câblage :

LED_PIN = GPIO 5

GND commun entre ESP32 et alimentation LEDs

📦 Dépendances Arduino
Dans l’IDE Arduino, installer :

FastLED

ESP32 BLE Arduino (natif dans les cartes ESP32)

🔧 Installation
Installer les cartes ESP32 dans Arduino IDE

Charger le fichier .ino dans l’ESP32

Alimenter le bandeau LED

Rechercher l’appareil BLE :
LoupGarou_LED

📡 Protocole BLE
Le projet utilise le Nordic UART Service (NUS) :

Élément	UUID
Service	d752c5fb-1380-4cd5-b0ef-cac7d72cff20
RX Characteristic	2d30c082-f39f-4ce6-923f-3484ea480596


📤 Commandes disponibles
Envoyer une chaîne de texte via BLE :

Ambiances
Code
jour
nuit
vote
off
Animations
Code
loup
clocher
horloge
tonnerre
violon
🎬 Aperçu des animations
🌞 Ambiances
Transitions douces (1 seconde) entre les couleurs prédéfinies.

🐺 Loup
Alternance rouge/noir pendant 10 secondes.

🔔 Clocher
12 flashs blancs espacés comme les coups de cloche.

🕰 Horloge
Flashs synchronisés avec les pics audio (table de timings).

⚡ Tonnerre
Flashs blancs avec variations naturelles.

🎻 Violon
Pulsations vertes répétées.

🧠 Architecture du code
handleCommand() : interprète les commandes BLE

startSampler() : déclenche les animations

transitionToColor() : interpolation douce entre couleurs

RXCallback : réception BLE

setup() : initialisation FastLED + BLE

loop() : callbacks BLE uniquement

🚀 Animation de démarrage
Un balayage blanc progressif sur toutes les LEDs, puis extinction.

📁 Structure du dépôt
Code
/Loupgarou-ESP32
 ├── src/
 │    └── loupgarou_led.ino
 ├── README.md
 └── LICENSE (optionnel)
🛠 Améliorations possibles
Ajout d’une caractéristique BLE TX pour retour d’état

Mode “fade” pour les animations

Gestion d’un nombre variable de LEDs via BLE

Intégration avec une app mobile dédiée