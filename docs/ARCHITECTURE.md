# AG Draw — Architecture

## Vision

AG Draw n'est pas un simple clone de CorelDRAW. L'objectif est un logiciel de dessin
vectoriel professionnel, augmenté par l'IA, capable de :

- Dessiner à partir d'une phrase (texte → vecteur).
- Transformer un croquis en dessin vectoriel propre.
- Corriger automatiquement les formes.
- Générer des logos et affiches en quelques secondes.
- Fonctionner sur Windows, macOS, Linux, Android et Web.
- Ouvrir les formats populaires : SVG, PDF, PNG, JPG, et un format natif AGD.

## Stack technique

| Domaine              | Choix              |
|-----------------------|--------------------|
| Langage                | C++ (C++20)        |
| Interface              | Qt 6               |
| Moteur graphique       | Skia               |
| Formats de fichiers    | SVG, PDF, PNG, JPG, AGD (natif) |
| Contrôle de version    | Git                |

## Modules

```
AG Draw
│
├── Interface utilisateur   (ui)       — fenêtres, panneaux, thèmes
├── Moteur de dessin        (engine)   — rendu Skia, scène, transformations
├── Gestion des fichiers    (io)       — import/export SVG, PDF, PNG, JPG, AGD
├── Gestion des calques     (layers)   — empilement, visibilité, verrouillage
├── Gestion du texte        (text)     — typographie, texte sur chemin
├── Gestion des images      (images)   — bitmaps, filtres, intégration vecteur/raster
├── Moteur d'effets         (effects)  — ombres, dégradés, distorsions
├── Moteur IA               (ai)       — texte→vecteur, croquis→vecteur, auto-correction
├── Cloud                   (cloud)    — synchronisation, comptes, partage
└── Plugins                 (plugins)  — API d'extension tierce
```

Chaque module correspond à un répertoire sous `src/` et pourra devenir une bibliothèque
statique/dynamique compilée séparément à mesure que le projet grossit.

## Écrans

1. Écran de démarrage
2. Fenêtre principale
3. Zone de dessin infinie
4. Barre d'outils à gauche
5. Barre des propriétés en haut
6. Panneau des calques à droite
7. Palette de couleurs
8. Barre d'état en bas

## Méthode (roadmap)

| Étape | Contenu |
|-------|---------|
| 1 | Concevoir l'interface (écrans, panneaux, layout Qt) |
| 2 | Créer le moteur de dessin (intégration Skia dans Qt, scène, rendu) |
| 3 | Ajouter les outils (sélection, rectangle, texte, plume, etc.) |
| 4 | Ajouter les calques et les effets |
| 5 | Intégrer l'IA (texte→vecteur, croquis→vecteur, auto-correction) |
| 6 | Publier une première version |

## Portabilité

Qt 6 et Skia sont choisis pour leur portabilité native (Windows, macOS, Linux) et leur
capacité à cibler Android. La cible Web sera étudiée séparément (Qt for WebAssembly ou
un moteur de rendu web dédié) une fois le cœur du moteur de dessin stabilisé.
