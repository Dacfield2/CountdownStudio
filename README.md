# Countdown Studio

A countdown timer for OBS Studio, controlled directly inside OBS. Change the time, appearance, and font without opening a browser.

## English

### Download and install on Windows

Download the latest version from [GitHub Releases](https://github.com/Dacfield2/CountdownStudio/releases/latest). Choose the installer that matches your OBS version:

- **OBS 32:** `CountdownStudio-0.1.0-OBS32-Windows-x64-Setup.exe`
- **OBS 33 or newer:** `CountdownStudio-0.1.0-OBS33-Windows-x64-Setup.exe`

Then:

1. Close OBS.
2. Run the downloaded installer.
3. For portable OBS, choose the `plugins/CountdownStudio` folder inside your OBS folder.
4. Reopen OBS.

The repository is private, so you need repository access to download the files.

### Add and control a timer

1. In OBS, go to **Sources**, select **+**, then choose **Countdown Studio Timer**.
2. Open the source **Properties** to set the starting time and appearance.
3. Open **Docks → Countdown Studio Controls** to start, pause, or restart the timer. You can also add 1 minute, 5 minutes, or a custom number of minutes while it is running.

### Change the look or use your own font

Open the timer source **Properties** to change its font, size, colors, transparency, and time format.

To use your own font, choose a `.ttf`, `.otf`, or `.ttc` file in **Custom font file**. OBS uses that font from its current location on your computer; the plugin does not copy the font into the scene or installer.

### Compatibility

Windows installers are provided for OBS 32 and OBS 33. The plugin has not yet been verified inside OBS on macOS or Linux, so installers for those systems are not available yet.

### Publish a release

Maintainers can publish a version by pushing its matching Git tag (for example, `v0.1.0`). GitHub Actions builds the Windows packages and creates the Release when both builds succeed. If the tag was already pushed but publishing failed, open **Actions → Windows OBS packages → Run workflow**, choose `main`, enter the existing tag, and start the workflow again.

### For developers

Build instructions and project details are in [Building](docs/BUILDING.md), [Architecture](docs/ARCHITECTURE.md), and [Release packaging](docs/RELEASE-PACKAGING.md).

Countdown Studio is licensed under [GPL-2.0-or-later](LICENSE).

## Français

Un compte à rebours pour OBS Studio, que vous contrôlez directement dans OBS. Modifiez la durée, l’apparence et la police sans ouvrir de navigateur.

### Télécharger et installer sous Windows

Téléchargez la dernière version depuis les [GitHub Releases](https://github.com/Dacfield2/CountdownStudio/releases/latest). Choisissez l’installateur correspondant à votre version d’OBS :

- **OBS 32 :** `CountdownStudio-0.1.0-OBS32-Windows-x64-Setup.exe`
- **OBS 33 ou plus récent :** `CountdownStudio-0.1.0-OBS33-Windows-x64-Setup.exe`

Puis :

1. Fermez OBS.
2. Lancez l’installateur téléchargé.
3. Pour OBS portable, choisissez le dossier `plugins/CountdownStudio` dans le dossier d’OBS.
4. Rouvrez OBS.

Le dépôt est privé : vous devez y avoir accès pour télécharger les fichiers.

### Ajouter et contrôler un minuteur

1. Dans OBS, ouvrez **Sources**, cliquez sur **+**, puis choisissez **Minuteur Countdown Studio**.
2. Ouvrez les **Propriétés** de la source pour régler la durée de départ et l’apparence.
3. Ouvrez **Docks → Commandes Countdown Studio** pour démarrer, mettre en pause ou réinitialiser le minuteur. Vous pouvez aussi ajouter 1 minute, 5 minutes ou le nombre de minutes de votre choix pendant le décompte.

### Modifier l’apparence ou utiliser votre police

Dans les **Propriétés** de la source, vous pouvez modifier la police, la taille, les couleurs, la transparence et le format du temps.

Pour utiliser votre police, choisissez un fichier `.ttf`, `.otf` ou `.ttc` dans **Fichier de police personnalisé**. OBS utilise le fichier à son emplacement actuel sur votre ordinateur; le plugin ne copie pas la police dans la scène ou l’installateur.

### Compatibilité

Des installateurs Windows sont fournis pour OBS 32 et OBS 33. Le plugin n’a pas encore été vérifié dans OBS sous macOS ou Linux; les installateurs pour ces systèmes ne sont donc pas disponibles.

### Publier une version

Les responsables du projet peuvent publier une version en poussant son étiquette Git correspondante (par exemple, `v0.1.0`). GitHub Actions compile les paquets Windows et crée la Release lorsque les deux compilations réussissent. Si l’étiquette a déjà été poussée mais que la publication a échoué, ouvrez **Actions → Windows OBS packages → Run workflow**, choisissez `main`, saisissez l’étiquette existante, puis relancez le workflow.

### Pour les développeurs

Les instructions de compilation et les détails du projet se trouvent dans [Compilation](docs/BUILDING.md), [Architecture](docs/ARCHITECTURE.md) et [Préparation des paquets](docs/RELEASE-PACKAGING.md).

Countdown Studio est distribué sous licence [GPL-2.0-or-later](LICENSE).
