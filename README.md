# AG Draw

AG Draw est un logiciel de dessin vectoriel professionnel, augmenté par l'IA.

L'objectif n'est pas de cloner CorelDRAW, mais d'aller plus loin :

- Dessiner à partir d'une phrase (texte → vecteur).
- Transformer un croquis en dessin vectoriel.
- Corriger automatiquement les formes.
- Générer des logos et affiches en quelques secondes.
- Fonctionner sur Windows, macOS, Linux, Android et Web.
- Ouvrir les formats SVG, PDF et autres formats populaires.

## Stack

C++ (C++20) · Qt 6 · Skia · Git

## Architecture

Voir [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) pour le détail des modules, des
écrans et de la feuille de route.

## Compiler et lancer

### Prérequis

- CMake ≥ 3.21
- Un compilateur C++20 (GCC, Clang ou MSVC)
- Qt 6 (module de base *Widgets* ; *Test* en plus pour les tests)

Sur Debian/Ubuntu :

```bash
sudo apt install qt6-base-dev qt6-base-dev-tools
```

### Configurer et compiler

```bash
cmake -S . -B build
cmake --build build -j"$(nproc)"
```

### Lancer l'application

```bash
./build/apps/agdraw/agdraw
```

### Exécuter les tests

```bash
cd build
ctest --output-on-failure
```

Sur une machine sans affichage (CI, conteneur headless), les tests d'interface
utilisent automatiquement `QT_QPA_PLATFORM=offscreen` (configuré dans
`tests/ui/CMakeLists.txt`).

## Statut

Étape 3 franchie : moteur de dessin (rendu QPainter, en attendant Skia),
outils Sélection/Rectangle/Ellipse/Texte/Plume (avec courbes de Bézier),
annuler/rétablir, redimensionnement, multi-sélection, ordre des plans,
sauvegarde/chargement au format natif `.agd` et export PNG. Voir
[`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) pour la feuille de route
complète et les étapes à venir.
