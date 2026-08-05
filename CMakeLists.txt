cmake_minimum_required(VERSION 3.21)
project(AGDraw LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Modules (activés au fur et à mesure que leur code est ajouté) :
# ui, engine, io, layers, text, images, effects, ai, cloud, plugins
# Voir docs/ARCHITECTURE.md pour le détail de chaque module.
# find_package(Skia REQUIRED) — sera ajouté à l'Étape 2 (moteur de dessin).

add_subdirectory(src/ui)
add_subdirectory(apps/agdraw)
