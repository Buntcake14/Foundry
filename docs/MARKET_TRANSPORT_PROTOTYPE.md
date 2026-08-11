# Foundry Five-Market Transport Prototype

This is the first executable prototype for the target architecture described in
`MARKETS_AND_TRANSPORT.md`. It is deliberately isolated from the live economy.
Running the game does not invoke it, and it cannot change campaign markets,
prices, saves, or trade routes.

## Current model

The prototype provides:

- state-style market nodes
- multiple commodities
- local supply and demand clearing before trade
- road, river, rail, and sea edges
- per-edge transport cost
- shared edge capacity across shipments and commodities
- utilization-based congestion cost
- open and closed routes
- additive prototype border tariffs
- cheapest-delivered-price routing
- decomposed shipment results for origin price, transport, tariff, path, and
  delivered price
- deterministic tie-breaking

The test scenario uses five markets:

```text
Coal State --road-- Industrial State --rail-- Port State
     |                  |
     +------river-------+

Port State ===== sea ===== Foreign Port State
```

## Verification

Configure tests, build the lightweight target, and run it:

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --target foundry_market_prototype_tests -j2
./build/tests/foundry_market_prototype_tests
```

The executable checks:

1. Local goods satisfy local demand before imports.
2. A route cannot exceed its capacity.
3. Traffic spills onto an alternate route when the cheapest route fills.
4. Rail adds a low transport cost to the origin price.
5. A nearby cheap foreign supplier can beat expensive domestic delivery.
6. Tariffs increase foreign delivered price.
7. Closing the international edge stops foreign trade.

## Intentional limitations

This is an algorithm and testing scaffold, not final balance or live gameplay.
It currently uses a deterministic greedy allocator. Congestion is sampled at
the start of each allocated shipment rather than integrated continuously.
Commodity processing order can therefore affect allocation when several goods
compete for the same edge. The tariff is additive for transparent testing,
whereas live Alice tariffs are percentage-based.

The prototype does not yet include:

- live `dcon::market_id` or `dcon::trade_route_id` integration
- cached candidate paths or dirty-flag rebuilding
- merchant shipping
- transit rights
- infrastructure ownership or transport revenue
- route losses
- weekly/incremental allocation
- player UI

These limitations must be addressed or explicitly accepted before enabling the
system in campaigns.

## Next step

A read-only live shadow adapter is now available through the developer console.
It runs only when explicitly invoked and never feeds results back into the live
economy.

To use it:

1. Select a land province whose market should seed the five-market sample.
2. Open the developer console.
3. Enter a commodity followed by `market-shadow`, using the console's postfix
   syntax. For example:

```text
coal market-shadow
```

If no province is selected, the command uses the player's capital. The report
shows the selected live markets, current price/supply/demand, local clearing,
prototype routes, capacity usage, shipments, delivered-price components,
unmet demand, unsold supply, and runtime in microseconds.

Current adapter values are deliberately provisional. In particular, it
converts Alice's effective route distance to prototype transport cost using
`distance * 0.01`, and converts live percentage tariffs into a symmetric
additive border estimate. These values exist to inspect routing behavior, not
to establish final balance.

The next step is to collect reports from several different regions and goods,
then compare shadow shipments against Alice's existing route volumes before
adding a recurring feature-flagged shadow run.
