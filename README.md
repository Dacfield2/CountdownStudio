# Countdown Studio

## English

Countdown Studio is a cross-platform OBS Studio countdown project. Its timer engine is kept independent from OBS so it can support scene overlays and OBS controls without coupling timer behavior to a particular interface.

### Project status

The project includes a C++20 timer engine, a native OBS countdown source and control dock, live time extensions, visual settings, custom font file loading, configuration examples, and English/French localization. All user-facing controls are available directly inside OBS. The Windows release workflow builds separate packages against OBS Studio 32.2.2 (legacy layout) and 33.0.1 (modern layout). The OBS module has not yet been built or load-verified on Ubuntu or macOS. The OBS module requires matching `libobs`, `obs-frontend-api`, and Qt 6 development packages. The source renders a transparent 1280×360 countdown overlay; OBS can scale it to any canvas resolution. The project is licensed under [GPL-2.0-or-later](LICENSE).

### Build the core and run its tests

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default
```

To configure the OBS module, install an OBS development environment that provides the `libobs` and `obs-frontend-api` CMake packages, plus Qt 6 Widgets, then configure with `-DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON`.

### Build Windows OBS packages

With Visual Studio 2026, CMake, Windows SDK 10.0.26100 or newer, and NSIS 3.12 installed, build either Windows package with the dependency-verified script. OBS 33's current supported Windows toolchain uses Visual Studio 2026; the hosted release workflow provides Visual Studio and installs NSIS automatically.

```powershell
pwsh -File scripts/Build-Windows-Obs.ps1 -ObsVersion 32.2.2
pwsh -File scripts/Build-Windows-Obs.ps1 -ObsVersion 33.0.1
```

The script downloads the matching OBS source and development dependencies, checks their SHA-256 hashes, builds the OBS development libraries and Countdown Studio, then creates and checks the NSIS installer and ZIP in `build-obs-<version>/packages/`. The GitHub workflow retries the NSIS installation up to three times if Chocolatey is temporarily unavailable.

### Install and use it in OBS

Build a separate package for each OBS SDK family and CPU architecture. Use the OBS 32 development package for OBS 32; use the OBS 33 or newer development package for OBS 33+:

```sh
# OBS 32 package
cmake -S . -B build-obs32 -DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON -DBUILD_TESTING=ON -DCOUNTDOWNSTUDIO_OBS_PLUGIN_LAYOUT=legacy
cmake --build build-obs32 --config Release

