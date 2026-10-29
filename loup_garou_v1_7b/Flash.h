#pragma once
#include <stdint.h>

// Un flash chronométré depuis le départ de l'animation.
// Défini dans un fichier .h à part parce que l'Arduino IDE génère automatiquement
// les prototypes des fonctions en haut du sketch, avant les types déclarés dans le .ino.
struct Flash {
  uint16_t atMs;      // début du flash, mesuré depuis le départ de l'animation
  uint8_t  level;     // intensité maximale (0 à 255)
  uint16_t attackMs;  // montée
  uint16_t holdMs;    // maintien à l'intensité maximale
  uint16_t decayMs;   // retombée vers l'ambiance
};
