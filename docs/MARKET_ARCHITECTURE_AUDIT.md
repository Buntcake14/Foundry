# Foundry Market Architecture Audit

Status: source audit and implementation planning only. No economy behavior was changed by this audit.

This report answers the architectural questions in Section 28 of
`MARKETS_AND_TRANSPORT.md`. It describes the current Foundry/Project Alice
implementation and proposes a staged path toward Foundry's local-first,
transport-constrained economy.

## Executive conclusion

The current engine is not a vanilla Victoria II global pool. It already has a
useful state-market foundation:

- Every state instance has one local market.
- POP, factory, artisan, RGO, construction, and government demand is aggregated
  into that market.
- Each market has per-good supply, demand, price, satisfaction, imports,
  exports, and merchant inventory.
- Markets are linked by persistent, bidirectional, per-good trade routes.
- Routes distinguish land and sea paths and already account for distance,
  tariffs, embargoes, war, blockades, transportation labor, and port service.
- Commodity markets and trade routes are processed in parallel.

This is close to the appropriate performance granularity for Foundry. The
recommended direction is therefore to preserve producer and consumer
accounting and evolve the route layer into a physical, capacity-constrained
transport graph.

The largest missing pieces are explicit route throughput, congestion pricing,
better multimodal path composition, a clear delivered-price model, route/path
caching tied to infrastructure changes, and player-facing explanations.

## 1. Current market unit

`local_market` is a saved one-to-one relationship between a `market` and a
`state_instance`. In normal play, a state is therefore the commodity-market
clearing unit.

Relevant definitions:

- `src/gamestate/dcon_generated.txt`: `market`, `local_market`, and
  `trade_route` definitions.
- `src/nations/nations.cpp`: creation of initial state instances and markets.

This is an important performance advantage: individual POPs and factories do
not pathfind to individual sellers.

## 2. Where goods are produced

### RGOs

`update_rgo_production` calculates province-level output from employed workers
and output per worker. It then aggregates every province's output into its
state market with `register_domestic_supply`.

### Factories

`update_single_factory_consumption` determines affordable inputs, employment,
output, input cost, and profit. `update_factories_production` subsequently
registers factory output in the factory province's state market.

### Artisans

Artisan output is calculated at province level and registered in the
province's state market by `update_artisan_production`.

Primary file: `src/economy/economy_production.cpp`.

## 3. How supply is aggregated

All physical commodity output reaches a market through
`register_domestic_supply`. Imported trade supply uses
`register_foreign_supply`. Both increment the market's per-commodity `supply`;
domestic supply additionally contributes to local GDP.

Primary files:

- `src/economy/economy_stats.cpp`
- `src/economy/economy.cpp`
- `src/economy/economy_trade_routes.cpp`

Unsold supply is not placed into a universal world pool. Market clearing moves
unsold local supply into that market's merchant stockpile, subject to spoilage.

## 4. How demand is calculated

`register_demand` increments per-good demand for a specific market.
`register_intermediate_demand` additionally records production input demand.

Demand sources include:

- POP life, everyday, and luxury needs
- factory and artisan inputs
- construction goods
- national and administrative purchases
- private investment
- advanced province buildings and services
- trade-route purchases at the exporting market

Demand is cleared and rebuilt during every economy update. Historical demand
is exponentially smoothed and used by expected satisfaction and trade-volume
decisions.

## 5. POP purchases

POPs are budgeted individually, but commodity demand is aggregated by market,
POP type, need category, and commodity. POPs do not independently select a
seller or calculate a route.

The market's actual purchase-satisfaction probability is subsequently applied
back to POP needs satisfaction. This is the correct broad architecture for the
target system: POP-level consequences with market-level routing.

Primary file: `src/economy/economy_pops.cpp`.

## 6. Factory and artisan input purchases

Factories and artisans read their local market's prices and expected access to
inputs. Their affordable/available inputs determine realized production and
profitability. Their input requirements are registered as intermediate demand
in the same state market.

They currently care about the local market price, not an explicit supplier's
production price plus a separately exposed route charge. Trade costs influence
route profitability and therefore local availability indirectly.

Primary file: `src/economy/economy_production.cpp`.

## 7. Market clearing and inventory

For every market and commodity, `daily_update` compares total local supply and
total local demand. It calculates:

- actual probability to buy
- actual probability to sell
- smoothed expected probability to buy
- smoothed expected probability to sell
- realized consumption
- merchant inventory changes
- national-stockpile draw when enabled at the capital market

Shortages are therefore market-specific. Unsold goods become local merchant
inventory rather than teleporting to another buyer.

Primary clearing block: `src/economy/economy.cpp`, around the `clear_market`
profile section.

## 8. Price formation

Each market has an independent price for each commodity. Prices move gradually
from historical local supply and demand and are clamped to commodity minimum
and maximum bounds. A commodity-level median price is also calculated for
statistics and several expectations.

