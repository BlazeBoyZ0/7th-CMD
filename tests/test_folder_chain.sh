#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 1 ]]; then
    echo "usage: test_folder_chain.sh <7thcmd-binary>" >&2
    exit 2
fi
temporary_home="$(mktemp -d)"
trap 'rm -rf "$temporary_home"' EXIT
mkdir -p "$temporary_home/Desktop"
output="$(
    HOME="$temporary_home" "$1" 2>&1 <<'COMMANDS'
Create a folder named Batch9ChainTest on Desktop, then create a folder named Child inside it
exit
COMMANDS
)"
parent="$temporary_home/Desktop/Batch9ChainTest"
child="$parent/Child"
if [[ ! -d "$parent" || ! -d "$child" ]]; then
    echo "dependent folder creation did not create both expected directories" >&2
    echo "$output" >&2
    exit 1
fi
grep -q "Completed (command)" <<<"$output"
grep -q "$child" <<<"$output"
echo "Dependent folder creation with trailing punctuation passed."
