# Countdown Studio

## English

Countdown Studio is a cross-platform OBS Studio countdown project. Its timer engine is kept independent from OBS so it can support scene overlays and OBS controls without coupling timer behavior to a particular interface.

### Project status

The project includes a C++20 timer engine, a native OBS countdown source and control dock, a local browser control page, live time extensions, visual settings, configuration examples, and English/French localization. The compatibility target starts at OBS Studio 32. Build separate plugin packages against the matching OBS SDK family: OBS 32 uses the legacy plugin layout, while OBS 33 and newer use the modern layout. The current automated CI still tests the platform-independent core only; OBS plugin loading must be validated against each supported OBS family before release. The OBS module requires matching `libobs`, `obs-frontend-api`, and Qt 6 development packages. The source renders a transparent 1280×360 countdown overlay; OBS can scale it to any canvas resolution. The project is licensed under [GPL-2.0-or-later](LICENSE).

### Build the core and run its tests

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default
```

To configure the OBS module, install an OBS development environment that provides the `libobs` and `obs-frontend-api` CMake packages, plus Qt 6 Widgets, then configure with `-DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON`.

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

**macOS (OBS 32+; use the matching SDK build):**

```sh
cmake --install build-obs33 --prefix "$HOME/Library/Application Support/obs-studio/plugins"
```

**Linux (OBS 32, legacy layout):**

```sh
cmake --install build-obs32 --prefix "$HOME/.config/obs-studio/plugins/CountdownStudio"
```

**Linux (OBS 33+, modern layout):**

```sh
cmake --install build-obs33 --prefix "$HOME/.local/share/obs-studio/plugins/CountdownStudio"
```

Then fully restart OBS. In **Sources**, select **+ → Countdown Studio Timer**. Set the duration and choose whether it starts automatically or shows a translucent black background. Use **Start / Resume**, **Pause**, and **Restart countdown** in the source properties to control it. Start / Resume after the timer reaches zero starts a fresh countdown using its configured duration. While it runs, **Add 1 minute** and **Add 5 minutes** extend the current countdown immediately; they also work while paused, and adding time after zero starts the timer again. The automatic format shows `MM:SS` below one hour and `HH:MM:SS` from one hour. Transform it in the scene to fit your canvas.

Open **Docks → Countdown Studio Controls** for the same controls inside OBS. In the source properties you can also choose the font and style, text/background colors and transparency, text size, and automatic, `MM:SS`, or `HH:MM:SS` formatting.

### Browser controls and installation package

Enable **Tools → WebSocket Server Settings** in OBS. Open `data/web/control.html` from the project or installed plug-in folder in a local browser, connect to `ws://127.0.0.1:4455`, and select the Countdown Studio source. The page can start, pause, restart, and add 1, 5, or a chosen number of minutes to the same OBS source. Enter your OBS WebSocket password if authentication is enabled; it is not saved by the page. This page is a control panel, not a Browser Source overlay.

After configuring and building the OBS module for the right OBS family, create its installation package:

```sh
cpack --config build-obs/CPackConfig.cmake
```

On Windows, CPack creates a guided installer and a ZIP. The installer defaults to `%ProgramData%\obs-studio\plugins\CountdownStudio`; for portable OBS, choose `<OBS folder>\plugins\CountdownStudio` in the installer. Use the `legacy` build for OBS 32 or the `modern` build for OBS 33+. On macOS and Linux, CPack creates a ZIP; extract it into the matching OBS plugin folder shown above. Each package includes the corresponding source files and license under `source/`. The Windows installer is not code-signed.

