#!/usr/bin/env bash
# Launches the Alice launcher against this machine's Victoria 2 install.
set -euo pipefail

V2_DIR="/mnt/3efc8a63-8689-4f23-a294-632f170f151c/SteamLibrary/steamapps/common/Victoria 2"
FOUNDRY_DIR="/home/seth/Documents/Projects/Foundry"

# Project Alice loads these GUI definitions from the Victoria 2 working
# directory, not from the build tree. Synchronize only the files maintained by
# this UI restoration work; other runtime assets may intentionally differ.
cmake -E copy_if_different "$FOUNDRY_DIR/assets/alice.gui" "$V2_DIR/assets/alice.gui"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/alice_menubar.gui" "$V2_DIR/assets/alice_menubar.gui"

# Foundry is the base game, not a Victoria 2 mod. Keep Foundry-owned Production
# UI definitions and graphics in the base runtime so they load with no mod
# selected. These currently live beside the imported Rise of Nations data and
# can be moved into a dedicated Foundry runtime tree as that migration proceeds.
cmake -E copy_if_different "$FOUNDRY_DIR/mod/Rise of Nations/interface/country_production.gui" "$V2_DIR/interface/country_production.gui"
cmake -E copy_if_different "$FOUNDRY_DIR/mod/Rise of Nations/interface/country_production.gfx" "$V2_DIR/interface/country_production.gfx"
cmake -E copy_if_different "$FOUNDRY_DIR/mod/Rise of Nations/gfx/interface/foundry_project_icons.png" "$V2_DIR/gfx/interface/foundry_project_icons.png"

cd "$V2_DIR"
export LD_LIBRARY_PATH="$FOUNDRY_DIR/build/dependencies/luajit/luajit-prefix/lib:${LD_LIBRARY_PATH:-}"
exec "$FOUNDRY_DIR/build/Launcher/launch_alice"
