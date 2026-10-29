# Loup Garou LED controller (ESP32 + BLE)

Contrôleur de LEDs pour l'application Loup Garou, basé sur un ESP32 WROOM et un bandeau WS2812B. Il reçoit des commandes texte en Bluetooth Low Energy et joue des ambiances et des animations synchronisées avec les sons du sampler.

Version courante: 1.7b, dans `loup_garou_v1_7b/`. Le dossier contient `loup_garou_v1_7b.ino` et `Flash.h`, qui doivent rester ensemble. Le dossier du sketch doit porter le même nom que le `.ino`.

## Matériel

- ESP32 WROOM
- Bandeau WS2812B de 300 LEDs, données sur le GPIO 5
- Alimentation 5 V 20 A, masse commune avec l'ESP32
- Luminosité par défaut de 150 sur 255 et limite de courant logicielle à 15 A

## Dépendances Arduino

- Bibliothèque FastLED
- Cartes ESP32 (BLE Arduino inclus), compatible avec les cores 2.x et 3.x

## Protocole BLE

| Élément | Valeur |
|---|---|
| Nom annoncé | LoupGarou_LED |
| Service | d752c5fb-1380-4cd5-b0ef-cac7d72cff20 |
| Écriture (application vers ESP32) | 2d30c082-f39f-4ce6-923f-3484ea480596 |
| Notification (ESP32 vers application) | 9b2f4d3a-5c1e-4a7b-8f6d-1e2c3b4a5d60 |

Ces UUID sont personnalisés, ce ne sont pas ceux du Nordic UART Service. Les commandes sont du texte UTF-8, insensible à la casse.

## Commandes

| Commande | Effet |
|---|---|
| jour, nuit, vote, off | Fondu de 1 s vers l'ambiance choisie |
| loup | Respiration de l'ambiance vers le rouge puis retour, 7 s |
| clocher | 12 flashs blancs de 300 ms sur les dongs, 25 s |
| horloge | 3 grands flashs jaunes et 5 tics plus légers, calés sur le son, 14,5 s |
| tonnerre | 3 flashs blancs sur l'ambiance, environ 2 s |
| violon | Serpent vert de 60 LEDs qui traverse le bandeau en 2,3 s |
| lum:N | Luminosité de 10 à 255 |
| leds:N | Nombre de LEDs utilisées, de 1 à 300 |

Les animations gardent la couleur d'ambiance en fond et y reviennent à la fin. Le clocher et l'horloge sont chronométrés depuis le départ de la commande, à partir des instants mesurés dans les fichiers audio. Les constantes `CLOCHER_OFFSET_MS` et `HORLOGE_OFFSET_MS` permettent de compenser la latence Bluetooth.

## Retour d'état

L'ESP32 envoie une notification à la fin de chaque commande.

| Notification | Signification |
|---|---|
| ok:jour | Ambiance appliquée (jour, nuit, vote ou off) |
| done:loup | Animation terminée |
| lum:150 | Luminosité appliquée |
| leds:120 | Nombre de LEDs appliqué |
| err:xxx | Commande inconnue ou invalide |

## Fonctionnement

Le callback BLE mémorise seulement la dernière commande reçue. La boucle principale l'exécute, ce qui libère la pile BLE pendant les animations. Une commande reçue pendant une animation remplace la précédente et ne démarre qu'à la fin de l'animation en cours.

La documentation détaillée de la version 1.6 se trouve dans `docs/retro-documentation-v1.6.md`.

## Outils de test

Le dossier `tools/` contient `testeur-led.html` (aperçu simulé des animations et pilotage du vrai bandeau par Bluetooth, avec les sons du sampler) et `ble-test-simple.html`. Voir `tools/README.md`. La page de test doit être régénérée quand les animations changent, la procédure est dans `tools/simulateur/README.md`.