For OBS 32 portable installs, use the legacy plugin path documented for that OBS build; for OBS 33+ portable installs, place the modern plugin folder inside the OBS portable root. For Flatpak OBS, use a package compatible with that Flatpak runtime. See the [OBS Plugins Guide](https://obsproject.com/kb/plugins-guide) and [plugin location migration guide](https://obsproject.com/kb/legacy-plugin-locations) for platform-specific locations.

### Layout

- `include/countdownstudio/` — public C++ timer API
- `src/core/` — platform-independent countdown logic
- `src/obs/` — OBS module entry point and countdown source
- `data/web/` — browser-based remote controls for OBS WebSocket
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

Le projet comprend un moteur de temps C++20, une source de compte à rebours et un panneau de commande OBS natifs, une page de commande dans le navigateur, l’ajout de temps en direct, des réglages visuels, des exemples de configuration et la localisation française/anglaise. L’objectif de compatibilité commence avec OBS Studio 32. Il faut compiler des paquets distincts avec la famille de SDK OBS correspondante : OBS 32 utilise l’ancien format de plugin, tandis qu’OBS 33 et les versions ultérieures utilisent le nouveau format. La CI actuelle teste encore uniquement le cœur indépendant de la plateforme; le chargement du plugin OBS doit être vérifié avec chaque famille OBS avant publication. Le module OBS nécessite les paquets de développement compatibles `libobs`, `obs-frontend-api` et Qt 6 Widgets. La source affiche une incrustation transparente de 1280×360 qu’OBS peut redimensionner pour toute résolution de canevas. Le projet est distribué sous licence [GPL-2.0-or-later](LICENSE).

### Compiler le cœur et lancer ses tests

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default
```

Pour configurer le module OBS, installez un environnement de développement OBS qui fournit les paquets CMake `libobs` et `obs-frontend-api`, ainsi que Qt 6 Widgets, puis configurez avec `-DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON`.

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

**macOS (OBS 32+ ; utiliser la compilation du SDK correspondant) :**

```sh
cmake --install build-obs33 --prefix "$HOME/Library/Application Support/obs-studio/plugins"
```

**Linux (OBS 32, format historique) :**

```sh
cmake --install build-obs32 --prefix "$HOME/.config/obs-studio/plugins/CountdownStudio"
```

**Linux (OBS 33+, format moderne) :**

```sh
cmake --install build-obs33 --prefix "$HOME/.local/share/obs-studio/plugins/CountdownStudio"
```

Redémarrez complètement OBS. Dans **Sources**, choisissez **+ → Minuteur Countdown Studio**. Réglez la durée et indiquez si le minuteur démarre automatiquement ou affiche un fond noir translucide. Les boutons **Démarrer / reprendre**, **Mettre en pause** et **Réinitialiser le compte à rebours** dans les propriétés de la source le contrôlent. Après zéro, **Démarrer / reprendre** lance un nouveau décompte avec la durée configurée. Pendant le décompte, **Ajouter 1 minute** et **Ajouter 5 minutes** prolongent immédiatement le temps restant; ils fonctionnent aussi en pause, et ajoutés après zéro, ils relancent le minuteur. Le format automatique affiche `MM:SS` sous une heure et `HH:MM:SS` à partir d’une heure. Transformez la source dans la scène pour l’adapter au canevas.

Ouvrez **Docks → Commandes Countdown Studio** pour retrouver les commandes dans OBS. Dans les propriétés de la source, vous pouvez aussi choisir la police et le style, la couleur et la transparence du texte et du fond, la taille du texte ainsi que le format automatique, `MM:SS` ou `HH:MM:SS`.

### Commandes navigateur et paquet d’installation

Activez **Outils → Paramètres du serveur WebSocket** dans OBS. Ouvrez `data/web/control.html` depuis le dépôt ou le dossier du plugin installé dans un navigateur local, connectez-vous à `ws://127.0.0.1:4455`, puis choisissez la source Countdown Studio. La page peut démarrer, mettre en pause, réinitialiser et ajouter 1, 5 ou un nombre choisi de minutes au même minuteur OBS. Saisissez le mot de passe WebSocket OBS si l’authentification est activée; la page ne l’enregistre pas. Cette page sert de télécommande; ce n’est pas une incrustation Browser Source.

Après avoir configuré et compilé le module OBS pour la bonne famille OBS, créez son paquet d’installation :

```sh
cpack --config build-obs/CPackConfig.cmake
```

Sous Windows, CPack crée un installateur guidé et un fichier ZIP. Le dossier proposé par défaut est `%ProgramData%\obs-studio\plugins\CountdownStudio`; pour OBS portable, choisissez `<dossier OBS>\plugins\CountdownStudio` dans l’installateur. Utilisez la compilation `legacy` pour OBS 32 ou `modern` pour OBS 33+. Sous macOS et Linux, CPack crée une archive ZIP à extraire dans le dossier OBS correspondant indiqué plus haut. Chaque paquet contient les sources correspondantes et la licence dans `source/`. L’installateur Windows n’est pas signé.

Pour OBS 32 portable, utilisez l’emplacement historique indiqué pour cette version; pour OBS 33+, placez le dossier du plugin au format moderne dans le dossier portable OBS. Pour OBS Flatpak, utilisez un paquet compatible avec cet environnement Flatpak. Consultez le [guide officiel des plugins OBS](https://obsproject.com/kb/plugins-guide) et le [guide de migration des dossiers de plugins](https://obsproject.com/kb/legacy-plugin-locations) pour les emplacements par plateforme.

### Arborescence

- `include/countdownstudio/` — API C++ publique du minuteur
- `src/core/` — logique de compte à rebours indépendante de la plateforme
- `src/obs/` — point d’entrée du module OBS et source du minuteur
- `data/web/` — commandes navigateur via OBS WebSocket
- `config/` — exemple de paramètres et schéma JSON
- `data/locale/` — textes OBS en français et en anglais
- `docs/` — notes d’architecture et de compilation
- `tests/` — tests du cœur sans dépendance externe

Consultez [Compilation](docs/BUILDING.md), [Architecture](docs/ARCHITECTURE.md) et [Contribution](CONTRIBUTING.md).

### Politique linguistique du README

Conservez ce README en français et en anglais dans chaque patch. Si un patch modifie le comportement, la compilation, la configuration ou une fonctionnalité visible, mettez à jour les deux sections linguistiques.

