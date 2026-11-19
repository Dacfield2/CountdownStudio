#!/usr/bin/env bash
set -euo pipefail

plugin_name="CountdownStudio"
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
language="${LANG:-en}"

say() {
    printf '%s\n' "$1"
}

fail() {
    say "$1" >&2
    exit 1
}

if [[ ${EUID:-$(id -u)} -eq 0 ]]; then
    if [[ "$language" == fr* ]]; then
        fail "Ne lancez pas cet installateur avec sudo. Il installe le plugin dans votre dossier utilisateur."
    else
        fail "Do not run this installer with sudo. It installs the plugin in your user folder."
    fi
fi

if command -v pgrep >/dev/null 2>&1 && pgrep -x obs >/dev/null 2>&1; then
    if [[ "$language" == fr* ]]; then
        fail "Fermez complètement OBS, puis relancez cet installateur."
    else
        fail "Close OBS completely, then run this installer again."
    fi
fi

if [[ -f "$script_dir/CountdownStudio.so" && -d "$script_dir/data" ]]; then
    layout="modern"
    module_file="$script_dir/CountdownStudio.so"
    plugin_dir="${XDG_DATA_HOME:-$HOME/.local/share}/obs-studio/plugins/$plugin_name"
elif [[ -f "$script_dir/bin/64bit/CountdownStudio.so" && -d "$script_dir/data" ]]; then
    layout="legacy"
    module_file="$script_dir/bin/64bit/CountdownStudio.so"
    plugin_dir="${XDG_CONFIG_HOME:-$HOME/.config}/obs-studio/plugins/$plugin_name"
else
    if [[ "$language" == fr* ]]; then
        fail "Fichiers du plugin introuvables. Extrayez entièrement le ZIP et lancez install-linux.sh depuis le dossier extrait."
    else
        fail "Plugin files not found. Extract the ZIP fully and run install-linux.sh from the extracted folder."
    fi
fi

if [[ "$layout" == "legacy" ]]; then
    mkdir -p "$plugin_dir/bin/64bit"
    cp -- "$module_file" "$plugin_dir/bin/64bit/CountdownStudio.so"
else
    mkdir -p "$plugin_dir"
    cp -- "$module_file" "$plugin_dir/CountdownStudio.so"
fi

mkdir -p "$plugin_dir/data"
cp -R -- "$script_dir/data/." "$plugin_dir/data/"

if [[ "$language" == fr* ]]; then
    say "Countdown Studio est installé dans :"
else
    say "Countdown Studio was installed to:"
fi
say "$plugin_dir"
if [[ "$language" == fr* ]]; then
    say "Rouvrez OBS pour utiliser la source Countdown Studio."
else
    say "Reopen OBS to use the Countdown Studio source."
fi
