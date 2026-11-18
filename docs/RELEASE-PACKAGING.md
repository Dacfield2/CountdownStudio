# Release packaging plan

## English

The repository remains private; GitHub Release assets are available only to people who can read it. The current release workflow prepares Windows x64 packages for the two OBS families below. Ubuntu and macOS plugin builds remain unverified.

### Build and package tools

- Build the Windows C++20 plugin in GitHub Actions with CMake and Visual Studio 2026 on the Windows Server 2025 image. The workflow downloads pinned OBS sources and development dependencies, verifies their SHA-256 hashes, then builds the OBS SDK and plugin.
- Install NSIS 3.12 on the runner, then generate an NSIS installer and ZIP for each Windows OBS family. Ubuntu and macOS packages remain future work until the OBS module builds and loads successfully on those systems.
- Do not ship a compiler to users. Actions compiles the plugin; users download the ready-to-install package.

### Current Windows targets and remaining platforms

| OBS family | Windows x64 | Ubuntu x86-64 | macOS |
| --- | --- | --- | --- |
| OBS 32 | Workflow target: OBS 32.2.2, legacy layout, NSIS `.exe` and ZIP | Not packaged; plugin build/load verification remains | Not packaged; plugin build/load verification remains |
| OBS 33+ | Workflow target: OBS 33.0.1, modern layout, NSIS `.exe` and ZIP | Not packaged; plugin build/load verification remains | Not packaged; plugin build/load verification remains |

The version numbers above are the SDK baselines used by the Windows workflow; compatibility with other OBS releases in those families still needs direct validation. For macOS, publish separate Intel and Apple Silicon installers unless a universal binary is built and verified. The `.pkg` should install into the current user's OBS plugin directory and include an uninstall path. Keep the ZIP as a fallback. For Ubuntu, verify the package's target path and install/remove behavior against the OBS loader before publishing the `.deb`; retain the tarball for compatible non-Ubuntu setups.

### GitHub Releases

`.github/workflows/windows-release.yml` builds both Windows packages on every push to `main` and when run manually. A matching version tag builds them and publishes a GitHub Release after both builds succeed. The PowerShell build script verifies pinned dependency hashes, builds the OBS development libraries and plugin, then checks that the ZIP contains the correct plugin layout, locale files, and license. The workflow publishes:

- `CountdownStudio-<version>-OBS32-Windows-x64-Setup.exe`
- `CountdownStudio-<version>-OBS32-Windows-x64.zip`
- `CountdownStudio-<version>-OBS33-Windows-x64-Setup.exe`
- `CountdownStudio-<version>-OBS33-Windows-x64.zip`
- `SHA256SUMS-OBS32.txt` and `SHA256SUMS-OBS33.txt`

Every archive includes the GPL-2.0-or-later license and matching project sources. The workflow confirms compilation and archive layout; it does not launch OBS for a runtime load test. Ubuntu/macOS plugin stability is not established until those builds and load checks exist. The general CI workflow tests only the platform-independent core.

### Public access

Keeping this repository private also keeps its releases restricted to people with repository access. When public downloads are wanted, either change the repository visibility or publish a public source-and-release mirror. No visibility change is part of this plan.

## Français

Le dépôt reste privé; seuls les utilisateurs autorisés à le lire peuvent accéder à ses fichiers de Release GitHub. Le workflow actuel prépare des paquets Windows x64 pour les deux familles OBS ci-dessous. La compilation du plugin sous Ubuntu et macOS n’a pas encore été vérifiée.

### Outils de compilation et de paquetage

- Compiler le plugin C++20 sous Windows dans GitHub Actions avec CMake et Visual Studio 2026 sur l’image Windows Server 2025. Le workflow télécharge les sources OBS et dépendances épinglées, vérifie leurs sommes SHA-256, puis compile le SDK OBS et le plugin.
- Installer NSIS 3.12 sur l’agent, puis créer un installateur NSIS et un ZIP pour chaque famille OBS Windows. Les paquets Ubuntu et macOS restent à faire tant que le module OBS n’a pas été compilé et chargé correctement sur ces systèmes.
- Ne pas distribuer de compilateur aux utilisateurs. Actions compile le plugin; les utilisateurs téléchargent directement le paquet prêt à installer.

### Cibles Windows actuelles et autres systèmes à traiter

| Famille OBS | Windows x64 | Ubuntu x86-64 | macOS |
| --- | --- | --- | --- |
| OBS 32 | Cible du workflow : OBS 32.2.2, format historique, NSIS `.exe` et ZIP | Aucun paquet; compilation/chargement du plugin à vérifier | Aucun paquet; compilation/chargement du plugin à vérifier |
| OBS 33+ | Cible du workflow : OBS 33.0.1, nouveau format, NSIS `.exe` et ZIP | Aucun paquet; compilation/chargement du plugin à vérifier | Aucun paquet; compilation/chargement du plugin à vérifier |

Les versions indiquées sont les bases SDK utilisées par le workflow Windows; la compatibilité avec les autres versions OBS de ces familles reste à vérifier directement. Pour macOS, publier des installateurs Intel et Apple Silicon distincts, sauf si un binaire universel est compilé et vérifié. Le `.pkg` devrait installer le plugin dans le dossier OBS de l’utilisateur courant et inclure une procédure de désinstallation. Conserver le ZIP en solution de secours. Sous Ubuntu, vérifier l’emplacement du paquet et son installation/désinstallation avec le chargeur OBS avant de publier le `.deb`; garder l’archive pour les environnements compatibles hors Ubuntu.

### GitHub Releases

`.github/workflows/windows-release.yml` compile les deux paquets Windows à chaque poussée sur `main` et lors d’un lancement manuel. Une étiquette de version correspondante les compile puis publie une Release GitHub lorsque les deux compilations réussissent. Le script PowerShell vérifie les sommes des dépendances épinglées, compile les bibliothèques de développement OBS et le plugin, puis contrôle que le ZIP contient le bon format de plugin, les fichiers de langue et la licence. Le workflow publie :

- `CountdownStudio-<version>-OBS32-Windows-x64-Setup.exe`
- `CountdownStudio-<version>-OBS32-Windows-x64.zip`
- `CountdownStudio-<version>-OBS33-Windows-x64-Setup.exe`
- `CountdownStudio-<version>-OBS33-Windows-x64.zip`
- `SHA256SUMS-OBS32.txt` et `SHA256SUMS-OBS33.txt`

Chaque archive contient la licence GPL-2.0-or-later et les sources correspondantes du projet. Le workflow vérifie la compilation et le contenu du ZIP; il ne lance pas OBS pour tester le chargement à l’exécution. La stabilité du plugin sous Ubuntu/macOS n’est pas établie avant que leurs compilations et vérifications de chargement soient disponibles. La CI générale teste uniquement le cœur indépendant de la plateforme.

### Accès public

Garder ce dépôt privé réserve aussi ses Releases aux personnes qui y ont accès. Pour proposer des téléchargements publics, il faudra rendre le dépôt public ou publier un miroir public des sources et des paquets. Ce plan ne modifie pas la visibilité du dépôt.
