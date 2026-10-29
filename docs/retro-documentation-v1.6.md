# Rétro-documentation du firmware loup_garou_v1_6

Document rédigé le 29/10/2026 à partir du fichier `loup_garou_v1_6/loup_garou_v1_6.ino` tel qu'il se trouve dans le dossier local (donc avec les correctifs non commités décrits en fin de document). Les durées sont calculées depuis le code et n'ont pas encore été mesurées sur la carte.

## Vue d'ensemble

Le firmware tourne sur un ESP32 WROOM et pilote un bandeau WS2812B (GPIO 5, 300 LEDs, luminosité 150 sur 255) avec la bibliothèque FastLED. L'ESP32 annonce un serveur BLE nommé `LoupGarou_LED` qui expose une seule caractéristique en écriture. L'application web envoie une commande texte, le firmware la mémorise, puis `loop()` l'exécute.

```
Application (Web Bluetooth)
        |  writeValue("nuit")
        v
RXCallback::onWrite()  -->  pendingCmd + hasPendingCmd
                                  |
                                  v
loop()  -->  handleCommand()  -->  ambiance (jour, nuit, vote, off)
                              \->  sampler (loup, clocher, horloge, tonnerre, violon)
                                        |
                                        v
                                  applyAmbient()  (retour à l'ambiance courante)
```

## Protocole BLE

| Élément | Valeur |
|---|---|
| Nom annoncé | LoupGarou_LED |
| Service | d752c5fb-1380-4cd5-b0ef-cac7d72cff20 |
| Caractéristique RX | 2d30c082-f39f-4ce6-923f-3484ea480596 (écriture seule) |
| Format | Texte UTF-8, insensible à la casse, espaces autour retirés |

Ces UUID sont personnalisés. Le code et le README parlent de Nordic UART Service, mais le vrai NUS utilise des UUID commençant par 6E400001, donc l'appellation est inexacte. Il n'existe aucune caractéristique de retour (TX), le firmware ne répond jamais à l'application.

## Commandes

### Ambiances

Une ambiance est une couleur unie qui reste affichée jusqu'à la prochaine commande. Le passage d'une ambiance à l'autre se fait par un fondu de 1 seconde (30 pas de 33 ms). Ce fondu bloque le firmware pendant sa durée.

| Commande | Couleur RGB | Rendu |
|---|---|---|
| jour | 255, 220, 150 | Blanc chaud type soleil |
| nuit | 50, 50, 120 | Bleu sombre |
| vote | 255, 0, 0 | Rouge |
| off | 0, 0, 0 | Éteint |

### Samplers

Un sampler est une animation ponctuelle liée à un son du jeu. Elle se joue en bloquant le firmware, puis le bandeau revient à l'ambiance courante.

| Commande | Ce que fait le code | Durée |
|---|---|---|
| loup | Une LED sur deux en rouge, les autres noires, image fixe | 10 s |
| clocher | 12 fois: tout blanc 300 ms puis tout noir 1800 ms | 25,2 s |
| horloge | 12 flashs blancs de 100 ms, précédés d'une attente variable (90, 50, 80, 60, 50, 50, 70, 60, 90, 90, 70, 70 ms) calée sur l'audio | 2,03 s |
| tonnerre | 3 fois: flash blanc 120 ms puis couleur d'ambiance pendant 200 à 300 ms (aléatoire), puis 1 s de pause finale | environ 2,1 s |
| violon | 7 fois: tout vert 250 ms puis tout noir 250 ms | 3,5 s |

Une commande inconnue ne déclenche aucune animation. Elle provoque quand même un retour à l'ambiance courante.

### Démarrage

À la mise sous tension, un balayage blanc allume les LEDs une par une (5 ms chacune, environ 1,5 s), attend 500 ms puis éteint tout. Le BLE est initialisé après cette animation.

## Fonctions du code

| Fonction | Rôle |
|---|---|
| setup() | Initialise FastLED, joue l'animation de démarrage, démarre le serveur BLE et l'annonce |
| loop() | Récupère la commande en attente, la passe à handleCommand(), puis attend 10 ms |
| RXCallback::onWrite() | Reçoit le texte BLE et le dépose dans pendingCmd |
| ServerCallbacks::onDisconnect() | Relance l'annonce BLE quand le client part |
| handleCommand() | Normalise la commande (minuscules, trim) et choisit ambiance ou sampler |
| startSampler() | Aiguille vers l'animation demandée puis rappelle applyAmbient() |
| applyAmbient() | Lance le fondu vers la couleur de l'ambiance courante |
| transitionToColor() | Fondu linéaire en 30 pas depuis currentColor |
| getAmbientColor() | Associe une ambiance à sa couleur |
| animationXxx() | Une fonction par animation décrite plus haut |

## Correctifs déjà présents dans le dossier local (non commités)

Le dépôt GitHub contient une version qui ne compile pas. Le fichier local corrige les points suivants.

- Les variables `numLeds` inexistantes sont remplacées par `numLedsUsed` dans animationHorloge et animationTonnerre.
- La lecture de la valeur BLE passe par `String(getValue().c_str())`, compatible avec le core ESP32 2.x et 3.x.
- Les commandes sont déposées dans `pendingCmd` et exécutées dans `loop()`, ce qui libère la pile BLE pendant les animations.
- Un `ServerCallbacks` relance l'annonce à la déconnexion.
- `val.trim()` ajouté dans handleCommand().
- Scan response activée, avec les paramètres d'intervalle préférés.

## Limites repérées dans le code actuel

1. Retour d'animation brutal. transitionToColor part de `currentColor`, qui n'est pas modifié par les samplers (sauf tonnerre). Après un sampler, la première image saute directement à la couleur d'ambiance au lieu de fondre depuis l'état réel du bandeau.
2. Double retour à l'ambiance. startSampler() appelle applyAmbient(), puis handleCommand() le rappelle. Le fondu est joué deux fois, ce qui rallonge la commande d'une seconde.
3. Commandes perdues. Il n'y a qu'un seul emplacement `pendingCmd`. Une commande reçue pendant une animation est écrasée par la suivante, et pendant un clocher (25 s) l'application n'a aucune réponse.
4. Partage non sécurisé entre tâches. `pendingCmd` est un String lu par `loop()` et écrit par la tâche BLE, avec seulement un `volatile bool` en protection.
5. Aucun arrêt possible. Une animation en cours ne peut pas être interrompue, même par off.
6. Pas de limite de courant. Aucune limite de puissance FastLED n'est définie. Avec 300 LEDs en blanc, le maximum théorique approche 10 A (estimation à confirmer selon l'alimentation utilisée).
7. Paramètres figés. Le nombre de LEDs (300) et la luminosité (150) sont fixes. L'application s'attend à un nombre par défaut de 50 LEDs et mentionne des commandes `ledcount` et `brightness` que le firmware ne gère pas.
8. Documentation. L'en-tête de version est dans le désordre (V1.6 avant V1.4), `BLE2902.h` est inclus sans être utilisé, et le README parle de NUS.
