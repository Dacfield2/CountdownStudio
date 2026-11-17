# Architecture

## English

The project is split into three layers:

1. **Core** — a C++20 countdown state machine based on `std::chrono::steady_clock`. It has no OBS, UI, or operating-system dependency.
2. **OBS adapter** — an optional loadable module linked against `libobs`. It registers a native video source that renders the timer as a transparent bitmap overlay. The source properties provide duration, auto-start, background, start, pause, and restart controls.
3. **Settings and localization** — a JSON example/schema document the source settings; OBS's INI locale mechanism provides English and French labels. The JSON file is a reference template and is not imported automatically by the plugin.

The source uses a fixed 1280×360 texture so OBS can transform it to match any scene canvas. The texture is generated with a small built-in bitmap font, avoiding platform-specific font dependencies.

The CI matrix builds and tests the platform-independent core on Linux, Windows, and macOS. Building the OBS module is intentionally separate until CI has a pinned OBS SDK for each platform.

## Français

Le projet est organisé en trois couches :

1. **Cœur** — une machine d’états C++20 basée sur `std::chrono::steady_clock`. Elle ne dépend ni d’OBS, ni d’une interface, ni du système d’exploitation.
2. **Adaptateur OBS** — un module chargeable facultatif lié à `libobs`. Il enregistre une source vidéo native qui affiche le minuteur sous forme d’incrustation bitmap transparente. Les propriétés de la source proposent la durée, le démarrage automatique, le fond, le démarrage, la pause et la réinitialisation.
3. **Paramètres et localisation** — un exemple et un schéma JSON documentent les réglages de la source ; le mécanisme INI d’OBS fournit les libellés français et anglais. Le fichier JSON sert de modèle de référence et n’est pas importé automatiquement par le plugin.

La source utilise une texture fixe de 1280×360 qu’OBS peut transformer pour l’adapter à tout canevas de scène. La texture est produite avec une petite police bitmap intégrée, sans dépendance de police propre à une plateforme.

La matrice CI compile et teste le cœur indépendant de la plateforme sous Linux, Windows et macOS. La compilation du module OBS reste séparée tant qu’un SDK OBS déterminé n’est pas configuré pour chaque plateforme.
