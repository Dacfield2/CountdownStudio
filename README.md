# Countdown Studio

A native OBS Studio source for countdown timers and a customizable local clock. Configure and control it directly in OBS—no browser source required.

[Download the latest release](https://github.com/Dacfield2/CountdownStudio/releases/latest) · [Report a problem](https://github.com/Dacfield2/CountdownStudio/issues) · [English](#english) · [Français](#français)

## English

## Features

- Countdown timer with pause, resume, reset, and time-add controls.
- Optional start when OBS begins streaming.
- Key-moment actions and an optional scene change when the timer ends.
- Custom text, system or custom fonts, colors, and animations.
- Clock mode with independent font and color settings, plus an optional customizable frame.
- In-properties preview that updates as you change settings.

## Download and install

Get the package for your operating system and OBS version from [Releases](https://github.com/Dacfield2/CountdownStudio/releases/latest).

### Windows

1. Close OBS.
2. Download and run the installer matching your OBS version: OBS 32 or OBS 33 and newer.
3. Reopen OBS.

### Linux

1. Download the package matching your OBS version: **legacy** for OBS 32 or **modern** for OBS 33 and newer.
2. Extract the ZIP file.
3. Close OBS, open a terminal in the extracted folder, and run `./install-linux.sh`. Do not run it with `sudo`.
4. Reopen OBS.

There is no macOS release package yet.

## Quick start

1. In OBS, choose **Sources → +** and add **Countdown Studio Timer**.
2. Open the source **Properties** to set the duration, appearance, and behavior.
3. Select the source to show its controls beside **Properties** and **Filters**.
4. Start the timer, or enable **Start when streaming starts** in the source properties.

To use the source as a clock, choose **Current local time** under **Display mode**. Select **HH:MM** or **HH:MM:SS**, then adjust the clock font, size, text color, and optional frame. The frame supports rectangle, rounded rectangle, pill, and ellipse shapes, with configurable fill, border, radius, and padding.

## Build and contribute

See [Building Countdown Studio](docs/BUILDING.md) for the development requirements and build steps. Contributions and bug reports are welcome through [GitHub Issues](https://github.com/Dacfield2/CountdownStudio/issues) and [Pull Requests](https://github.com/Dacfield2/CountdownStudio/pulls). Read [CONTRIBUTING.md](CONTRIBUTING.md) before submitting a change.

## License

Countdown Studio is licensed under [GPL-2.0-or-later](LICENSE).

## Français

Countdown Studio est une source native pour OBS Studio qui propose un compte à rebours et une horloge locale personnalisable. Tout se règle et se commande directement dans OBS, sans source navigateur.

[Télécharger la dernière version](https://github.com/Dacfield2/CountdownStudio/releases/latest) · [Signaler un problème](https://github.com/Dacfield2/CountdownStudio/issues) · [English](#english) · [Français](#français)

### Fonctionnalités

- Compte à rebours avec commandes de pause, reprise, réinitialisation et ajout de temps.
- Démarrage facultatif au début du direct OBS.
- Actions aux moments clés et changement de scène facultatif à la fin du décompte.
- Texte, polices système ou personnalisées, couleurs et animations réglables.
- Mode horloge avec police et couleurs indépendantes, ainsi qu’un cadre personnalisable.
- Aperçu dans les propriétés, actualisé lorsque vous modifiez les réglages.

### Télécharger et installer

Téléchargez le paquet correspondant à votre système et à votre version d’OBS depuis les [releases](https://github.com/Dacfield2/CountdownStudio/releases/latest).

#### Windows

1. Fermez OBS.
2. Téléchargez puis lancez l’installateur correspondant à votre version d’OBS : OBS 32 ou OBS 33 et versions ultérieures.
3. Rouvrez OBS.

#### Linux

1. Téléchargez le paquet correspondant à votre version d’OBS : **legacy** pour OBS 32 ou **modern** pour OBS 33 et versions ultérieures.
2. Extrayez le fichier ZIP.
3. Fermez OBS, ouvrez un terminal dans le dossier extrait et lancez `./install-linux.sh`. Ne lancez pas ce script avec `sudo`.
4. Rouvrez OBS.

Aucun paquet macOS n’est disponible pour le moment.

### Premiers pas

1. Dans OBS, choisissez **Sources → +**, puis ajoutez **Minuteur Countdown Studio**.
2. Ouvrez les **Propriétés** de la source pour régler la durée, l’apparence et le fonctionnement.
3. Sélectionnez la source pour afficher ses commandes à côté de **Propriétés** et **Filtres**.
4. Démarrez le minuteur ou cochez **Démarrer au lancement du direct** dans les propriétés.

Pour utiliser la source comme horloge, choisissez **Heure locale actuelle** dans **Mode d’affichage**. Sélectionnez **HH:MM** ou **HH:MM:SS**, puis réglez la police, la taille, la couleur du texte et le cadre facultatif. Le cadre peut être rectangulaire, arrondi, en forme de capsule ou elliptique; sa couleur, son contour, son arrondi et sa marge sont personnalisables.

### Compilation et contribution

Consultez [Compiler Countdown Studio](docs/BUILDING.md) pour les prérequis et les étapes de compilation. Les contributions et signalements de problèmes sont les bienvenus dans les [Issues GitHub](https://github.com/Dacfield2/CountdownStudio/issues) et les [Pull Requests](https://github.com/Dacfield2/CountdownStudio/pulls). Lisez [CONTRIBUTING.md](CONTRIBUTING.md) avant de proposer une modification.

### Licence

Countdown Studio est distribué sous licence [GPL-2.0-or-later](LICENSE).
