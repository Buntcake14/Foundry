#!/usr/bin/env bash
# Launches the Alice launcher against this machine's Victoria 2 install.
set -euo pipefail

V2_DIR="/mnt/3efc8a63-8689-4f23-a294-632f170f151c/SteamLibrary/steamapps/common/Victoria 2"
FOUNDRY_DIR="/home/seth/Documents/Projects/Foundry"

# Keep both executables current. The launcher starts AliceIncremental for the
# game itself, so rebuilding only launch_alice can pair new GUI files with stale
# C++ element names and cause null UI-child crashes.
cmake --build "$FOUNDRY_DIR/build" --target AliceIncremental launch_alice -j2

# Project Alice loads these GUI definitions from the Victoria 2 working
# directory, not from the build tree. Synchronize only the files maintained by
# this UI restoration work; other runtime assets may intentionally differ.
cmake -E copy_if_different "$FOUNDRY_DIR/assets/alice.gui" "$V2_DIR/assets/alice.gui"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/alice.gfx" "$V2_DIR/assets/alice.gfx"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/alice_menubar.gui" "$V2_DIR/assets/alice_menubar.gui"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_toolkit.gui" "$V2_DIR/assets/foundry_toolkit.gui"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_toolkit.gfx" "$V2_DIR/assets/foundry_toolkit.gfx"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_project_icons.png" "$V2_DIR/assets/foundry_project_icons.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_urban_density.png" "$V2_DIR/assets/foundry_urban_density.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_urban_center_bg.png" "$V2_DIR/assets/foundry_urban_center_bg.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_urban_density_inset.png" "$V2_DIR/assets/foundry_urban_density_inset.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_urban_density_fitted.png" "$V2_DIR/assets/foundry_urban_density_fitted.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_urban_center_bg_v2.png" "$V2_DIR/assets/foundry_urban_center_bg_v2.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_urban_center_bg_v3.png" "$V2_DIR/assets/foundry_urban_center_bg_v3.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_urban_center_bg_v4.png" "$V2_DIR/assets/foundry_urban_center_bg_v4.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_urban_center_bg_v5.png" "$V2_DIR/assets/foundry_urban_center_bg_v5.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_urban_center_bg_v6.png" "$V2_DIR/assets/foundry_urban_center_bg_v6.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_urban_center_bg_v7.png" "$V2_DIR/assets/foundry_urban_center_bg_v7.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_urban_subheader.png" "$V2_DIR/assets/foundry_urban_subheader.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_urban_subheader_wide.png" "$V2_DIR/assets/foundry_urban_subheader_wide.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_urban_stats_box.png" "$V2_DIR/assets/foundry_urban_stats_box.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_urban_stats_box_v2.png" "$V2_DIR/assets/foundry_urban_stats_box_v2.png"
cmake -E make_directory "$V2_DIR/assets/foundry_ui/buttons"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/buttons/foundry_primary_button_200x48_v1.png" "$V2_DIR/assets/foundry_ui/buttons/foundry_primary_button_200x48_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/buttons/foundry_primary_button_144x30_v1.png" "$V2_DIR/assets/foundry_ui/buttons/foundry_primary_button_144x30_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/buttons/foundry_close_button_24x24_v1.png" "$V2_DIR/assets/foundry_ui/buttons/foundry_close_button_24x24_v1.png"
cmake -E make_directory "$V2_DIR/assets/foundry_ui/tabs"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/tabs/foundry_subtabs_120x30_v1.png" "$V2_DIR/assets/foundry_ui/tabs/foundry_subtabs_120x30_v1.png"
cmake -E make_directory "$V2_DIR/assets/foundry_ui/frames"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/frames/foundry_inset_panel_250x66_v1.png" "$V2_DIR/assets/foundry_ui/frames/foundry_inset_panel_250x66_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/frames/foundry_section_header_360x28_v1.png" "$V2_DIR/assets/foundry_ui/frames/foundry_section_header_360x28_v1.png"
cmake -E make_directory "$V2_DIR/assets/foundry_ui/icons"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/icons/foundry_status_indicators_24x24_v1.png" "$V2_DIR/assets/foundry_ui/icons/foundry_status_indicators_24x24_v1.png"
cmake -E make_directory "$V2_DIR/assets/foundry_ui/controls"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/controls/foundry_progress_fill_240x14_v1.png" "$V2_DIR/assets/foundry_ui/controls/foundry_progress_fill_240x14_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/controls/foundry_progress_empty_240x14_v1.png" "$V2_DIR/assets/foundry_ui/controls/foundry_progress_empty_240x14_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/controls/foundry_dropdown_220x28_v1.png" "$V2_DIR/assets/foundry_ui/controls/foundry_dropdown_220x28_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/controls/foundry_dropdown_option_220x24_v1.png" "$V2_DIR/assets/foundry_ui/controls/foundry_dropdown_option_220x24_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/controls/foundry_dropdown_popup_224x76_v1.png" "$V2_DIR/assets/foundry_ui/controls/foundry_dropdown_popup_224x76_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/controls/foundry_slider_track_220x20_v1.png" "$V2_DIR/assets/foundry_ui/controls/foundry_slider_track_220x20_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/controls/foundry_slider_knob_20x20_v1.png" "$V2_DIR/assets/foundry_ui/controls/foundry_slider_knob_20x20_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/controls/foundry_slider_left_20x20_v1.png" "$V2_DIR/assets/foundry_ui/controls/foundry_slider_left_20x20_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/controls/foundry_slider_right_20x20_v1.png" "$V2_DIR/assets/foundry_ui/controls/foundry_slider_right_20x20_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/controls/foundry_checkbox_22x22_v1.png" "$V2_DIR/assets/foundry_ui/controls/foundry_checkbox_22x22_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/controls/foundry_radio_22x22_v1.png" "$V2_DIR/assets/foundry_ui/controls/foundry_radio_22x22_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/controls/foundry_table_header_320x24_v1.png" "$V2_DIR/assets/foundry_ui/controls/foundry_table_header_320x24_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/controls/foundry_table_row_302x26_v1.png" "$V2_DIR/assets/foundry_ui/controls/foundry_table_row_302x26_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/controls/foundry_vscroll_track_16x16_v1.png" "$V2_DIR/assets/foundry_ui/controls/foundry_vscroll_track_16x16_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/controls/foundry_vscroll_thumb_16x18_v1.png" "$V2_DIR/assets/foundry_ui/controls/foundry_vscroll_thumb_16x18_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/controls/foundry_vscroll_up_16x16_v1.png" "$V2_DIR/assets/foundry_ui/controls/foundry_vscroll_up_16x16_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/controls/foundry_vscroll_down_16x16_v1.png" "$V2_DIR/assets/foundry_ui/controls/foundry_vscroll_down_16x16_v1.png"
cmake -E make_directory "$V2_DIR/assets/foundry_ui/notifications"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/notifications/foundry_notification_340x92_v1.png" "$V2_DIR/assets/foundry_ui/notifications/foundry_notification_340x92_v1.png"
cmake -E make_directory "$V2_DIR/assets/foundry_ui/cards"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/cards/foundry_divider_320x8_v1.png" "$V2_DIR/assets/foundry_ui/cards/foundry_divider_320x8_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/cards/foundry_resource_card_320x62_v1.png" "$V2_DIR/assets/foundry_ui/cards/foundry_resource_card_320x62_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/buttons/foundry_secondary_button_144x30_v1.png" "$V2_DIR/assets/foundry_ui/buttons/foundry_secondary_button_144x30_v1.png"
cmake -E copy_if_different "$FOUNDRY_DIR/assets/foundry_ui/buttons/foundry_action_bar_340x42_v1.png" "$V2_DIR/assets/foundry_ui/buttons/foundry_action_bar_340x42_v1.png"

# Foundry is the base game, not a Victoria 2 mod. Keep Foundry-owned Production
# UI definitions and graphics in the base runtime so they load with no mod
# selected. These currently live beside the imported Rise of Nations data and
# can be moved into a dedicated Foundry runtime tree as that migration proceeds.
cmake -E copy_if_different "$FOUNDRY_DIR/mod/Rise of Nations/interface/country_production.gui" "$V2_DIR/interface/country_production.gui"
cmake -E copy_if_different "$FOUNDRY_DIR/mod/Rise of Nations/interface/country_production.gfx" "$V2_DIR/interface/country_production.gfx"
cmake -E copy_if_different "$FOUNDRY_DIR/mod/Rise of Nations/gfx/interface/foundry_project_icons.png" "$V2_DIR/gfx/interface/foundry_project_icons.png"
cmake -E copy_if_different "$FOUNDRY_DIR/common/civic_buildings.txt" "$V2_DIR/common/civic_buildings.txt"

cd "$V2_DIR"
export LD_LIBRARY_PATH="$FOUNDRY_DIR/build/dependencies/luajit/luajit-prefix/lib:${LD_LIBRARY_PATH:-}"
exec "$FOUNDRY_DIR/build/Launcher/launch_alice"
