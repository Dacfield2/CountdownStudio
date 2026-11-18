# Architecture

## English

The project is split into two layers:

1. **Core** — a C++20 countdown state machine based on `std::chrono::steady_clock`. It has no OBS, UI, or operating-system dependency.
2. **OBS module, dock, and settings** — a loadable module linked against `libobs`, `obs-frontend-api`, and Qt 6. It registers the transparent video source, a native controls dock, and compact controls inserted beside OBS's built-in source Properties and Filters buttons. The compact controls act on the single selected Countdown Studio timer and support start, pause, restart, fixed or custom minute additions. Source properties control colors, size, format, duration, background, and the message shown after completion. The dock remains available as a fallback if the OBS source toolbar cannot be found. The JSON example/schema document those settings; the plugin does not import the JSON file automatically. English and French labels use OBS's INI localization mechanism.

The source uses a fixed 1280×360 texture so OBS can transform it to match any scene canvas. Qt renders the selected system font into the texture with the configured foreground/background colors and alpha. Native text transitions use OBS's per-frame source tick to advance elapsed time and render old/new text layers with the selected preset and easing. Start and text-change effects can be configured independently; rendering stops at the end of each transition.

The CI matrix builds and tests the platform-independent core on Linux, Windows, and macOS. The OBS module needs a matching OBS frontend SDK and Qt 6 installation; CPack can then create packages for the current platform. The OBS module has not yet been verified as a loaded plugin on Linux or macOS.

## Français

Le projet est organisé en deux couches :

1. **Cœur** — une machine d’états C++20 basée sur `std::chrono::steady_clock`. Elle ne dépend ni d’OBS, ni d’une interface, ni du système d’exploitation.
2. **Module OBS, dock et réglages** — un module chargeable lié à `libobs`, `obs-frontend-api` et Qt 6. Il enregistre une source vidéo transparente, un dock natif de commandes et des commandes compactes placées à côté des boutons Propriétés et Filtres intégrés à OBS. Ces commandes agissent sur l’unique source minuteur Countdown Studio sélectionnée et permettent de démarrer, mettre en pause, réinitialiser et ajouter des minutes fixes ou personnalisées. Les propriétés de la source règlent les couleurs, la taille, le format, la durée, le fond et le message de fin. Le dock reste disponible si la barre de source OBS est introuvable. L’exemple et le schéma JSON documentent ces réglages; le plugin n’importe pas automatiquement le fichier JSON. Les libellés français et anglais utilisent les fichiers INI d’OBS.

La source utilise une texture fixe de 1280×360 qu’OBS peut transformer pour l’adapter à tout canevas de scène. Qt affiche la police système choisie dans cette texture, avec les couleurs, la taille et la transparence configurées. Les transitions natives du texte utilisent la mise à jour par image d’OBS pour faire évoluer le temps écoulé et afficher l’ancien et le nouveau texte selon l’effet et la courbe choisis. Les animations du démarrage et des changements de texte se règlent séparément; le rendu s’arrête à la fin de chaque transition.

La matrice CI compile et teste le cœur indépendant de la plateforme sous Linux, Windows et macOS. Le module OBS nécessite un SDK frontend OBS compatible et Qt 6; CPack peut alors créer des paquets pour la plateforme actuelle. Le chargement du module OBS n’a pas encore été validé sous Linux ou macOS.