The nation and world price helper functions are weighted/aggregate views; they
are not a universal purchasing pool.

Primary files:

- `src/economy/economy_stats.cpp`
- `src/economy/economy.cpp`, `update commodity prices` section
- `src/economy/economy_templates_pure.hpp`

## 9. Current trade routes

Each saved `trade_route` connects two markets and stores:

- effective, land, and sea distance
- land/sea eligibility
- forbidden-trade and tariff flags
- volume and stabilization volume for every commodity

Trade direction and volume evolve from the profitability of moving each good
between the endpoint markets. The calculation considers endpoint prices,
expected buying and selling success, tariffs, distance-related transport cost,
transport availability, merchant margin, embargo/war conditions, and goods
lost in transit.

The route registers demand in the exporting market. Later accounting transfers
the purchased quantity, payments, and tariff revenue between the endpoints.

Primary file: `src/economy/economy_trade_routes.cpp`.

## 10. Route topology and geography

Initial land routes are created between geographically neighboring states.
Initial sea routes are selected from coastal access, population, distance,
naval infrastructure, and connectivity rules. Domestic disconnected coastal
regions receive additional connections.

Land effective distance is calculated along a province path. Rivers, coastal
connections, movement cost, and railroads already reduce it. Sea distance uses
a sea path and available transport-unit speed.

This means the current implementation already approximates the desired
"nearby first" behavior through a sparse network and transport cost, although
it does not yet offer the explicit capacity/congestion model in the design.

Primary file: `src/nations/nations.cpp`.

## 11. Tariffs, policy, spheres, and market access

Import and export tariff rates are nation settings multiplied by tariff
efficiency. Route endpoints decide whether tariffs apply. War, embargoes,
spheres, market leaders, land/sea trade bans, and blockades can forbid or alter
trade.

The active market-clearing loop is not ordered by `nations_by_rank`; an old
rank-ordered loop is commented out. Great-Power rank is therefore not the
primary local purchasing queue in the current implementation. Spheres still
affect route access and tariff/embargo behavior and require dedicated gameplay
tests before their final Foundry role is chosen.

Primary files:

- `src/economy/economy_trade_routes.cpp`
- `src/economy/economy_stats.cpp`
- `src/economy/economy_government.cpp`
- `src/nations/nations.cpp`

## 12. Infrastructure effects today

Railways already reduce calculated land-route distance when both provinces on
an edge have railway levels. River and coastal adjacency also reduce effective
distance. Naval bases, civilian ports, transport units, port-service price,
port-service satisfaction, and blockade status affect maritime trade.

Market `max_throughput` is calculated from naval bases, railroads, and
population. However, the current trade-volume logic does not yet expose the
clean, conserved edge-capacity system required by the design. Port availability
constrains sea-route expansion, while land transport is largely represented by
labor availability and cost.

## 13. Existing cache and invalidation behavior

The engine already has `trade_route_cached_values_out_of_date`. Province
ownership/control, diplomacy, war, sphere changes, and related commands mark
the cache dirty. The game loop recalculates market distances when necessary.

This is a strong starting point for event-driven transport-graph updates.
Railroad, port, canal, technology, treaty, blockade, and border changes should
eventually feed a more explicit topology/cost/capacity version counter.

Primary files:

- `src/gamestate/system_state.hpp`
- `src/gamestate/system_state.cpp`
- `src/nations/nations.cpp`
- `src/provinces/province.cpp`
- `src/military/military.cpp`
- `src/scripting/effects.cpp`

## 14. Save-game implications

Markets, local-market relationships, routes, route distances, flags, volumes,
and stabilization volumes are already tagged for saving in
`dcon_generated.txt`.

Future persistent additions may include:

- route or edge capacity by mode
- current utilization/congestion
- cached route/path identity or a safely regenerable topology version
- merchant shipping allocation
- transit/access agreements
- transport revenue and ownership, if modeled

Derived paths and route caches should preferably be regenerated after loading
rather than serialized unless profiling proves regeneration too expensive.

## 15. UI assumptions

The restored vanilla-style trade window hides much of Alice's geographic
market model, but specialized Alice UI still reads market prices, route
volumes, and distances.

Important existing surfaces include:

- `src/gui/market_prices_report.cpp`
- `src/gui/market_trade_report.cpp`
- `src/gui/province_economy_overview.cpp`
- `src/gui/province_tiles/gui_province_market_window.hpp`
- `src/gui/gui_tooltips.cpp`
- `src/gui/production.cpp`

Foundry eventually needs native Victoria II-style views/tooltips for local
price, available quantity, origin, delivered cost, route utilization,
shortages, imports, and exports. Debug-oriented Alice reports should remain
available during prototyping even if they are not part of the final player UI.