# OBS 33+ package
cmake -S . -B build-obs33 -DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON -DBUILD_TESTING=ON -DCOUNTDOWNSTUDIO_OBS_PLUGIN_LAYOUT=modern
cmake --build build-obs33 --config Release
```

OBS 32 rejects plugins built for a newer OBS release. OBS 33 still loads legacy-layout plugins but marks them as legacy; OBS 34 is planned to remove that fallback. See the [OBS 32 release notes](https://obsproject.com/blog/obs-studio-32-0-release-notes) and the [plugin location migration guide](https://obsproject.com/kb/legacy-plugin-locations).

### Supported OS baseline

For a stable upstream OBS base, use Ubuntu 24.04 with OBS 32, Ubuntu 26.04 LTS with OBS 33+, and macOS 13 or newer with either family. OBS 33 removed Ubuntu 24.04 support; its current Linux packages target Ubuntu 26.04+, and its macOS build targets macOS 13+. These are OBS platform requirements, not a claim that this plugin has already passed an OBS load test on those systems. Our current CI still builds only the platform-independent core on Ubuntu and macOS.

Install the package matching the OBS version family:

**Windows (OBS 32, legacy layout):**

```powershell
cmake --install build-obs32 --prefix "C:/ProgramData/obs-studio/plugins/CountdownStudio"
```

The module is placed under `bin/64bit/` for OBS 32.

**Windows (OBS 33+):**

```powershell
cmake --install build-obs33 --prefix "C:/ProgramData/obs-studio/plugins/CountdownStudio"
```

**macOS 13+ (OBS 32+; use the matching SDK build):**

```sh
cmake --install build-obs33 --prefix "$HOME/Library/Application Support/obs-studio/plugins"
```

**Ubuntu 24.04 with OBS 32 (legacy layout):**

```sh
cmake --install build-obs32 --prefix "$HOME/.config/obs-studio/plugins/CountdownStudio"
```

**Ubuntu 26.04 LTS with OBS 33+ (modern layout):**

```sh
cmake --install build-obs33 --prefix "$HOME/.local/share/obs-studio/plugins/CountdownStudio"
```

Then fully restart OBS. In **Sources**, select **+ → Countdown Studio Timer**. Set the duration and choose whether it starts automatically or shows a translucent black background. Use **Start / Resume**, **Pause**, and **Restart countdown** in the source properties to control it. Start / Resume after the timer reaches zero starts a fresh countdown using its configured duration. While it runs, **Add 1 minute** and **Add 5 minutes** extend the current countdown immediately; they also work while paused, and adding time after zero starts the timer again. The automatic format shows `MM:SS` below one hour and `HH:MM:SS` from one hour. Transform it in the scene to fit your canvas.

Open **Docks → Countdown Studio Controls** for the native OBS control panel. It can start, pause, restart, add 1 or 5 minutes, add a chosen number of minutes (1–120), and open the selected source's OBS properties. In those properties you can set duration, auto-start, font and style, text/background colors and transparency, text size, and automatic, `MM:SS`, or `HH:MM:SS` formatting.

### Use a custom font

Open the timer source's **Properties**, then choose a `.ttf`, `.otf`, or `.ttc` file in **Custom font file**. Leave that field empty to use the font and style selected in **Font**. The chosen font is loaded locally by OBS; Countdown Studio stores its file path but does not copy the font into the scene or installer. Keep the file at that path. If you move the scene to another computer, install the same font there or select its local font file again. If the file cannot be loaded, the source falls back to the font selected in **Font** and logs the failure in OBS.

### Installation package

After configuring and building the OBS module for the right OBS family, create its installation package:

```sh
cpack --config build-obs/CPackConfig.cmake
```

On Windows, CPack creates a guided installer and a ZIP. The guided installer uses `%ProgramData%\obs-studio\plugins` as its default destination and installs the plugin in the `CountdownStudio` subfolder. For portable OBS, choose `<OBS folder>\plugins\CountdownStudio` in the installer. Use the `legacy` package for OBS 32 or the `modern` package for OBS 33+. The Windows workflow produces packages for OBS 32.2.2 and OBS 33.0.1, each with a SHA-256 checksum file. On macOS and Linux, CPack creates a ZIP; extract it into the matching OBS plugin folder shown above. Each package includes the corresponding source files and license under `source/`. The Windows installer is not code-signed. The repository is private, so its GitHub Releases are only available to repository members.

The current Windows package outputs and remaining platform packaging work are described in [Release packaging](docs/RELEASE-PACKAGING.md).

### Publish a GitHub Release manually

Each push to `main`, or a manual run from **Actions → Windows OBS packages → Run workflow**, builds and checks the installers without publishing a Release. To publish, push a version tag matching the CMake version (for example, `v0.1.0` for project version `0.1.0`). The workflow builds the OBS 32.2.2 and OBS 33.0.1 packages and publishes them with SHA-256 checksums after both builds succeed. Ubuntu and macOS plugin packages are not included yet because their OBS module builds have not been verified. Since the repository is private, only people with repository access can download its releases.

For OBS 32 portable installs, use the legacy plugin path documented for that OBS build; for OBS 33+ portable installs, place the modern plugin folder inside the OBS portable root. For Flatpak OBS, use a package compatible with that Flatpak runtime. See the [OBS Plugins Guide](https://obsproject.com/kb/plugins-guide) and [plugin location migration guide](https://obsproject.com/kb/legacy-plugin-locations) for platform-specific locations.

### Layout

- `include/countdownstudio/` — public C++ timer API
- `src/core/` — platform-independent countdown logic
- `src/obs/` — OBS module entry point and countdown source
- `config/` — example settings and JSON schema
- `data/locale/` — English and French OBS strings
- `docs/` — architecture and build notes
- `tests/` — dependency-free core tests

See [Building](docs/BUILDING.md), [Architecture](docs/ARCHITECTURE.md), and [Contributing](CONTRIBUTING.md).

### README language policy

Keep this README bilingual in every patch. When a patch changes behavior, build steps, configuration, or user-visible features, update both language sections.

## Français

Countdown Studio est un projet de compte à rebours multiplateforme pour OBS Studio. Le moteur de temps reste indépendant d’OBS afin de prendre en charge l’affichage dans les scènes et les commandes OBS sans lier le comportement du minuteur à une interface particulière.

### État du projet

Le projet comprend un moteur de temps C++20, une source de compte à rebours et un panneau de commande OBS natifs, l’ajout de temps en direct, des réglages visuels, le chargement de fichiers de police personnalisés, des exemples de configuration et la localisation française/anglaise. Toutes les commandes destinées à l’utilisateur sont disponibles directement dans OBS. Le workflow Windows compile des paquets distincts pour OBS Studio 32.2.2 (format historique) et 33.0.1 (nouveau format). Le module OBS n’a pas encore été compilé ni vérifié comme plugin chargé sous Ubuntu ou macOS. Il nécessite les paquets de développement compatibles `libobs`, `obs-frontend-api` et Qt 6 Widgets. La source affiche une incrustation transparente de 1280×360 qu’OBS peut redimensionner pour toute résolution de canevas. Le projet est distribué sous licence [GPL-2.0-or-later](LICENSE).

### Compiler le cœur et lancer ses tests

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default
```

