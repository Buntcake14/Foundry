#!/usr/bin/env bash
# Launches the Alice launcher against this machine's Victoria 2 install.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/sync_assets.sh"

# Keep both executables current. The launcher starts AliceIncremental for the
# game itself, so rebuilding only launch_alice can pair new GUI files with stale
# C++ element names and cause null UI-child crashes.
cmake --build "$FOUNDRY_DIR/build" --target AliceIncremental launch_alice -j2

cd "$V2_DIR"
export LD_LIBRARY_PATH="$FOUNDRY_DIR/build/dependencies/luajit/luajit-prefix/lib:${LD_LIBRARY_PATH:-}"
exec "$FOUNDRY_DIR/build/Launcher/launch_alice"
