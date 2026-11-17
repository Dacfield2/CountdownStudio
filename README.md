# Countdown Studio

## English

Countdown Studio is a cross-platform OBS Studio countdown project. Its timer engine is kept independent from OBS so it can support scene overlays and OBS controls without coupling timer behavior to a particular interface.

### Project status

The project includes a C++20 timer engine, deterministic tests, a working native OBS timer source, source properties for timer controls, configuration examples, and English/French localization. The OBS module is optional at configure time because a matching OBS development package is required. The source renders a transparent 1280×360 countdown overlay; OBS can scale it to any canvas resolution.

### Build the core and run its tests

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default
```

To configure the OBS module as well, install an OBS development environment that provides the `libobs` CMake package, then configure with `-DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON`.

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

Then fully restart OBS. In **Sources**, select **+ → Countdown Studio Timer**. Set the duration and choose whether it starts automatically or shows a translucent black background. Use **Start / Resume**, **Pause**, and **Restart countdown** in the source properties to control it. The source displays `MM:SS`, or `H:MM:SS` for durations of at least one hour. Transform it in the scene to fit your canvas.

Older Windows OBS installs may use the legacy `bin/64bit` plug-in folder. For Flatpak OBS, install through the matching Flatpak plug-in package or its documented plug-in path. See the [OBS Plugins Guide](https://obsproject.com/kb/plugins-guide) for platform-specific locations.

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

Le projet comprend un moteur de temps C++20, des tests déterministes, une source OBS native fonctionnelle, des commandes dans les propriétés de la source, des exemples de configuration et la localisation française/anglaise. Le module OBS est facultatif à la configuration, car il nécessite un environnement de développement OBS compatible. La source affiche une incrustation transparente de 1280×360 qu’OBS peut redimensionner pour toute résolution de canevas.

### Compiler le cœur et lancer ses tests

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default
```

Pour configurer aussi le module OBS, installez un environnement de développement OBS qui fournit le paquet CMake `libobs`, puis configurez avec `-DCOUNTDOWNSTUDIO_BUILD_OBS_PLUGIN=ON`.

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

Redémarrez complètement OBS. Dans **Sources**, choisissez **+ → Minuteur Countdown Studio**. Réglez la durée et indiquez si le minuteur démarre automatiquement ou affiche un fond noir translucide. Les boutons **Démarrer / reprendre**, **Mettre en pause** et **Réinitialiser le compte à rebours** dans les propriétés de la source le contrôlent. L’affichage utilise `MM:SS`, ou `H:MM:SS` à partir d’une heure. Transformez la source dans la scène pour l’adapter au canevas.

Les anciennes versions Windows d’OBS peuvent utiliser le dossier historique `bin/64bit`. Pour OBS Flatpak, installez le plugin via le paquet Flatpak compatible ou suivez le chemin de plugins documenté pour cette installation. Consultez le [guide officiel des plugins OBS](https://obsproject.com/kb/plugins-guide) pour les dossiers propres à chaque plateforme.

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
