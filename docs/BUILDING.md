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

Install an OBS Studio development environment that exports the `libobs` CMake package. Then configure and build:

```sh
cmake -S . -B build-obs -DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON -DBUILD_TESTING=ON
cmake --build build-obs --config Release
ctest --test-dir build-obs -C Release --output-on-failure
```

The exact SDK setup and packaging steps depend on the OBS version and target platform. This bootstrap does not bundle OBS or its SDK.

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

Installez un environnement de développement OBS Studio qui fournit le paquet CMake `libobs`. Configurez puis compilez :

```sh
cmake -S . -B build-obs -DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON -DBUILD_TESTING=ON
cmake --build build-obs --config Release
ctest --test-dir build-obs -C Release --output-on-failure
```

La préparation exacte du SDK et l’empaquetage dépendent de la version d’OBS et de la plateforme cible. Ce projet initial ne fournit ni OBS ni son SDK.
