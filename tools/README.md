# Outils de test

| Fichier | Rôle |
|---|---|
| testeur-led.html | Aperçu simulé des animations et pilotage du vrai bandeau en Bluetooth, avec les sons du sampler, le réglage de luminosité, du nombre de LEDs et du décalage lumière/son |
| ble-test-simple.html | Petite page de test Bluetooth sans simulation, avec ambiances, animations, luminosité et journal |
| simulateur/ | Sources pour régénérer testeur-led.html quand le firmware change |

Les deux pages s'ouvrent dans Chrome ou Edge (Web Bluetooth). Elles se connectent au service du firmware, nom annoncé LoupGarou_LED.
