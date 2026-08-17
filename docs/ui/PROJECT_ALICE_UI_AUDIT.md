# Project Alice UI Removal Audit

This audit tracks the removal of Project Alice-specific interface exposure while
preserving the restored Victoria 2 interface and intentional Foundry mechanics.

## Removed from normal play

- Project Alice map modes 23-44 and the expanded-map-mode toggle.
- Day/night lighting toggle and sun-rotation controls.
- Economy-viewer button and province-tile links into the economy viewer.
- Visible console button. The Foundry development console remains available
  through the tilde key for testing.
- Macro-builder button.
- Province actions for moving the capital, toggling administration, taking a
  province, and granting a province.
- Unit and multi-unit AI-control toggles.
- Automated army-group/battle-planner entry button.
- Hidden keyboard routes into the battle planner, economy viewer, and alternate
  Alice production scene.
- Army build-to-template control and Alice-only siege, strategic-redeployment,
  and pursuit order buttons.
- Alice event requirements and event-odds icons (event choices remain intact).
- Alice's custom province map-detail overlay for capital, railway, fort, bank,
  and university markers.
- Nation-picker observer, gamerules, show-all-saves, and Alice readme widgets.
- Player-facing Project Alice launcher and game-window branding; these now read
  Foundry.

The underlying code and data for several of these systems remains loadable so
old assets, scenarios, and saves do not fail merely because a named GUI element
exists. They are no longer reachable through the normal interface.

## Preserved Victoria 2 interface

- Standard map modes 1-22.
- Main menu, ledger, province search, zoom controls, and outliner.
- Outliner category filters and ordinary army/navy management.
- Restored trade, budget, production, politics, diplomacy, military, and
  province interfaces.

## Preserved Foundry interface

- Roads and road construction.
- Urban centers and the urban-center detail window.
- RGO levels and government/private RGO upgrades.
- Construction capacity and the Projects tab.
- Foundry market and AI audit commands through the development console.

## Playtest checklist

1. Start without selecting a mod and enter a nation.
2. Confirm the launcher and game window are branded Foundry and that the nation
   picker has no Alice readme, observer, gamerules, or save-filter controls.
3. Confirm the minimap has only the standard map-mode row and ordinary utility
   controls.
4. Click commodity/RGO tiles in the province window and confirm no economy
   overlay opens.
5. Open a province and confirm no move-capital, administration, take-province,
   or grant-province controls appear.
6. Select one army and multiple armies; confirm no AI-control, template,
   special-order, or army-group controls appear.
7. Open several events and confirm their choices work without Alice's odds and
   requirements icons.
8. Confirm the outliner filters, ledger, province search, zoom, and tilde
   development console still work.
9. Confirm Foundry roads, urban centers, RGO upgrades, and Projects remain
   accessible.
