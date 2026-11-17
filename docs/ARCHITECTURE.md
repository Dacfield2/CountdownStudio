# Architecture

## English

The project is split into three layers:

1. **Core** — a C++20 countdown state machine based on `std::chrono::steady_clock`. It has no OBS, UI, or operating-system dependency.
2. **OBS adapter** — an optional loadable module linked against `libobs`. The current entry point only verifies the module lifecycle; source registration is a later patch.
3. **Interfaces** — the planned native OBS control dock and browser overlay. They will consume the same core behavior and share the JSON settings model.

The settings example and JSON Schema define an initial configuration contract. The plugin does not read this file yet. English and French OBS strings use OBS's standard INI localization mechanism.

The CI matrix builds and tests the platform-independent core on Linux, Windows, and macOS. Building the OBS module is intentionally separate until CI has a pinned OBS SDK for each platform.

## Français

Le projet est organisé en trois couches :

1. **Cœur** — une machine d’états C++20 basée sur `std::chrono::steady_clock`. Elle ne dépend ni d’OBS, ni d’une interface, ni du système d’exploitation.
2. **Adaptateur OBS** — un module chargeable facultatif lié à `libobs`. Le point d’entrée actuel vérifie uniquement le cycle de vie du module ; l’enregistrement d’une source fera l’objet d’un prochain patch.
3. **Interfaces** — le panneau de commande OBS natif et l’incrustation navigateur prévus. Ils utiliseront le même comportement du minuteur et partageront le modèle de paramètres JSON.

L’exemple de paramètres et le schéma JSON définissent un premier contrat de configuration. Le plugin ne lit pas encore ce fichier. Les textes OBS français et anglais utilisent le mécanisme de localisation INI standard d’OBS.

La matrice CI compile et teste le cœur indépendant de la plateforme sous Linux, Windows et macOS. La compilation du module OBS reste séparée tant qu’un SDK OBS déterminé n’est pas configuré pour chaque plateforme.
