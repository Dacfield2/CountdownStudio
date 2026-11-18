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

The compatibility target starts at OBS 32. Build separate plugin packages against the matching OBS SDK family: OBS 32 rejects plugins built for a newer OBS release. OBS 32 uses the legacy layout, while OBS 33 and newer use the modern layout. Select it with `-DCOUNTDOWNSTUDIO_OBS_PLUGIN_LAYOUT=legacy` or `modern`. For upstream OBS 32 use Ubuntu 24.04; OBS 33 has dropped Ubuntu 24.04, so use Ubuntu 26.04 LTS for OBS 33+. The current OBS 33 macOS target is macOS 13+. These are OBS platform baselines; Countdown Studio's OBS module has not yet been validated as loaded on Linux or macOS. The GitHub Actions package workflow builds Linux x64 packages on Ubuntu 24.04 for OBS 32 and Ubuntu 26.04 for OBS 33+. Successful workflow runs upload ZIP artifacts; version releases include them after both Linux and Windows builds pass. This project does not bundle OBS or its SDK.

To create a portable ZIP from the install rules for the current platform, run:

```sh
cpack --config build-obs/CPackConfig.cmake
```

The archive includes the OBS module, locale files, corresponding source files, and the license. It is not a signed platform installer.

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

L’objectif de compatibilité commence avec OBS 32. Compilez des paquets distincts avec la famille de SDK OBS correspondante : OBS 32 refuse les plugins compilés pour une version OBS plus récente. OBS 32 utilise l’ancien format, OBS 33 et les versions ultérieures le format moderne. Sélectionnez-le avec `-DCOUNTDOWNSTUDIO_OBS_PLUGIN_LAYOUT=legacy` ou `modern`. Pour OBS 32, utilisez Ubuntu 24.04; OBS 33 ayant retiré Ubuntu 24.04, utilisez Ubuntu 26.04 LTS pour OBS 33+. La cible macOS actuelle d’OBS 33 nécessite macOS 13+. Ce sont les bases de plateforme d’OBS; le chargement du module Countdown Studio n’a pas encore été validé sous Linux ou macOS. Le workflow GitHub Actions compile les paquets Linux x64 sous Ubuntu 24.04 pour OBS 32 et Ubuntu 26.04 pour OBS 33+. Les exécutions réussies fournissent des ZIP en tant qu’artefacts; les releases les incluent si les compilations Linux et Windows réussissent. Ce projet ne fournit ni OBS ni son SDK.

Pour créer une archive ZIP portable à partir des règles d’installation de la plateforme actuelle :

```sh
cpack --config build-obs/CPackConfig.cmake
```

L’archive contient le module OBS, les fichiers de langue, les sources correspondantes et la licence. Ce n’est pas un installateur signé propre à une plateforme.

Les commandes d’installation par plateforme et les étapes pour ajouter la source dans OBS figurent dans le [README bilingue](../README.md).
