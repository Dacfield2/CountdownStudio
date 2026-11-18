# Countdown Studio

A native countdown timer for OBS Studio. Set it up and control it directly inside OBS.

## English

### Install on Windows

1. Download the installer from [GitHub Releases](https://github.com/Dacfield2/CountdownStudio/releases/latest).
2. Choose the installer for **OBS 32** or **OBS 33 or newer**.
3. Close OBS, run the installer, then reopen OBS.

The repository is private, so you need access to download the installer.

### Use the timer

1. In OBS, add **Countdown Studio Timer** from **Sources → +**.
2. Open the source properties to set the starting time and appearance.
3. Select the source to show its controls beside **Properties** and **Filters**. Start, pause, reset, or add 1 or 5 minutes. Use **+…** to add a custom number of minutes while the timer is running.

If the source controls are not visible, open **Docks → Countdown Studio Controls**.

### Appearance and animation

Set the font, size, colors, transparency, time format, and end message in the source properties. To use a custom font, select a `.ttf`, `.otf`, or `.ttc` file. Keep the file on your computer; the plugin uses it from its current location. Turn on **Text antialiasing** for smoother edges, or turn it off for a sharper pixel look.

Under **Animation**, choose effects for the timer start and text changes. By default, only changed digits animate, while unchanged digits and colons stay still. Turn off **Animate changed digits only** to animate the whole timer. Choose an effect, duration, easing, and digit stagger. The end message animates as a full text.

### Compatibility

Windows x64 installers are available for OBS 32 and OBS 33 or newer. Linux x64 ZIP packages are compiled against OBS 32.2.2 on Ubuntu 24.04 and OBS 33.0.1 on Ubuntu 26.04. Download the ZIP from the **Artifacts** section of a successful [GitHub Actions run](https://github.com/Dacfield2/CountdownStudio/actions/workflows/windows-release.yml). Linux packages target native Ubuntu OBS installations; Flatpak/Snap support and runtime loading still need testing. macOS packages are not currently available.

### Build from source

See [Build instructions](docs/BUILDING.md).

Countdown Studio is licensed under [GPL-2.0-or-later](LICENSE).

## Français

Un minuteur natif pour OBS Studio, à configurer et à contrôler directement dans OBS.

### Installer sous Windows

1. Téléchargez l’installateur depuis les [GitHub Releases](https://github.com/Dacfield2/CountdownStudio/releases/latest).
2. Choisissez celui correspondant à **OBS 32** ou **OBS 33 et versions ultérieures**.
3. Fermez OBS, lancez l’installateur, puis rouvrez OBS.

Le dépôt est privé : vous devez y avoir accès pour télécharger l’installateur.

### Utiliser le minuteur

1. Dans OBS, ajoutez **Minuteur Countdown Studio** avec **Sources → +**.
2. Ouvrez les propriétés de la source pour régler la durée de départ et l’apparence.
3. Sélectionnez la source pour afficher les commandes à côté de **Propriétés** et **Filtres**. Démarrez, mettez en pause, réinitialisez ou ajoutez 1 ou 5 minutes. Utilisez **+…** pour ajouter le nombre de minutes souhaité pendant le décompte.

Si les commandes ne sont pas visibles, ouvrez **Docks → Commandes Countdown Studio**.

### Apparence et animation

Dans les propriétés de la source, réglez la police, la taille, les couleurs, la transparence, le format du temps et le message de fin. Pour utiliser une police personnalisée, choisissez un fichier `.ttf`, `.otf` ou `.ttc`. Gardez ce fichier sur votre ordinateur : le plugin l’utilise depuis son emplacement actuel. Activez **Lissage des contours du texte (anticrénelage)** pour des bords plus doux, ou désactivez-le pour un rendu plus pixelisé.

Dans **Animation**, choisissez les effets au démarrage et lors des changements de texte. Par défaut, seuls les chiffres modifiés s’animent; les autres chiffres et les deux-points restent fixes. Décochez **Animer uniquement les chiffres modifiés** pour animer tout le minuteur. Réglez l’effet, la durée, la courbe d’accélération et le décalage entre les chiffres. Le message de fin s’anime comme un texte complet.

### Compatibilité

Des installateurs Windows x64 sont disponibles pour OBS 32 et OBS 33 ou plus récent. Des ZIP Linux x64 sont compilés avec OBS 32.2.2 sous Ubuntu 24.04 et OBS 33.0.1 sous Ubuntu 26.04. Téléchargez le ZIP dans la section **Artifacts** d’une [exécution réussie de GitHub Actions](https://github.com/Dacfield2/CountdownStudio/actions/workflows/windows-release.yml). Les paquets Linux ciblent OBS installé nativement sous Ubuntu; leur compatibilité Flatpak/Snap et leur chargement restent à vérifier. Les paquets macOS ne sont pas disponibles pour le moment.

### Compiler le projet

Consultez les [instructions de compilation](docs/BUILDING.md).

Countdown Studio est distribué sous licence [GPL-2.0-or-later](LICENSE).