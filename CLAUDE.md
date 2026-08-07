# Foundry — a Project Alice fork for the Rise of Nations Vic2 mod

This file exists so a fresh Claude Code session (any machine) can pick up this project with full context. It's the durable record — treat it as more authoritative than any prior conversation, and update it whenever a major decision or convention changes. This mirrors the sister project's own convention — see `../Rise-of-Nations/CLAUDE.md`.

## What this is

A personal fork of [schombert/Project-Alice](https://github.com/schombert/Project-Alice) (an open-source C++ reimplementation of the Victoria 2 engine, GPL-3.0, itself a continuation of the Open V2 codebase), forked to `github.com/Buntcake14/Foundry` on 2026-08-07 and cloned locally to `/run/media/seth/Games/Projects/Foundry` (sibling to `../Rise-of-Nations/`, the actual mod content repo — kept as two fully separate repos on purpose, see "Why two repos" below).

**Fork-only, main branch only.** Upstream (`schombert/Project-Alice`) had only two other branches at fork time (`eriks_branch`, `experimental-map-text`) — both read as in-progress/experimental, not needed as a baseline. They're still fetchable from upstream later if that changes.

## Why this exists

The Rise of Nations mod (`../Rise-of-Nations/`) spent a long time pushing vanilla Victoria 2's closed engine to its limits — see that repo's own CLAUDE.md for the full history (a mesh-builder crash from a hardcoded province-corner buffer, a hardcoded 23-frame unit icon strip that crashes the military screen if exceeded, no live effect to force-construct a building anywhere except a country's capital state, no documented `on_state_conquest` scope, `common/buildings.txt` factory entries having no trigger/condition field at all, a fixed-size GUI background texture that had to be manually stretched twice). Each of those got worked around, not fixed, because the original engine is closed-source.

The concrete trigger was a specific feature: the mod's "Government Administration" building system (`../Rise-of-Nations/events/Government Administration.txt`, `decisions/Government Administration AI.txt`, `common/buildings.txt`/`common/production_types.txt`) — a 4-tier administrative-building chain implemented as a real `type = factory` building (Vic2 has no other extensible building type) whose actual buff is delivered separately via province modifiers synced through an event, because vanilla Vic2 has no conditional/trigger field on factory buildings and no way to force-construct one outside the AI's own workaround (a decision that grants the modifier with **no physical building at all** — see that repo's CLAUDE.md "Government buildings" section for the full reasoning and known asymmetries this caused, e.g. conquering AI land gives you the stat bonus with nothing to see in the production UI, since the AI's path never builds anything real).

Since this is **single-player, for personal use only** (not something being distributed to other players), the usual reasons to stay inside vanilla's constraints (mod portability, requiring players to install a custom engine build) don't apply. Project Alice already ships real Linux support (see "Build" below) and loads Victoria 2's actual game/mod files directly, so the plan is: get Foundry building and running the existing Rise of Nations mod unmodified first, then start extending the engine itself for features vanilla Vic2's script language can't express.

## Why two repos, not one

`../Rise-of-Nations/` (the mod's Paradox-script content: provinces, events, decisions, common/, history/, etc.) is kept completely separate from this engine fork, deliberately, so that **if Foundry work goes badly wrong, the mod keeps working standalone in vanilla/base Victoria 2 exactly as it does today.** Nothing in this repo touches that one. The long-term intent (not yet started) is for Foundry to eventually load that mod's content directly, same as Project Alice loads any other Vic2 mod — not to fork or duplicate the mod's content into this repo.

## Two target features driving this (design intent, not yet implemented)

1. **A "real" government administration system** — replace the mod's province-modifier-riding-a-fake-factory workaround with an actual first-class mechanic, once we have source access to make administration a real conditional/triggered building type instead of an always-buildable factory with a companion event.
2. **One urban center per state** — restrict each state to a single designated village/town/city province where all factories *and* admin buildings can exist, simulating a real urban core instead of Vic2's status quo (the engine already silently assigns factories to some arbitrary province within a state; nothing currently makes that explicit or meaningful, and it undercuts this mod's whole "deliberately split states as a permanent reason for border wars" design philosophy — see `../Rise-of-Nations/CLAUDE.md`'s "Breakup philosophy" — since a building's benefit can currently leak across a split state via `state_scope = { has_building = X }` aggregating the whole geographic state regardless of which owner actually holds the building's province).

**Both ideas turned out to already have a real foothold in Project Alice's source**, found 2026-08-07 while first exploring the codebase — this is the concrete starting point, not a from-scratch build:

- **`src/economy/economy_government.cpp`/`.hpp`** — NOT building-placement code. It's a fully simulated tax/administration economy: `tax_collection_rate`, `count_active_administrations`, separate `capital_administration` vs `local_administration` "control production" functions, real `employment_record`s for people staffing local vs. capital administration. Project Alice has already replaced vanilla Vic2's flat `tax_efficiency` stat with administration modeled as real simulated labor.
- **`src/economy/advanced_province_buildings.cpp`/`.hpp`** — defines three "advanced province buildings": `schools_and_universities`, `civilian_ports`, and **`local_cities_and_towns`** (internal comment: `// cities`). The last one already models urban housing capacity per province — construction cost/build time tied to `province_building_type::railroad`'s costs, size growing or decaying based on profitability and labor, maintenance with decay if unmaintained (`services::list::urban_housing`, `economy::labor::basic_education` as its throughput labor type, `.requires_labor = false` since construction/maintenance itself is what consumes labor here, not ongoing output labor). This is most of the way to the urban-center idea already, just not yet gating factory/admin placement to a single such province per state.
- **The file's own header comment, from the actual maintainer**: *"Advanced province buildings are a future replacement for province buildings... Currently they are hardcoded as 'enums with properties'. Future work should be directed toward moving their definitions into content files and integration with dcon."* — a standing invitation to do close to exactly what both target features need: take this out of hardcoded C++ enums (`services::list::*` / `advanced_province_buildings::list::*`) and make it a real moddable content type, at which point our own admin-tier buildings and a real one-city-per-state gate become natural additions to an existing system rather than something bolted on.

Nothing has been implemented yet — this section is a design-intent record from initial codebase exploration, to be updated as real work starts.

## Build

**Confirmed real native Linux support** (not just a Windows binary run through Proton/Wine) — `src/entry_point_nix.cpp` exists alongside `entry_point_win.cpp`, and upstream ships a GitHub Actions AppImage build. CI (`.github/workflows/appimage.yml`) validates against **clang-20 on Ubuntu 24.04**; this machine is Ubuntu 26.04, close enough that it should be fine but worth knowing if a compiler-version issue ever surfaces.

Debian/Ubuntu build deps (matches both `docs/contributing.md` and the actual CI workflow):
```
sudo apt update
sudo apt install git build-essential clang cmake libgl1-mesa-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libicu-dev
```

**Known gotcha, already satisfied by this clone's location**: the bundled Intel TBB library fails to compile if the project's own path contains any spaces. `/run/media/seth/Games/Projects/Foundry` has none. (Victoria 2's own install path *does* have a space — `.../steamapps/common/Victoria 2` — but that's irrelevant to this gotcha, which is specifically about Foundry's own source path, not the game install it points at.)

Build commands (from `docs/contributing.md`, "Linux (Generic)" section):
```
cmake -B build . -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel --target launch_alice   # the launcher
cmake --build build --parallel --target Alice          # the main game (full rebuild every time)
cmake --build build --parallel --target AliceIncremental  # same, but split into smaller translation units for fast iterative rebuilds — prefer this one day-to-day
```
`SaveEditor` is Windows-only per the docs, skip it. The full `Alice` target combines nearly all source into one translation unit — a single-line change can mean a ~10 minute rebuild — so use `AliceIncremental` for actual development; GitHub CI builds the plain `Alice` target, so that's still worth doing at least once before considering something done, to catch anything that only breaks under the combined build.

**"Final touches" step (from the docs, not yet done here)**: Alice needs to find Victoria 2's actual game files (graphics, etc.) to run. This machine's Victoria 2 install is at `/run/media/seth/Games/SteamLibrary/steamapps/common/Victoria 2/` (confirmed — this is also where `mod/Rise of Nations` is symlinked per the mod repo's own CLAUDE.md). The docs describe copying this repo's `assets/` folder into that directory and configuring the build/debug launcher's working directory to point there — needs to be actually done and this section updated once it has been.

## Current status (as of 2026-08-07)

Cloned and confirmed clean (`git status` clean, on `main`, tracking `origin/main`). **Not yet built.** Build dependencies not yet installed on this machine (`cmake`, `clang++`/`g++` all confirmed absent before this session). Next concrete step: install the apt dependencies above, run the CMake configure + build, then do the "Final touches" step to point it at the real Vic2 install and confirm it actually launches — before any code changes are attempted.

## Process notes

- This is a **personal, single-player fork** — no obligation under GPL-3.0 to publish changes, since nothing is being distributed to other players. Forking never obligates contributing back upstream either.
- Repo naming: forked as "Foundry" (user's choice) — nods to Victoria 2's core industrialization loop and quietly echoes the mod's original working title "Forge of Nations" before it became Rise of Nations.
- GitHub CLI (`gh`) is not installed on this machine; repo operations so far have gone through the GitHub REST API directly via `curl` (unauthenticated, read-only — works fine for public repos) or the GitHub web UI for anything requiring auth (forking itself).
