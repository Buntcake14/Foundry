# Foundry Urban Centers

Implementation note: the reusable geometry, raster, GUI/C++ binding, build,
cache, and testing lessons from this window are documented in
[`docs/FOUNDRY_UI_WINDOW_GUIDE.md`](docs/FOUNDRY_UI_WINDOW_GUIDE.md).

Urban centers are government-planned provincial projects spanning Foundry's
1760–1960 timeline. They are not factories and they are not an abstract
national construction sector. They represent the long-lived physical and
administrative development that allows increasingly complex urban economies
to exist in a province.

## Core rules

- Only a province's government may begin or upgrade an urban center.
- An upgrade enters the national Projects queue and consumes civil construction
  capacity while active.
- Construction goods are purchased from the province's market through the
  national construction budget. Construction cannot progress without them.
- Private investors cannot create or upgrade urban centers.
- Level 1 unlocks new factory construction in the province.
- Higher levels increase local construction capacity, permit later civic and
  industrial systems, and gradually reduce land available to RGOs.
- Urban centers are never automatic rewards for reaching a population number.
  Population, technology, infrastructure, and administration are requirements;
  the government must still choose and fund the project.
- Military-unit construction remains outside civil construction capacity.

## Long-timeline progression

| Level | Working name | Intended era | Role | Total RGO land penalty |
|---:|---|---:|---|---:|
| 1 | Market Town | 1760+ | Establish an urban economy; unlock factories | 2% |
| 2 | Developed Town | 1790+ | Local administration and larger workshops | 4% |
| 3 | Industrial Town | 1830+ | Sustained industrial concentration | 6% |
| 4 | City | 1870+ | Mature rail-era city | 9% |
| 5 | Metropolitan City | 1900+ | Large regional industrial center | 12% |
| 6 | Electrified Metropolis | 1925+ | Electrical and mass-service infrastructure | 14% |
| 7 | Modern Metropolis | 1945+ | Postwar metropolitan development | 16% |
| 8 | Megalopolis | 1955+ | Exceptional late-game urban concentration | 18% |

The dates are design targets, not unconditional calendar unlocks. Final gates
will combine technology with minimum provincial population and infrastructure.
Costs and build times rise sharply so developing several great cities remains
a meaningful national program late in the game.

## Initial allocation rule

Urban-center projects will enter the civil queue after factories and ordinary
province buildings during the first implementation. A later queue-management
pass will let the player reorder projects and establish government priorities.

## Scenario initialization

Historical starting levels must be seeded from scenario data or a one-time
scenario-generation process. They must not be inferred and granted every day.
This lets 1760, 1836, and later bookmarks represent historically different
urban networks without free automatic development during play.
# Universal Two-Stage Construction

All permanent buildings use the same two-stage project lifecycle:

1. **Acquiring goods:** the project purchases and reserves its complete construction recipe. Purchased materials remain attached to that project through market shortages, pauses, and save/load cycles.
2. **Under construction:** only after every required commodity is stockpiled does physical construction begin. Build progress then advances for the building's configured construction time, modified by the share of national construction capacity assigned to the project.

The building becomes operational only when both stages are complete. Factories, factory upgrades and refits, railroads, forts, naval bases, and civic buildings such as Urban Centers all follow this rule. Queued projects do not advance either stage until they receive construction capacity.

Goods acquisition does not consume construction capacity. Any authorized project may reserve materials while other projects are being built. Once fully supplied, it enters the physical construction queue; it may remain `Supplied - Queued` indefinitely until national capacity becomes available.

The Projects UI must always distinguish `Acquiring Goods`, `Supplied - Queued`, and `Building`, show actual reserved/required quantities for every commodity, and show physical build percentage only during the second stage.
