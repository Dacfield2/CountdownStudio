# Architecture

## English

The project is split into three layers:

1. **Core** — a C++20 countdown state machine based on `std::chrono::steady_clock`. It has no OBS, UI, or operating-system dependency.
2. **OBS adapter and dock** — a loadable module linked against `libobs`, `obs-frontend-api`, and Qt 6. It registers the transparent video source and a native dock for start, pause, restart, fixed or custom minute additions, and opening the selected source's OBS properties.
3. **Optional browser remote and settings** — `data/web/control.html` uses OBS WebSocket v5 as an optional separate remote panel. It is not needed for normal use. The source properties control colors, size, format, duration, and background. The JSON example/schema document those settings; the plugin does not import the JSON file automatically. English and French labels use OBS's INI localization mechanism.

The source uses a fixed 1280×360 texture so OBS can transform it to match any scene canvas. Qt renders the selected system font into the texture with the configured foreground/background colors and alpha.

The CI matrix builds and tests the platform-independent core on Linux, Windows, and macOS. The OBS module needs a matching OBS frontend SDK and Qt 6 installation; CPack can then create packages for the current platform. The OBS module has not yet been verified as a loaded plugin on Linux or macOS.

## Français

Le projet est organisé en trois couches :

1. **Cœur** — une machine d’états C++20 basée sur `std::chrono::steady_clock`. Elle ne dépend ni d’OBS, ni d’une interface, ni du système d’exploitation.
2. **Adaptateur et panneau OBS** — un module chargeable lié à `libobs`, `obs-frontend-api` et Qt 6. Il enregistre une source vidéo transparente et un dock natif pour démarrer, mettre en pause, réinitialiser, ajouter des minutes fixes ou personnalisées et ouvrir les propriétés OBS de la source sélectionnée.
3. **Télécommande navigateur facultative et paramètres** — `data/web/control.html` utilise OBS WebSocket v5 comme télécommande séparée facultative. Elle n’est pas nécessaire à l’utilisation normale. Les propriétés de la source règlent les couleurs, la taille, le format, la durée et le fond. L’exemple et le schéma JSON documentent ces réglages; le plugin n’importe pas automatiquement le fichier JSON. Les libellés français et anglais utilisent les fichiers INI d’OBS.

La source utilise une texture fixe de 1280×360 qu’OBS peut transformer pour l’adapter à tout canevas de scène. Qt affiche la police système choisie dans cette texture, avec les couleurs, la taille et la transparence configurées.

La matrice CI compile et teste le cœur indépendant de la plateforme sous Linux, Windows et macOS. Le module OBS nécessite un SDK frontend OBS compatible et Qt 6; CPack peut alors créer des paquets pour la plateforme actuelle. Le chargement du module OBS n’a pas encore été validé sous Linux ou macOS.
