# Foundry UI Assets

This directory contains the reusable raster component library for Foundry's
player-facing interface.

## Directory ownership

- `source/` — approved references and unsliced production source sheets.
- `frames/` — reusable outer window and inner-panel surfaces.
- `buttons/` — primary, secondary, and icon-button state strips.
- `tabs/` — active, inactive, hover, and disabled tab states.
- `controls/` — scrollbars, sliders, progress bars, toggles, and selectors.
- `icons/` — gold interface, system, and action icons.
- `status/` — semantic normal, warning, alert, and critical indicators.
- `notifications/` — notification frames and category decoration.
- `resources/` — colorful physical commodity/resource artwork.
- `decorations/` — dividers, corners, and restrained ornamentation.

## Rules

1. Dynamic text, numbers, progress, and game state must not be baked into
   artwork.
2. Interface/action icons are gold; physical commodities remain colorful.
3. State strips use the order documented beside each consuming sprite
   definition. Do not assume every Alice widget uses the same frame order.
4. Production assets should be 32-bit PNG with alpha unless an engine-tested
   exception is documented.
5. Source sheets are not loaded directly by the game. Crop, clean, size, and
   test individual production assets first.
6. Validate every component at its actual in-game size.

## Initial source references

- `source/foundry_toolkit_components_v1.png` — generated text-free component
  source based on the approved Foundry toolkit and topbar references.
- `source/foundry_topbar_reference.png` — approved topbar direction supplied by
  the project owner.

## Implemented components

### Primary button — 200 x 48

- Production strip: `buttons/foundry_primary_button_200x48_v1.png`
- Master: `source/foundry_primary_button_states_source_v1.png`
- Frame order: default, hover, pressed, disabled
- Frame size: 200 x 48; strip size: 800 x 48
- Runtime sprite: `GFX_foundry_primary_button_200`
- First integration: Urban Center build/upgrade button

The generated master is normalized by `tools/ui/normalize_button_strip.py`.
Preserve it so revised crops can be produced without regenerating the artwork.

### Primary button — 144 x 30

- Production strip: `buttons/foundry_primary_button_144x30_v1.png`
- Frame order: default, hover, pressed, disabled
- Frame size: 144 x 30; strip size: 576 x 30
- Runtime sprite: `GFX_foundry_primary_button_144`
- First integrations: Roads, Urban Center, and Upgrade RGO province controls

### Close button — 24 x 24

- Production strip: `buttons/foundry_close_button_24x24_v1.png`
- Master: `source/foundry_close_button_states_source_v1.png`
- Frame order: default, hover, pressed, disabled
- Frame size: 24 x 24; strip size: 96 x 24
- Runtime sprite: `GFX_foundry_close_button_24`
- First integration: Urban Center window
- Normalizer: `tools/ui/normalize_close_button_strip.py`

### Sub-tabs — 120 x 30

- Production strip: `tabs/foundry_subtabs_120x30_v1.png`
- Frame order: inactive, active
- Frame size: 120 x 30; strip size: 240 x 30
- Runtime sprite: `GFX_foundry_subtabs_120`
- First integration: temporary Urban Center component test bench
- Extractor: `tools/ui/extract_toolkit_tabs.py`

### Inset panel — 250 x 66

- Production asset: `frames/foundry_inset_panel_250x66_v1.png`
- Runtime sprite: `GFX_foundry_inset_panel_250`
- First integration: Urban Center level/capacity summary

### Section header — 360 x 28

- Production asset: `frames/foundry_section_header_360x28_v1.png`
- Runtime sprite: `GFX_foundry_section_header_360`
- First integration: Urban Center construction-status header
- Surface extractor: `tools/ui/extract_toolkit_surfaces.py`

### Status indicators — 24 x 24

- Production strip: `icons/foundry_status_indicators_24x24_v1.png`
- Master: `source/foundry_status_indicators_source_v1.png`
- Frame order: positive, warning, negative, information
- Frame size: 24 x 24; strip size: 96 x 24
- Runtime sprite: `GFX_foundry_status_indicators_24`
- First integration: temporary Urban Center Effects-tab rows
- Normalizer: `tools/ui/normalize_status_indicators.py`

### Progress bar — 240 x 14

- Production assets: `controls/foundry_progress_fill_240x14_v1.png` and
  `controls/foundry_progress_empty_240x14_v1.png`
- Runtime sprite: `GFX_foundry_urban_progress`
- First integration: Urban Center construction progress
- Generator: `tools/ui/generate_foundry_progress_bar.py`
