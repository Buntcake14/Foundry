#!/usr/bin/env bash
# Launches the game binary directly with the Phase 1 web UI active, bypassing
# the GUI launcher entirely -- see .claude/plans/quiet-puzzling-raven.md.
#
# The launcher (launch.sh) currently has no way to pass -host/-webui through to
# the game (its Singleplayer path never sets host mode at all, and it has no UI
# for host_settings.json's alice_expose_webui flag), so this is the only path
# that doesn't require hand-editing that JSON file.
#
# Usage:
#   ./launch_webui.sh                 # uses the most recently built scenario
#   ./launch_webui.sh SOME_FILE.bin    # uses a specific scenario (bare filename,
#                                       # not a path -- resolved against
#                                       # ~/.local/share/Alice/scenarios/)
#   ./launch_webui.sh -- -headless     # extra args passed straight through
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/sync_assets.sh"

cmake --build "$FOUNDRY_DIR/build" --target AliceIncremental -j2

SCENARIO_DIR="$HOME/.local/share/Alice/scenarios"

scenario=""
if [[ $# -gt 0 && "$1" != "--" ]]; then
	scenario="$1"
	shift
fi
if [[ "${1:-}" == "--" ]]; then
	shift
fi

if [[ -z "$scenario" ]]; then
	latest="$(ls -t "$SCENARIO_DIR"/*.bin 2>/dev/null | head -1 || true)"
	if [[ -z "$latest" ]]; then
		echo "No scenario .bin found in $SCENARIO_DIR -- build one first (see CLAUDE.md)." >&2
		exit 1
	fi
	scenario="$(basename "$latest")"
fi

echo "Using scenario: $scenario"
echo "Web UI will be at http://localhost:1234/ once the sim starts ticking."

cd "$V2_DIR"
export LD_LIBRARY_PATH="$FOUNDRY_DIR/build/dependencies/luajit/luajit-prefix/lib:${LD_LIBRARY_PATH:-}"
exec "$FOUNDRY_DIR/build/AliceIncremental" "$scenario" -host -webui "$@"
