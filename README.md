# Countdown Studio

## English

Countdown Studio is a cross-platform OBS Studio countdown project. Its timer engine is kept independent from OBS so it can support scene overlays and OBS controls without coupling timer behavior to a particular interface.

### Project status

The project includes a C++20 timer engine, a native OBS countdown source and control dock, a local browser control page, live time extensions, visual settings, configuration examples, and English/French localization. The OBS module requires matching `libobs`, `obs-frontend-api`, and Qt 6 development packages. The source renders a transparent 1280×360 countdown overlay; OBS can scale it to any canvas resolution. The project is licensed under [GPL-2.0-or-later](LICENSE).

### Build the core and run its tests

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default
```

To configure the OBS module, install an OBS development environment that provides the `libobs` and `obs-frontend-api` CMake packages, plus Qt 6 Widgets, then configure with `-DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON`.

### Install and use it in OBS

Build against the OBS development package for the same OBS version and CPU architecture as the OBS application:

```sh
cmake -S . -B build-obs -DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON -DBUILD_TESTING=ON
cmake --build build-obs --config Release
```

Install the plugin into the standard user plug-in folder for your platform:

**Windows (OBS 34+):**

```powershell
cmake --install build-obs --prefix "C:/ProgramData/obs-studio/plugins/CountdownStudio"
```

**macOS:**

```sh
cmake --install build-obs --prefix "$HOME/Library/Application Support/obs-studio/plugins"
```

**Linux (native OBS):**

```sh
cmake --install build-obs --prefix "$HOME/.config/obs-studio/plugins/CountdownStudio"
```

Then fully restart OBS. In **Sources**, select **+ → Countdown Studio Timer**. Set the duration and choose whether it starts automatically or shows a translucent black background. Use **Start / Resume**, **Pause**, and **Restart countdown** in the source properties to control it. Start / Resume after the timer reaches zero starts a fresh countdown using its configured duration. While it runs, **Add 1 minute** and **Add 5 minutes** extend the current countdown immediately; they also work while paused, and adding time after zero starts the timer again. The automatic format shows `MM:SS` below one hour and `HH:MM:SS` from one hour. Transform it in the scene to fit your canvas.

Open **Docks → Countdown Studio Controls** for the same controls inside OBS. In the source properties you can also choose the font and style, text/background colors and transparency, text size, and automatic, `MM:SS`, or `HH:MM:SS` formatting.

### Browser controls and portable package

Enable **Tools → WebSocket Server Settings** in OBS. Open `data/web/control.html` from the project or installed plug-in folder in a local browser, connect to `ws://127.0.0.1:4455`, and select the Countdown Studio source. The page can start, pause, restart, and add 1, 5, or a chosen number of minutes to the same OBS source. Enter your OBS WebSocket password if authentication is enabled; it is not saved by the page. This page is a control panel, not a Browser Source overlay.

Generate a portable ZIP for the current platform after configuring and building the OBS module:

```sh
cpack --config build-obs/CPackConfig.cmake
```

Extract the ZIP into the platform's OBS plug-in directory shown above. Each archive includes the matching source files and license under `source/`. CPack creates portable archives; it does not create signed MSI, PKG, or DEB installers.

Older Windows OBS installs may use the legacy `bin/64bit` plug-in folder. For Flatpak OBS, install through the matching Flatpak plug-in package or its documented plug-in path. See the [OBS Plugins Guide](https://obsproject.com/kb/plugins-guide) for platform-specific locations.

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

Le projet comprend un moteur de temps C++20, une source de compte à rebours et un panneau de commande OBS natifs, une page de commande dans le navigateur, l’ajout de temps en direct, des réglages visuels, des exemples de configuration et la localisation française/anglaise. Le module OBS nécessite les paquets de développement compatibles `libobs`, `obs-frontend-api` et Qt 6 Widgets. La source affiche une incrustation transparente de 1280×360 qu’OBS peut redimensionner pour toute résolution de canevas. Le projet est distribué sous licence [GPL-2.0-or-later](LICENSE).

### Compiler le cœur et lancer ses tests

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default
```

Pour configurer le module OBS, installez un environnement de développement OBS qui fournit les paquets CMake `libobs` et `obs-frontend-api`, ainsi que Qt 6 Widgets, puis configurez avec `-DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON`.

### Installer et utiliser le plugin dans OBS

Compilez avec le paquet de développement OBS correspondant à la même version d’OBS et à la même architecture processeur que l’application OBS :

```sh
cmake -S . -B build-obs -DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON -DBUILD_TESTING=ON
cmake --build build-obs --config Release
```

Installez le plugin dans le dossier utilisateur prévu pour votre plateforme :

**Windows (OBS 34+) :**

```powershell
cmake --install build-obs --prefix "C:/ProgramData/obs-studio/plugins/CountdownStudio"
```

**macOS :**

```sh
cmake --install build-obs --prefix "$HOME/Library/Application Support/obs-studio/plugins"
```

**Linux (OBS natif) :**

```sh
cmake --install build-obs --prefix "$HOME/.config/obs-studio/plugins/CountdownStudio"
```

Redémarrez complètement OBS. Dans **Sources**, choisissez **+ → Minuteur Countdown Studio**. Réglez la durée et indiquez si le minuteur démarre automatiquement ou affiche un fond noir translucide. Les boutons **Démarrer / reprendre**, **Mettre en pause** et **Réinitialiser le compte à rebours** dans les propriétés de la source le contrôlent. Après zéro, **Démarrer / reprendre** lance un nouveau décompte avec la durée configurée. Pendant le décompte, **Ajouter 1 minute** et **Ajouter 5 minutes** prolongent immédiatement le temps restant; ils fonctionnent aussi en pause, et ajoutés après zéro, ils relancent le minuteur. Le format automatique affiche `MM:SS` sous une heure et `HH:MM:SS` à partir d’une heure. Transformez la source dans la scène pour l’adapter au canevas.

Ouvrez **Docks → Commandes Countdown Studio** pour retrouver les commandes dans OBS. Dans les propriétés de la source, vous pouvez aussi choisir la police et le style, la couleur et la transparence du texte et du fond, la taille du texte ainsi que le format automatique, `MM:SS` ou `HH:MM:SS`.

### Commandes navigateur et archive portable

Activez **Outils → Paramètres du serveur WebSocket** dans OBS. Ouvrez `data/web/control.html` depuis le dépôt ou le dossier du plugin installé dans un navigateur local, connectez-vous à `ws://127.0.0.1:4455`, puis choisissez la source Countdown Studio. La page peut démarrer, mettre en pause, réinitialiser et ajouter 1, 5 ou un nombre choisi de minutes au même minuteur OBS. Saisissez le mot de passe WebSocket OBS si l’authentification est activée; la page ne l’enregistre pas. Cette page sert de télécommande; ce n’est pas une incrustation Browser Source.

Après avoir configuré et compilé le module OBS, générez une archive ZIP portable pour la plateforme actuelle :

```sh
cpack --config build-obs/CPackConfig.cmake
```

Décompressez l’archive dans le dossier de plugins OBS indiqué plus haut. Chaque archive contient les sources correspondantes et la licence dans `source/`. CPack produit des archives portables; il ne crée pas d’installateurs MSI, PKG ou DEB signés.

Les anciennes versions Windows d’OBS peuvent utiliser le dossier historique `bin/64bit`. Pour OBS Flatpak, installez le plugin via le paquet Flatpak compatible ou suivez le chemin de plugins documenté pour cette installation. Consultez le [guide officiel des plugins OBS](https://obsproject.com/kb/plugins-guide) pour les dossiers propres à chaque plateforme.

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
