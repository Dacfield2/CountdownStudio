# Countdown Studio

A countdown timer for OBS Studio, controlled directly inside OBS. Change the time, appearance, and font without opening a browser.

## English

### Download and install on Windows

Download the latest version from [GitHub Releases](https://github.com/Dacfield2/CountdownStudio/releases/latest). Choose the installer that matches your OBS version:

- **OBS 32:** `CountdownStudio-0.1.1-OBS32-Windows-x64-Setup.exe`
- **OBS 33 or newer:** `CountdownStudio-0.1.1-OBS33-Windows-x64-Setup.exe`

Then:

1. Close OBS.
2. Run the downloaded installer.
3. For portable OBS, choose the `plugins/CountdownStudio` folder inside your OBS folder.
4. Reopen OBS.

The repository is private, so you need repository access to download the files.

### Add and control a timer

1. In OBS, go to **Sources**, select **+**, then choose **Countdown Studio Timer**.
2. Open the source **Properties** to set the starting time and appearance.
3. Select the timer source in **Sources**. Use the small buttons beside **Properties** and **Filters** to start, pause, restart, or add 1 or 5 minutes. Select **+…** to add a custom number of minutes while the timer is running. If the buttons are unavailable, open **Docks → Countdown Studio Controls**. If **Animation** is also missing from the source properties, the updated plugin did not load; check the OBS log for `[Countdown Studio] countdown source registered` after installing the package that matches your OBS version.

### Change the look or use your own font

Open the timer source **Properties** to change its font, size, colors, transparency, time format, and the message shown when the countdown ends. The color preview and rendered timer use the same OBS color values.

To use your own font, choose a `.ttf`, `.otf`, or `.ttc` file in **Custom font file**. OBS uses that font from its current location on your computer; the plugin does not copy the font into the scene or installer.

### Animate the timer text

In the source **Properties**, open **Animation**. Choose separate effects for when the timer starts and whenever its displayed text changes. By default, only changed digits animate; unchanged digits and colons stay still. Turn off **Animate changed digits only** to animate the whole timer instead. The end message always animates as a full text. Effects include fade, slide, zoom, bounce, pulse, reveal, shake, flip, and roll. Set the duration, easing, and digit stagger there too. All animation is rendered by the native OBS plugin.

### Compatibility

Windows installers are provided for OBS 32 and OBS 33. The plugin has not yet been verified inside OBS on macOS or Linux, so installers for those systems are not available yet.

### Publish a release

Maintainers can publish a version by pushing its matching Git tag (for example, `v0.1.1`). GitHub Actions builds the Windows plugin for OBS 32.2.2 and OBS 33.0.1 on every push to `main`. It publishes a Release only after both builds succeed, when a version tag is pushed or a manual run includes a release tag. If publishing failed or the version tag was deleted, open **Actions → Windows OBS packages → Run workflow**, choose `main`, enter the version tag, and start the workflow again. The workflow rebuilds both packages and creates a missing tag from `main` before publishing. Each Release contains the two Windows installers, their ZIP packages, and SHA-256 checksum files.

### For developers

Build instructions and project details are in [Building](docs/BUILDING.md), [Architecture](docs/ARCHITECTURE.md), and [Release packaging](docs/RELEASE-PACKAGING.md).

Countdown Studio is licensed under [GPL-2.0-or-later](LICENSE).

## Français

Un compte à rebours pour OBS Studio, que vous contrôlez directement dans OBS. Modifiez la durée, l’apparence et la police sans ouvrir de navigateur.

### Télécharger et installer sous Windows

Téléchargez la dernière version depuis les [GitHub Releases](https://github.com/Dacfield2/CountdownStudio/releases/latest). Choisissez l’installateur correspondant à votre version d’OBS :

- **OBS 32 :** `CountdownStudio-0.1.1-OBS32-Windows-x64-Setup.exe`
- **OBS 33 ou plus récent :** `CountdownStudio-0.1.1-OBS33-Windows-x64-Setup.exe`

Puis :

1. Fermez OBS.
2. Lancez l’installateur téléchargé.
3. Pour OBS portable, choisissez le dossier `plugins/CountdownStudio` dans le dossier d’OBS.
4. Rouvrez OBS.

Le dépôt est privé : vous devez y avoir accès pour télécharger les fichiers.

### Ajouter et contrôler un minuteur

1. Dans OBS, ouvrez **Sources**, cliquez sur **+**, puis choisissez **Minuteur Countdown Studio**.
2. Ouvrez les **Propriétés** de la source pour régler la durée de départ et l’apparence.
3. Sélectionnez la source minuteur dans **Sources**. Utilisez les petits boutons à côté de **Propriétés** et **Filtres** pour démarrer, mettre en pause, réinitialiser ou ajouter 1 ou 5 minutes. Cliquez sur **+…** pour ajouter le nombre de minutes souhaité pendant le décompte. Si ces boutons ne sont pas disponibles, ouvrez **Docks → Commandes Countdown Studio**. Si le groupe **Animation** manque aussi dans les propriétés, le plugin mis à jour ne s’est pas chargé; vérifiez dans le journal OBS que `[Countdown Studio] countdown source registered` apparaît après avoir installé le paquet correspondant à votre version d’OBS.

### Modifier l’apparence ou utiliser votre police

Dans les **Propriétés** de la source, vous pouvez modifier la police, la taille, les couleurs, la transparence, le format du temps et le message affiché à la fin du décompte. L’aperçu du sélecteur de couleur correspond au rendu du minuteur.

Pour utiliser votre police, choisissez un fichier `.ttf`, `.otf` ou `.ttc` dans **Fichier de police personnalisé**. OBS utilise le fichier à son emplacement actuel sur votre ordinateur; le plugin ne copie pas la police dans la scène ou l’installateur.

### Animer le texte du minuteur

Dans les **Propriétés** de la source, ouvrez **Animation**. Choisissez un effet pour le démarrage du minuteur et un autre pour chaque changement. Par défaut, seuls les chiffres modifiés s’animent; les autres chiffres et les deux-points restent fixes. Décochez **Animer uniquement les chiffres modifiés** pour animer tout le minuteur. Le message final s’anime toujours comme un texte complet. Effets disponibles : fondu, glissement, zoom, rebond, pulsation, révélation, secousse, retournement et défilement. Réglez aussi la durée, la courbe d’accélération et le décalage entre les chiffres. Toute l’animation est rendue par le plugin OBS natif.

### Compatibilité

Des installateurs Windows sont fournis pour OBS 32 et OBS 33. Le plugin n’a pas encore été vérifié dans OBS sous macOS ou Linux; les installateurs pour ces systèmes ne sont donc pas disponibles.

### Publier une version

Les responsables du projet peuvent publier une version en poussant son étiquette Git correspondante (par exemple, `v0.1.1`). GitHub Actions compile le plugin Windows pour OBS 32.2.2 et OBS 33.0.1 à chaque push sur `main`. Il publie une Release seulement après la réussite des deux compilations, lors d’un push d’étiquette de version ou d’une exécution manuelle avec une étiquette saisie. Si la publication a échoué ou si l’étiquette de version a été supprimée, ouvrez **Actions → Windows OBS packages → Run workflow**, choisissez `main`, saisissez l’étiquette de version, puis relancez le workflow. Il recompile les deux paquets et recrée l’étiquette manquante depuis `main` avant la publication. Chaque Release contient les deux installateurs Windows, leurs archives ZIP et les fichiers de sommes SHA-256.

### Pour les développeurs

Les instructions de compilation et les détails du projet se trouvent dans [Compilation](docs/BUILDING.md), [Architecture](docs/ARCHITECTURE.md) et [Préparation des paquets](docs/RELEASE-PACKAGING.md).

Countdown Studio est distribué sous licence [GPL-2.0-or-later](LICENSE).
