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

Étape 4 franchie, plus trois chantiers de parité avec CorelDRAW : moteur de
dessin (rendu QPainter, en attendant Skia), outils
Sélection/Rectangle/Ellipse/Texte (multi-lignes)/Plume (avec courbes de
Bézier)/Pinceau (pression de tablette graphique), annuler/rétablir,
redimensionnement, multi-sélection, alignement/distribution, ordre des
plans, sauvegarde/chargement au format natif `.agd`, export PNG, calques
avec visibilité/verrouillage réels, effets (ombre portée, dégradé
linéaire, contour à anneaux concentriques, fondu entre deux formes,
enveloppe à 4 poignées de coin déformables, extrusion en relief 3D
simulé, PowerClip pour masquer un contenu dans un contenant), documents
multi-pages, et automatisation (enregistrement/rejeu de macros). Voir
[`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) pour la feuille de route
complète et les étapes à venir.

## Licence

Logiciel propriétaire — tous droits réservés. Voir [`LICENSE`](LICENSE).