## 16. Performance assessment

### Existing strengths

- State-level aggregation avoids producer-to-consumer pair matching.
- The market and route loops are vectorized and/or parallelized.
- The route graph is sparse rather than all-to-all.
- Historical expectations damp oscillation.
- A dirty flag already avoids some unnecessary distance recalculation.

### Main risks

- Running pathfinding for every good, producer, or consumer would be
  unacceptable.
- A route count approaching all market pairs would make per-good daily route
  updates expensive.
- Capacity rerouting can oscillate if every route responds immediately to
  congestion and price changes.
- Saving large per-good, per-edge histories would inflate saves and memory.
- Province-level clearing for all commodities would multiply current work
  substantially.

### Required constraints

- Retain state/market aggregation for the initial implementation.
- Pathfind by market node and transport mode, never by POP or factory.
- Keep the physical edge graph sparse.
- Cache candidate paths and delivered-cost components.
- Rebuild topology only on dirty events.
- Update trade allocation incrementally, with hysteresis/smoothing.
- Profile route count multiplied by tradable commodity count before scaling.

## 17. Systems that should remain untouched initially

The first transport prototype should not rewrite:

- POP budgeting or needs calculation
- factory/artisan production functions
- RGO output calculation
- construction purchasing
- labor markets and wages
- national budgets, banks, and loans
- the local market clearing transaction
- existing save compatibility
- restored base-style player windows

These systems can continue registering demand and supply against state markets.

## 18. Proposed incremental architecture

### Layer A: producer/consumer accounting

Keep current state-market registration. Provinces remain the source of output
and effects, while state markets remain the commodity-clearing aggregation.

### Layer B: physical transport graph

Introduce a graph whose nodes initially correspond to state markets. Edges
represent:

- neighboring-state land connections
- navigable river connections
- coastal/sea connections through ports
- later canals and explicit hubs

Each edge exposes cost, capacity, enabled modes, access owner, and current
utilization. Province paths remain useful for deriving an edge's infrastructure
quality and physical distance.

### Layer C: cached market routes

For each market, cache a small number of economically plausible paths to other
markets. Cache topology separately from commodity-specific delivered cost so a
single railway change does not require producer-to-consumer matching.

### Layer D: commodity allocation

Adapt existing per-good route volumes to respect shared edge capacity. Prefer
local clearing automatically because it pays no intermarket transport cost.
Surplus moves onto routes only when expected destination revenue exceeds local
purchase, tariff, loss, and transport costs.

### Layer E: explanation and UI

Expose a decomposed delivered price:

- origin price
- inland transport
- maritime transport
- congestion
- tariff/trade friction
- loss/merchant margin
- delivered total

The UI must explain why a good is unavailable or expensive.

## 19. Recommended first prototype

Use a deterministic sandbox of five state markets, two countries, and four
goods. Do not begin with the full world.

Suggested layout:

```text
Coal State --road-- Industrial State --rail-- Port State
                    |
                  river
                    |
                 Farm State

Port State ===== sea ===== Foreign Port State
```

Prototype features:

1. Fixed local production and demand.
2. Existing local market prices and clearing.
3. Land, river, rail, and sea edges with configurable cost and capacity.
4. One tariff/open-trade toggle.
5. Shared capacity across commodities.
6. Debug output for route, quantity, cost components, congestion, and unmet
   demand.
7. A comparison mode that runs the old route result beside the prototype
   without affecting the live economy.

Success criteria:

- Local supply wins when delivered alternatives are more expensive.
- Rail investment lowers delivered costs and increases sustainable volume.
- Congestion causes marginal traffic to use another route or remain unmet.
- A nearby foreign supplier can beat a distant domestic supplier.
- Closing a border or blockading a port reroutes or interrupts trade.
- Results are deterministic and stable rather than oscillatory.
- Measured update cost supports extrapolation to the full map.

## 20. Recommended implementation order

1. Add instrumentation and deterministic economy/route test fixtures.
2. Record baseline market and route performance on the full scenario.
3. Define transport-node and transport-edge data without changing simulation.
4. Build the five-market shadow prototype.
5. Add explicit edge capacity and utilization to the prototype.
6. Validate delivered-price decomposition and congestion behavior.
7. Integrate one land region behind a feature flag.
8. Add ports/sea routes and international access.
9. Compare economic stability and performance over multi-year simulations.
10. Migrate the full world only after the prototype meets its correctness and
    performance targets.

## Final recommendation

Foundry can achieve the target with manageable simulation cost because the
current engine already aggregates at state-market level and processes a sparse
route network in parallel. The project should evolve the existing trade-route
layer, not make every province or POP perform market searches.

The next authorized coding task should be instrumentation plus a non-invasive,
feature-flagged five-market shadow prototype. It should not yet alter live
campaign trade.