Pour configurer le module OBS, installez un environnement de développement OBS qui fournit les paquets CMake `libobs` et `obs-frontend-api`, ainsi que Qt 6 Widgets, puis configurez avec `-DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON`.

### Compiler les paquets Windows pour OBS

Avec Visual Studio 2026, CMake, le SDK Windows 10 version 10.0.26100 ou ultérieure et NSIS 3.12, compilez l’un ou l’autre des paquets Windows à l’aide du script qui vérifie les dépendances. La version Windows prise en charge par OBS 33 utilise Visual Studio 2026; le workflow de publication fournit Visual Studio et installe NSIS automatiquement.

```powershell
pwsh -File scripts/Build-Windows-Obs.ps1 -ObsVersion 32.2.2
pwsh -File scripts/Build-Windows-Obs.ps1 -ObsVersion 33.0.1
```

Le script télécharge les sources OBS et les dépendances de développement correspondantes, vérifie leurs empreintes SHA-256, compile les bibliothèques de développement OBS et Countdown Studio, puis crée et vérifie l’installateur NSIS et le ZIP dans `build-obs-<version>/packages/`. Le workflow GitHub réessaie jusqu’à trois fois l’installation de NSIS si Chocolatey est temporairement indisponible.

### Installer et utiliser le plugin dans OBS

Compilez un paquet distinct pour chaque famille de SDK OBS et architecture processeur. Utilisez le paquet de développement OBS 32 pour OBS 32, et celui d’OBS 33 ou plus récent pour OBS 33+ :

```sh
# Paquet OBS 32
cmake -S . -B build-obs32 -DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON -DBUILD_TESTING=ON -DCOUNTDOWNSTUDIO_OBS_PLUGIN_LAYOUT=legacy
cmake --build build-obs32 --config Release

# Paquet OBS 33+
cmake -S . -B build-obs33 -DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON -DBUILD_TESTING=ON -DCOUNTDOWNSTUDIO_OBS_PLUGIN_LAYOUT=modern
cmake --build build-obs33 --config Release
```

