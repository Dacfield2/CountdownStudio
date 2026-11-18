# Release packaging plan

## English

This is the proposed public-release layout. It prepares public-facing downloads while the repository remains private; GitHub Release assets in a private repository are only available to people who can read that repository.

### Build and package tools

- Build the C++20 plugin in GitHub Actions with CMake and the native toolchain for each runner: MSVC on Windows, GCC on Ubuntu, and Apple Clang on macOS.
- Generate packages with CPack. Use the existing NSIS installer plus ZIP on Windows; add a Debian package and a portable tarball on Ubuntu; generate a macOS ProductBuild installer, with a ZIP fallback.
- Do not ship a compiler to users. Actions compiles the plugin; users download the ready-to-install package.

### Release matrix

| OBS family | Windows x64 | Ubuntu x86-64 | macOS |
| --- | --- | --- | --- |
| OBS 32 | Legacy layout, NSIS `.exe` and ZIP | Ubuntu 24.04, legacy layout, `.deb` and `.tar.gz` | OBS 32 SDK, `.pkg` and ZIP |
| OBS 33+ | Modern layout, NSIS `.exe` and ZIP | Ubuntu 26.04 LTS, modern layout, `.deb` and `.tar.gz` | OBS 33+ SDK, `.pkg` and ZIP |

For macOS, publish separate Intel and Apple Silicon installers unless a universal binary is built and verified. The `.pkg` should install into the current user's OBS plugin directory and include an uninstall path. Keep the ZIP as a fallback. For Ubuntu, verify the package's target path and install/remove behavior against the OBS loader before publishing the `.deb`; retain the tarball for compatible non-Ubuntu setups.

### GitHub Releases

On a version tag, the workflow should build every supported OBS/OS combination, run the existing checks, create the installer and archive, and attach the files to a draft GitHub Release. Suggested asset names:

- `CountdownStudio-<version>-OBS32-Windows-x64-Setup.exe`
- `CountdownStudio-<version>-OBS33-Windows-x64-Setup.exe`
- `CountdownStudio-<version>-OBS32-Ubuntu-24.04-x86_64.deb`
- `CountdownStudio-<version>-OBS33-Ubuntu-26.04-x86_64.deb`
- `CountdownStudio-<version>-OBS32-macOS-<arch>.pkg`
- `CountdownStudio-<version>-OBS33-macOS-<arch>.pkg`

Every archive should include its GPL-2.0-or-later license and matching source files. Do not mark a release as compatible until the OBS module has compiled and loaded on that target. The current CI tests only the platform-independent core, so Ubuntu/macOS plugin stability is not yet established.

### Public access

Keeping this repository private also keeps its releases restricted to people with repository access. When public downloads are wanted, either change the repository visibility or publish a public source-and-release mirror. No visibility change is part of this plan.

## Français

Voici la structure proposée pour une publication publique. Elle prépare des téléchargements destinés au public tout en gardant le dépôt privé; les fichiers d’une Release GitHub privée ne sont accessibles qu’aux personnes autorisées à lire le dépôt.

### Outils de compilation et de paquetage

- Compiler le plugin C++20 dans GitHub Actions avec CMake et l’outil natif de chaque système : MSVC sous Windows, GCC sous Ubuntu et Apple Clang sous macOS.
- Créer les paquets avec CPack. Garder l’installateur NSIS et le ZIP déjà prévus sous Windows; ajouter un paquet Debian et une archive portable sous Ubuntu; créer un installateur macOS ProductBuild, avec un ZIP de secours.
- Ne pas distribuer de compilateur aux utilisateurs. Actions compile le plugin; les utilisateurs téléchargent directement le paquet prêt à installer.

### Matrice de publication

| Famille OBS | Windows x64 | Ubuntu x86-64 | macOS |
| --- | --- | --- | --- |
| OBS 32 | Format historique, installateur NSIS `.exe` et ZIP | Ubuntu 24.04, format historique, `.deb` et `.tar.gz` | SDK OBS 32, `.pkg` et ZIP |
| OBS 33+ | Format moderne, installateur NSIS `.exe` et ZIP | Ubuntu 26.04 LTS, format moderne, `.deb` et `.tar.gz` | SDK OBS 33+, `.pkg` et ZIP |

Pour macOS, publier des installateurs Intel et Apple Silicon distincts, sauf si un binaire universel est compilé et vérifié. Le `.pkg` devrait installer le plugin dans le dossier OBS de l’utilisateur courant et inclure une procédure de désinstallation. Conserver le ZIP en solution de secours. Sous Ubuntu, vérifier l’emplacement du paquet et son installation/désinstallation avec le chargeur OBS avant de publier le `.deb`; garder l’archive pour les environnements compatibles hors Ubuntu.

### GitHub Releases

À chaque étiquette de version, le workflow devrait compiler chaque combinaison OBS/système prise en charge, lancer les vérifications existantes, créer l’installateur et l’archive, puis joindre les fichiers à une Release GitHub en brouillon. Noms de fichiers proposés :

- `CountdownStudio-<version>-OBS32-Windows-x64-Setup.exe`
- `CountdownStudio-<version>-OBS33-Windows-x64-Setup.exe`
- `CountdownStudio-<version>-OBS32-Ubuntu-24.04-x86_64.deb`
- `CountdownStudio-<version>-OBS33-Ubuntu-26.04-x86_64.deb`
- `CountdownStudio-<version>-OBS32-macOS-<arch>.pkg`
- `CountdownStudio-<version>-OBS33-macOS-<arch>.pkg`

Chaque paquet doit contenir la licence GPL-2.0-or-later et les fichiers source correspondants. Ne pas annoncer une compatibilité avant d’avoir compilé et chargé le module OBS sur le système cible. La CI actuelle ne teste que le cœur indépendant de la plateforme; la stabilité du plugin sous Ubuntu et macOS n’est donc pas encore établie.

### Accès public

Garder ce dépôt privé réserve aussi ses Releases aux personnes qui y ont accès. Pour proposer des téléchargements publics, il faudra rendre le dépôt public ou publier un miroir public des sources et des paquets. Ce plan ne modifie pas la visibilité du dépôt.
