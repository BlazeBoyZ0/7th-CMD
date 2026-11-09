#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: test_cli_controls.sh <7thcmd-binary>" >&2
    exit 2
fi

output="$(
    printf '%s\n' '--help' '--h' '--version' '--v' 'exit' | "$1" 2>&1
)"

grep -q "CAPABILITIES" <<<"$output"
grep -q "Creating folders" <<<"$output"
grep -q "inspect task <task-id>" <<<"$output"
grep -q "show last task" <<<"$output"
grep -q "Execution history: session-only, latest 100 records" <<<"$output"
grep -q "Type --help at the prompt" <<<"$output"

version_count="$(grep -c '^7th cmd 0.1.0$' <<<"$output" || true)"
if [[ "$version_count" -ne 2 ]]; then
    echo "expected both --version and --v to print version information" >&2
    exit 1
fi

echo "Interactive help and version controls passed."
