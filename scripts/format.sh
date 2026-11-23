#!/usr/bin/env bash
# Format (or check) all C++ in the repo with clang-format, using .clang-format.
#   scripts/format.sh          rewrite files in place
#   scripts/format.sh --check  fail if any file would change (used by CI)
set -euo pipefail
cd "$(dirname "$0")/.."

# Only our own code: never JUCE/, modules/ or cmake/ (submodules).
files=$(git ls-files 'source/*' 'tests/*' 'benchmarks/*' | grep -E '\.(cpp|h|hpp|mm)$' || true)

if [[ "${1:-}" == "--check" ]]; then
    echo "$files" | xargs clang-format --dry-run -Werror
    echo "Formatting OK"
else
    echo "$files" | xargs clang-format -i
    echo "Formatted"
fi
