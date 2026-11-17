# Building Countdown Studio

## English

### Requirements

- CMake 3.28 or newer
- A C++20 compiler
- Ninja for the supplied configure preset (or configure manually with another generator)

### Core and tests

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default
```

The default preset builds only the platform-independent core and its tests.

### OBS module

Install a matching OBS Studio development environment that exports the `libobs` and `obs-frontend-api` CMake packages, and install Qt 6 Widgets. Then configure and build:

```sh
cmake -S . -B build-obs -DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON -DBUILD_TESTING=ON
cmake --build build-obs --config Release
ctest --test-dir build-obs -C Release --output-on-failure
```

The exact SDK setup and packaging steps depend on the OBS version and target platform. This bootstrap does not bundle OBS or its SDK.

To create a portable ZIP from the install rules for the current platform, run:

```sh
cpack --config build-obs/CPackConfig.cmake
```

The archive includes the OBS module, locale files, browser remote-control page, corresponding source files, and the license. It is not a signed platform installer.

Platform install commands and the steps for adding the source in OBS are documented in the bilingual [README](../README.md).

## Français

### Prérequis

- CMake 3.28 ou plus récent
- Un compilateur C++20
- Ninja pour le preset fourni (ou une configuration manuelle avec un autre générateur)

### Cœur et tests

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default
```

Le preset par défaut compile uniquement le cœur indépendant de la plateforme et ses tests.

### Module OBS

Installez un environnement de développement OBS Studio compatible qui fournit les paquets CMake `libobs` et `obs-frontend-api`, ainsi que Qt 6 Widgets. Configurez puis compilez :

```sh
cmake -S . -B build-obs -DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON -DBUILD_TESTING=ON
cmake --build build-obs --config Release
ctest --test-dir build-obs -C Release --output-on-failure
```

La préparation exacte du SDK et l’empaquetage dépendent de la version d’OBS et de la plateforme cible. Ce projet initial ne fournit ni OBS ni son SDK.

Pour créer une archive ZIP portable à partir des règles d’installation de la plateforme actuelle :

```sh
cpack --config build-obs/CPackConfig.cmake
```

L’archive contient le module OBS, les fichiers de langue, la page de commande navigateur, les sources correspondantes et la licence. Ce n’est pas un installateur signé propre à une plateforme.

Les commandes d’installation par plateforme et les étapes pour ajouter la source dans OBS figurent dans le [README bilingue](../README.md).
