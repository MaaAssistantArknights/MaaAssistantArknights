#!/usr/bin/env bash

set -euo pipefail

mode="init"
depth=""
declare -a paths=()

while (($# > 0)); do
    case "$1" in
        --init)
            mode="init"
            shift
            ;;
        --remote)
            mode="remote"
            shift
            ;;
        --depth)
            depth="$2"
            shift 2
            ;;
        --)
            shift
            paths+=("$@")
            break
            ;;
        *)
            paths+=("$1")
            shift
            ;;
    esac
done

if ((${#paths[@]} == 0)); then
    echo "Usage: $0 [--init|--remote] [--depth N] path [path ...]" >&2
    exit 64
fi

is_submodule_path() {
    local path="$1"

    git config -f .gitmodules --get-regexp '^submodule\..*\.path$' 2>/dev/null \
        | awk '{ print $2 }' \
        | grep -Fxq "$path"
}

for path in "${paths[@]}"; do
    if ! is_submodule_path "$path"; then
        echo "Skipping $path: not configured as a submodule in .gitmodules."
        continue
    fi

    if [[ "$mode" == "remote" ]]; then
        echo "Updating submodule: $path"
        git submodule update --remote "$path"
        continue
    fi

    if [[ -n "$depth" ]]; then
        echo "Initializing submodule: $path (depth=$depth)"
        git submodule update --init --depth "$depth" "$path"
        continue
    fi

    echo "Initializing submodule: $path"
    git submodule update --init "$path"
done
