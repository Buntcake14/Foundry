# Foundry Console Commands

Type `foundry-help` in the in-game console to display the short version of this reference.

The console uses Forth-style argument order. Put a commodity before a command:

```text
coal market-shadow
iron market-live-audit
```

## Market snapshots

These are read-only, run once, and do not remain active.

| Command | Purpose |
|---|---|
| `<good> market-shadow` | Runs the routed-market shadow calculation for the selected province (or national capital) and five nearby markets. Shows prices, supply, demand, unmet demand, routes, transport cost, tariffs, and delivered prices. |
| `<good> market-shadow-wide` | Wider version of `market-shadow`, covering up to 25 markets. Its report may require scrolling. |
| `market-shadow-batch` | Runs a compact 12-good shadow snapshot over 25 markets for performance and aggregate-behavior testing. |

## Live market audits

These run repeatedly as game time advances and can reduce simulation speed. Use `market-live-audit-off` when finished.

| Command | Purpose |
|---|---|
| `<good> market-live-audit` | Enables a daily comparison for one commodity. Vanilla clearing remains authoritative. |
| `market-live-audit-basket` | Enables a daily 12-good comparison over 25 state markets. |
| `market-live-audit-all` | Enables a daily all-active-goods comparison over 25 state markets. |
| `market-live-audit-50` | Tests all active goods over 50 state markets. |
| `market-live-audit-100` | Tests all active goods over 100 state markets. This can noticeably slow maximum speed. |
| `market-live-audit-world` | Tests all active goods over all reachable state markets. Routed results refresh weekly to limit cost. |
| `market-access-audit-world` | Tests the lightweight tiered state-access model worldwide each day without route pathfinding. |
| `market-live-status` | Shows whether an audit is active, its cadence, run/failure totals, market and goods counts, deltas, runtime, and search count. |
| `market-live-audit-off` | Disables the active live-market audit. Always use this after a performance test. |

Only one live market audit configuration is active at a time; starting another replaces the previous configuration.

## RGO AI audits

These commands are read-only reports.

| Command | Purpose |
|---|---|
| `rgo-ai-audit` | Ranks RGO upgrade candidates for the current nation using shortage, utilization, price, labor, trend, and remaining natural potential. |
| `rgo-ai-world-audit` | Lists actual RGO selections made by nations worldwide, including score, shortage, utilization, trend, and progress. |
| `rgo-government-audit` | Lists active government-funded RGO upgrades worldwide and their stockpiling, queue, construction, goods, progress, and capacity status; its header also counts active private RGO projects. |

## Urban-center AI audits

These commands are read-only reports.

| Command | Purpose |
|---|---|
| `urban-ai-audit` | Shows the current nation's ranked urban-center locations and the live factory-demand trigger rules. |
| `urban-ai-world-audit` | Lists actual AI-funded urban-center projects worldwide, including level, stockpiling/building state, acquired goods, and progress. |

## Construction test shortcuts

These commands modify the game state and are intended for development testing.

| Command | Purpose |
|---|---|
| `add-road` | Instantly adds one completed Road Network level to the selected land province. It bypasses money, goods, queue time, and construction capacity, then refreshes transport-route caches. |

## General console commands used during Foundry testing

These are inherited Project Alice commands rather than Foundry-specific diagnostics, but are often useful:

| Command | Purpose |
|---|---|
| `clear` | Clears console output. |
| `fps true` / `fps false` | Enables or disables the FPS display. |
| `dump-econ` | Toggles the economy CSV/debug dump. |
| `complete-construction` | Completes construction using the command's required nation argument. This is broader than Foundry's targeted `add-road` shortcut. |

Update this file whenever a Foundry console command is added, renamed, or removed.
