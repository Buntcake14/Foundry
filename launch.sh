#!/usr/bin/env bash
# Launches the Alice launcher against this machine's Victoria 2 install.
set -euo pipefail

V2_DIR="/mnt/3efc8a63-8689-4f23-a294-632f170f151c/SteamLibrary/steamapps/common/Victoria 2"
FOUNDRY_DIR="/home/seth/Documents/Projects/Foundry"

cd "$V2_DIR"
export LD_LIBRARY_PATH="$FOUNDRY_DIR/build/dependencies/luajit/luajit-prefix/lib:${LD_LIBRARY_PATH:-}"
exec "$FOUNDRY_DIR/build/Launcher/launch_alice"
