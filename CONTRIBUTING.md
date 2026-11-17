# Contributing

## English

Keep changes small and focused. Add or update deterministic tests when changing countdown behavior. Keep `README.md` bilingual in every patch; update its English and French sections together whenever behavior, build steps, configuration, or user-facing features change. Keep `config/countdown.example.json` and its schema aligned when the configuration model changes.

Before submitting a patch, configure, build, and run CTest using the commands in [Building](docs/BUILDING.md). Do not commit generated build directories or machine-specific OBS SDK paths.

## Français

Gardez les changements ciblés. Ajoutez ou mettez à jour des tests déterministes lorsque le comportement du minuteur change. Conservez `README.md` en français et en anglais dans chaque patch ; mettez à jour les deux sections ensemble si le comportement, la compilation, la configuration ou une fonctionnalité visible change. Maintenez `config/countdown.example.json` et son schéma synchronisés lorsque le modèle de configuration évolue.

Avant de proposer un patch, configurez, compilez et lancez CTest avec les commandes de [Compilation](docs/BUILDING.md). Ne validez pas les répertoires de compilation générés ni les chemins du SDK OBS propres à une machine.
