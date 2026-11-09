#!/usr/bin/env bash
set -euo pipefail

binary="${1:?path to 7thcmd executable is required}"
home="$(mktemp -d)"
trap 'rm -rf "$home"' EXIT

printf '%s\n' 'int main() { return 0; }' > "$home/main.cpp"

output="$(
    printf '%s\n' 'rename file main.cpp' 'test.cpp' 'exit' |
        HOME="$home" "$binary" 2>&1
)"

if [[ ! -f "$home/test.cpp" ]]; then
    printf '%s\n' "$output"
    echo "Expected clarification answer to rename main.cpp to test.cpp." >&2
    exit 1
fi

if [[ -e "$home/main.cpp" ]]; then
    printf '%s\n' "$output"
    echo "The original source still exists after the rename." >&2
    exit 1
fi

if ! grep -Fq "Completed (command)" <<<"$output"; then
    printf '%s\n' "$output"
    echo "The CLI did not report successful completion." >&2
    exit 1
fi

echo "Rename clarification was applied and verified."