OBS 32 refuse les plugins compilés pour une version OBS plus récente. OBS 33 charge encore les plugins au format historique, mais les marque comme anciens; OBS 34 doit retirer cette compatibilité. Consultez les [notes de version d’OBS 32](https://obsproject.com/blog/obs-studio-32-0-release-notes) et le [guide de migration des dossiers de plugins](https://obsproject.com/kb/legacy-plugin-locations).

### Systèmes recommandés

Pour utiliser une version de base OBS stable, choisissez Ubuntu 24.04 avec OBS 32, Ubuntu 26.04 LTS avec OBS 33+, et macOS 13 ou plus récent avec les deux familles. OBS 33 a retiré la prise en charge d’Ubuntu 24.04; ses paquets Linux actuels ciblent Ubuntu 26.04+, et sa version macOS cible macOS 13+. Ce sont les exigences de plateforme d’OBS, pas la preuve que ce plugin a déjà passé un test de chargement dans ces systèmes. La CI actuelle compile seulement le cœur indépendant de la plateforme sous Ubuntu et macOS.

Installez le paquet correspondant à la famille de versions OBS :

**Windows (OBS 32, format historique) :**

```powershell
cmake --install build-obs32 --prefix "C:/ProgramData/obs-studio/plugins/CountdownStudio"
```

Le module est placé dans `bin/64bit/` pour OBS 32.

**Windows (OBS 33+) :**

```powershell
cmake --install build-obs33 --prefix "C:/ProgramData/obs-studio/plugins/CountdownStudio"
```

**macOS 13+ (OBS 32+ ; utiliser la compilation du SDK correspondant) :**

```sh
cmake --install build-obs33 --prefix "$HOME/Library/Application Support/obs-studio/plugins"
```

**Ubuntu 24.04 avec OBS 32 (format historique) :**

```sh
cmake --install build-obs32 --prefix "$HOME/.config/obs-studio/plugins/CountdownStudio"
```

**Ubuntu 26.04 LTS avec OBS 33+ (format moderne) :**

```sh
cmake --install build-obs33 --prefix "$HOME/.local/share/obs-studio/plugins/CountdownStudio"
```

Redémarrez complètement OBS. Dans **Sources**, choisissez **+ → Minuteur Countdown Studio**. Réglez la durée et indiquez si le minuteur démarre automatiquement ou affiche un fond noir translucide. Les boutons **Démarrer / reprendre**, **Mettre en pause** et **Réinitialiser le compte à rebours** dans les propriétés de la source le contrôlent. Après zéro, **Démarrer / reprendre** lance un nouveau décompte avec la durée configurée. Pendant le décompte, **Ajouter 1 minute** et **Ajouter 5 minutes** prolongent immédiatement le temps restant; ils fonctionnent aussi en pause, et ajoutés après zéro, ils relancent le minuteur. Le format automatique affiche `MM:SS` sous une heure et `HH:MM:SS` à partir d’une heure. Transformez la source dans la scène pour l’adapter au canevas.

Ouvrez **Docks → Commandes Countdown Studio** pour afficher le panneau natif. Il permet de démarrer, mettre en pause, réinitialiser, ajouter 1 ou 5 minutes, ajouter un nombre choisi de minutes (1 à 120) et ouvrir les propriétés OBS de la source sélectionnée. Ces propriétés permettent de régler la durée, le démarrage automatique, la police et son style, les couleurs et la transparence du texte et du fond, la taille ainsi que le format automatique, `MM:SS` ou `HH:MM:SS`.

### Utiliser une police personnalisée

Ouvrez les **Propriétés** de la source minuteur, puis choisissez un fichier `.ttf`, `.otf` ou `.ttc` dans **Fichier de police personnalisé**. Laissez ce champ vide pour utiliser la police et le style choisis dans **Police**. OBS charge la police localement; Countdown Studio mémorise le chemin du fichier sans copier la police dans la scène ou l’installateur. Gardez le fichier à cet emplacement. Si vous déplacez la scène vers un autre ordinateur, installez-y la même police ou sélectionnez son fichier local. Si le chargement échoue, la source reprend la police choisie dans **Police** et consigne l’échec dans le journal OBS.

### Paquet d’installation

Après avoir configuré et compilé le module OBS pour la bonne famille OBS, créez son paquet d’installation :

```sh
cpack --config build-obs/CPackConfig.cmake
```

Sous Windows, CPack crée un installateur guidé et un fichier ZIP. L’installateur guidé utilise `%ProgramData%\obs-studio\plugins` comme dossier de destination par défaut et installe le plugin dans le sous-dossier `CountdownStudio`. Pour OBS portable, choisissez `<dossier OBS>\plugins\CountdownStudio` dans l’installateur. Utilisez le paquet `legacy` pour OBS 32 ou `modern` pour OBS 33+. Le workflow Windows crée des paquets pour OBS 32.2.2 et OBS 33.0.1, chacun accompagné d’un fichier de somme SHA-256. Sous macOS et Linux, CPack crée une archive ZIP à extraire dans le dossier OBS correspondant indiqué plus haut. Chaque paquet contient les sources correspondantes et la licence dans `source/`. L’installateur Windows n’est pas signé. Le dépôt est privé; ses GitHub Releases sont donc réservées aux personnes qui y ont accès.

Les paquets Windows actuels et le travail restant pour les autres systèmes sont décrits dans [Préparation des paquets de publication](docs/RELEASE-PACKAGING.md).

### Publier manuellement une Release GitHub

Chaque poussée sur `main`, ou un lancement manuel depuis **Actions → Windows OBS packages → Run workflow**, compile et vérifie les installateurs sans publier de Release. Pour publier, poussez une étiquette de version identique à la version CMake (par exemple `v0.1.0` pour la version du projet `0.1.0`). Le workflow compile les paquets OBS 32.2.2 et OBS 33.0.1 et les publie avec leurs sommes SHA-256 lorsque les deux compilations réussissent. Les paquets plugin Ubuntu et macOS ne sont pas inclus, car la compilation de leur module OBS n’a pas encore été vérifiée. Le dépôt étant privé, seules les personnes autorisées à le lire peuvent télécharger ses Releases.

Pour OBS 32 portable, utilisez l’emplacement historique indiqué pour cette version; pour OBS 33+, placez le dossier du plugin au format moderne dans le dossier portable OBS. Pour OBS Flatpak, utilisez un paquet compatible avec cet environnement Flatpak. Consultez le [guide officiel des plugins OBS](https://obsproject.com/kb/plugins-guide) et le [guide de migration des dossiers de plugins](https://obsproject.com/kb/legacy-plugin-locations) pour les emplacements par plateforme.

### Arborescence

- `include/countdownstudio/` — API C++ publique du minuteur
- `src/core/` — logique de compte à rebours indépendante de la plateforme
- `src/obs/` — point d’entrée du module OBS et source du minuteur
- `config/` — exemple de paramètres et schéma JSON
- `data/locale/` — textes OBS en français et en anglais
- `docs/` — notes d’architecture et de compilation
- `tests/` — tests du cœur sans dépendance externe

Consultez [Compilation](docs/BUILDING.md), [Architecture](docs/ARCHITECTURE.md) et [Contribution](CONTRIBUTING.md).

### Politique linguistique du README

Conservez ce README en français et en anglais dans chaque patch. Si un patch modifie le comportement, la compilation, la configuration ou une fonctionnalité visible, mettez à jour les deux sections linguistiques.
