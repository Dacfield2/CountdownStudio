# Countdown Studio

A native OBS countdown timer with its controls and settings inside OBS.

## English

### Install

- **Windows:** Download the OBS 32 or OBS 33+ installer from [Releases](https://github.com/Dacfield2/CountdownStudio/releases/latest), close OBS, run the installer, and reopen OBS. To update, run it over the existing installation; it no longer calls the older uninstaller.
- **Linux:** Download the OBS 32 **legacy** ZIP or OBS 33+ **modern** ZIP, extract it, close OBS, and run `install-linux.sh`. Then reopen OBS.

### Use

1. In OBS, add **Countdown Studio Timer** from **Sources → +**.
2. Open **Properties** to set the duration, appearance, end message, and automation.
3. Select the source to show its buttons beside **Properties** and **Filters**. You can start, pause, reset, or add 1, 5, or a custom number of minutes.
4. Enable **Start when streaming starts** if the timer should begin with your live. You can choose to reset it first.
5. Enable up to three **Key moments** to show a message, pause, add one minute, or switch scene when a chosen number of seconds remains.
6. Optionally choose a scene to switch to after the timer ends, with a delay.
7. Set **Display mode** to **Current local time** to use the source as a clock. Choose **HH:MM** or **HH:MM:SS**.

### Appearance and animation

Set the countdown font, size, colors, background, time format, and end message in the source properties. The clock has separate controls for its font, optional custom font file, size, and text color. Its optional frame can use a rectangle, rounded rectangle, pill, or ellipse, with configurable fill and border colors, corner radius, padding, and border width. The in-properties preview updates as you change these settings. Enable anti-aliasing for smoother text edges.

Choose separate animations for starting and changing text. The **Custom** animation lets you combine a starting scale, horizontal and vertical movement, rotation, fade, easing, duration, and digit stagger. Animate only changed digits or the whole timer.

### Compatibility and build

Windows x64 packages target OBS 32 and OBS 33+. Linux x64 packages are provided for OBS 32 (legacy) and OBS 33+ (modern). Linux loading has been confirmed on Ubuntu 26.04 with OBS 33; OBS 32 Linux and Flatpak/Snap installations still need validation. macOS packages are not available yet. SHA-256 checksum files remain available in successful GitHub Actions build artifacts, but are not attached to releases.

The source properties include a live preview of the selected display format and appearance. The **About** footer shows **Haribou_** and the package version. Tagged releases derive that version directly from the release tag, so the footer and package filename stay in sync.

See [build instructions](docs/BUILDING.md). Countdown Studio is licensed under [GPL-2.0-or-later](LICENSE). The repository is private, so access is required to download packages.

## Français

Un minuteur natif pour OBS, avec ses commandes et ses réglages directement dans OBS.

### Installation

- **Windows :** téléchargez l’installateur OBS 32 ou OBS 33+ depuis les [releases](https://github.com/Dacfield2/CountdownStudio/releases/latest), fermez OBS, lancez l’installateur, puis rouvrez OBS. Pour mettre à jour, lancez-le sur l’installation existante; il n’appelle plus l’ancien désinstallateur.
- **Linux :** téléchargez le ZIP **legacy** pour OBS 32 ou **modern** pour OBS 33+, extrayez-le, fermez OBS, puis lancez `install-linux.sh`. Rouvrez OBS ensuite.

### Utilisation

1. Dans OBS, ajoutez **Minuteur Countdown Studio** avec **Sources → +**.
2. Ouvrez **Propriétés** pour régler la durée, l’apparence, le message de fin et les automatismes.
3. Sélectionnez la source pour afficher ses boutons à côté de **Propriétés** et **Filtres**. Vous pouvez démarrer, mettre en pause, réinitialiser ou ajouter 1, 5 ou un nombre personnalisé de minutes.
4. Cochez **Démarrer au lancement du direct** pour lancer le minuteur avec le live. Vous pouvez aussi le réinitialiser avant le démarrage.
5. Activez jusqu’à trois **Actions aux moments clés** : afficher un message, mettre en pause, ajouter une minute ou changer de scène lorsque le nombre de secondes choisi reste.
6. Vous pouvez choisir une scène à afficher après la fin du décompte et régler un délai.
7. Choisissez **Heure locale actuelle** dans **Mode d’affichage** pour utiliser la source comme horloge, puis sélectionnez le format **HH:MM** ou **HH:MM:SS**.

### Apparence et animation

Réglez séparément la police, la taille et la couleur du compte à rebours et de l’horloge. L’horloge peut utiliser une police système ou un fichier personnalisé `.ttf`, `.otf` ou `.ttc`. Vous pouvez activer un cadre et choisir son remplissage, son contour, sa forme (rectangle, rectangle arrondi, capsule ou ellipse), l’arrondi des coins et la marge autour de l’heure. L’aperçu des propriétés se met à jour quand vous modifiez ces réglages. Activez l’anticrénelage pour lisser les contours.

Choisissez une animation pour le démarrage et une autre pour les changements de texte. L’animation **Personnalisée** combine taille de départ, déplacement horizontal et vertical, rotation, fondu, accélération, durée et décalage des chiffres. Animez seulement les chiffres modifiés ou le minuteur entier.

### Compatibilité et compilation

Des paquets Windows x64 sont prévus pour OBS 32 et OBS 33+. Les paquets Linux x64 existent pour OBS 32 (**legacy**) et OBS 33+ (**modern**). Le chargement Linux a été confirmé sous Ubuntu 26.04 avec OBS 33; OBS 32 Linux et Flatpak/Snap restent à vérifier. Le paquet macOS n’est pas encore disponible. Les fichiers SHA-256 restent dans les artefacts de compilation GitHub Actions, mais ne sont pas joints aux releases.

Les propriétés de la source incluent un aperçu actualisé du format et de l’apparence sélectionnés. Le pied de page **À propos** affiche **Haribou_** et la version du paquet. Pour les releases étiquetées, la version vient directement du tag, ce qui garde le pied de page et le nom du paquet synchronisés.

Consultez les [instructions de compilation](docs/BUILDING.md). Countdown Studio est sous licence [GPL-2.0-or-later](LICENSE). Le dépôt est privé : il faut y avoir accès pour télécharger les paquets.
