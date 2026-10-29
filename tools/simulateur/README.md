# Simulateur et testeur du firmware

Le fichier `../testeur-led.html` se génère à partir du vrai code du firmware. Il faut le régénérer à chaque modification des animations, des couleurs ou des temps dans `loup_garou_v1_7b.ino`.

Le principe: `sim2.cpp` inclut le `.ino` tel quel, avec de fausses versions d'Arduino, de FastLED et du BLE (dossier `inc`) et une horloge virtuelle. Il joue chaque animation sur chaque ambiance et enregistre toutes les images du bandeau dans `runs.json`. `build.py` ajoute les sons du sampler et produit la page HTML.

## Régénérer

Il faut g++ et Python 3. Depuis ce dossier:

```
g++ -std=c++17 -Iinc -I../../loup_garou_v1_7b sim2.cpp -o sim2
./sim2 runs.json
python3 build.py runs.json <chemin vers loup-garou-party-selector/public/audio/sampler> ../testeur-led.html
```

## Limites

Le simulateur ne reproduit ni la luminosité réelle, ni la limite de courant, ni le rendu des diodes, ni la latence Bluetooth. Pour cela, utiliser le mode bandeau réel de la page.
