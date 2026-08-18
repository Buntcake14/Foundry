# Foundry UI Implementation Roadmap

This roadmap turns `FOUNDRY_GUI_TOOLKIT.md` into engine-ready assets and
reusable GUI elements.

The overhaul must also reserve the maintenance, recurring-cost, supply,
operating-method, and effective-modifier patterns defined in
`docs/BUILDING_MAINTENANCE_AND_OPERATING_METHODS.md`. Those fields are part of
the planned Foundry information architecture even though their mechanics are
not implemented during the toolkit phase.

## Milestone 1 — Component test window

Build a development-only Foundry toolkit test window containing:

- Outer frame and title bar
- Parchment inner panel
- Primary button: default, hover, pressed, disabled
- Secondary button: default, hover, pressed, disabled
- Active and inactive tabs
- Normal, alternate, hover, selected, and disabled rows
- Scrollbar
- Slider
- Progress bar
- Section divider

The test window is the rendering laboratory for the toolkit. Components do not
graduate into production windows until their scaling, alpha, text placement,
and interaction states are correct in-game.

## Milestone 2 — Foundry topbar shell

Replace the existing topbar frame and navigation with the approved structure:

1. National identity
2. Economy
3. Treasury
4. Technology
5. Government
6. Population
7. Markets
8. Diplomacy
9. Military
10. States

Each module is both navigation and a small national dashboard. Initial values
must come from real game state; unavailable future values use intentionally
empty slots rather than invented data.

Status: first shell pass underway. The existing F1-F8 navigation bindings are
preserved while the module frames and terminology move to Foundry. The States
module is initially a non-functional reserved shell until its production
window exists. National identity placement and the final live dashboard field
selection follow the geometry playtest.

## Validated component inventory

The Urban Center laboratory currently contains engine-tested implementations of:

- Primary buttons and close buttons
- Subtabs and inset/section panels
- Status indicators and tooltips
- Progress bars, dropdowns, sliders, checkboxes, and radio buttons
- Selectable tables/listboxes and the Foundry vertical scrollbar
- Five-category notification popup (information, economic, diplomatic, military, critical)
- Ornamental dividers and data-driven resource cards

The resource card is exposed on the laboratory's Construction tab and reads
the first required commodity directly from the selected province's next Urban
Center level. It disappears outside its owning tab, confirming reusable cards
can be scoped cleanly to a content view.

The Supply tab now also hosts the action-bar laboratory: a secondary Cancel
action and primary Apply action with pending, committed, and disabled states.

Notification testing is exposed from the laboratory's Effects tab. Repeated use
cycles the semantic categories; each popup supports explicit dismissal.

## Milestone 3 — Shared production components

Move validated component definitions into a shared Foundry GUI/GFX layer and
use them for all new windows. Retire temporary test-window definitions after
the components have production consumers.

Status: underway. `assets/foundry_toolkit.gui` and
`assets/foundry_toolkit.gfx` are now the shared base-game component layer. The
validated Foundry scrollbar, table row, notification, secondary action button,
and footer surface have been migrated there while the Urban Center laboratory
continues consuming them unchanged.

The shared vertical scrollbar deliberately maps the extracted arrow sprite
strips to their correct visual directions while retaining semantic up/down
button behavior.

## Milestone 4 — Module windows

Rebuild one module at a time using the common toolkit. Preserve functional
mechanics during migration and remove the superseded Victoria II/Alice window
only after its Foundry replacement passes playtesting.
