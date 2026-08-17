# Foundry UI Implementation Roadmap

This roadmap turns `FOUNDRY_GUI_TOOLKIT.md` into engine-ready assets and
reusable GUI elements.

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

## Milestone 3 — Shared production components

Move validated component definitions into a shared Foundry GUI/GFX layer and
use them for all new windows. Retire temporary test-window definitions after
the components have production consumers.

## Milestone 4 — Module windows

Rebuild one module at a time using the common toolkit. Preserve functional
mechanics during migration and remove the superseded Victoria II/Alice window
only after its Foundry replacement passes playtesting.

