FOUNDRY — LOCAL MARKETS, TRADE & TRANSPORT SYSTEM
DESIGN HANDOFF FOR CODEX

============================================================
1. PURPOSE
============================================================

Foundry intends to eventually replace Victoria II's global market-pool
model with a geographically grounded market and trade system.

VANILLA VIC2 MODEL:

Goods enter a global market pool.

Access is largely determined by country rank:

Great Powers
    ↓
Secondary Powers
    ↓
Other Countries

This means physical distance, transportation infrastructure and actual
trade relationships have relatively little influence over who can buy
a good.

FOUNDRY SHOULD EVENTUALLY REPLACE THIS.

CORE PRINCIPLE:

"Goods should attempt to satisfy demand as locally as possible.
Moving goods farther away increases their delivered cost, while
infrastructure, geography, trade policy and international agreements
determine where goods can economically travel."


============================================================
2. IMPORTANT CURRENT DEVELOPMENT INSTRUCTION
============================================================

DO NOT immediately replace the current Victoria II market system.

The project is currently restoring/maintaining a clean vanilla Victoria
II baseline from Project Alice.

For now:

1. Inspect how Project Alice currently implements:
   - production
   - supply
   - demand
   - purchasing
   - market pools
   - country market access
   - tariffs
   - trade
   - prices

2. Identify where a future Foundry market/transport layer could be
   introduced.

3. Preserve vanilla behavior unless explicitly instructed otherwise.

4. Avoid architectural decisions that unnecessarily prevent this future
   system.

5. Report findings before attempting a major market rewrite.

This document describes the TARGET ARCHITECTURE, not permission to
immediately implement the entire system.


============================================================
3. LOCAL-FIRST GOODS DISTRIBUTION
============================================================

Instead of immediately sending production into one global pool, goods
should attempt to find buyers geographically.

Conceptually:

PROVINCE PRODUCES GOOD
        ↓
1. LOCAL PROVINCE DEMAND
        ↓ surplus
2. DEMAND ELSEWHERE IN STATE
        ↓ surplus
3. NEARBY / NEIGHBORING STATES
        ↓ surplus
4. WIDER DOMESTIC MARKET
        ↓ surplus
5. ACCESSIBLE FOREIGN MARKETS
        ↓ surplus
6. DISTANT / OVERSEAS MARKETS

This should NOT necessarily be implemented as a rigid literal sequence.

The eventual routing system should find economically sensible buyers
while strongly favoring nearby demand because nearby transportation is
usually cheaper.

The important design outcome is:

LOCAL PRODUCTION NATURALLY HAS A LOCAL MARKET ADVANTAGE.


============================================================
4. DELIVERED PRICE
============================================================

The relevant economic price should eventually be more than simply the
producer's base/local price.

Conceptually:

DELIVERED PRICE =
    GOOD PRICE
  + TRANSPORT COST
  + TARIFFS
  + BORDER / TRADE FRICTION
  + OTHER APPLICABLE COSTS

Example:

Iron produced locally:

Base price:      £10
Transport:        £1
--------------------
Delivered price: £11

Same iron shipped a long distance:

Base price:      £10
Transport:        £6
--------------------
Delivered price: £16

A foreign producer might therefore be competitive even if its actual
production price is higher.

Example:

Domestic distant iron:
£10 production + £6 transport = £16

Nearby foreign iron:
£12 production + £2 transport + £1 tariff = £15

The nearby foreign iron may be the economically preferred purchase.

This is intentional.


============================================================
5. TRANSPORT NETWORK
============================================================

Goods should conceptually travel through a transport graph.

Nodes may eventually include:

- provinces
- states
- urban centers
- ports
- rail hubs
- important trade hubs

Edges may represent:

- roads
- rivers
- canals
- railways
- coastal shipping
- ocean shipping
- potentially other later transportation methods

Each edge should eventually have at least two important properties:

TRANSPORT COST

and

TRANSPORT CAPACITY.


============================================================
6. TRANSPORT COST
============================================================

Moving goods should cost money/resources economically.

Distance should generally increase cost.

Infrastructure reduces it.

Conceptually:

DIRT ROAD
High cost

IMPROVED ROAD
Medium cost

NAVIGABLE RIVER
Low cost

CANAL
Low cost

RAILWAY
Very low land transport cost

SEA ROUTE
Low long-distance bulk cost when proper ports/shipping are available

Exact values are TBD.

Do not hard-code these examples as final balance values.


============================================================
7. TRANSPORT CAPACITY
============================================================

Transport should NOT have infinite throughput.

A route should have limited capacity.

Conceptually:

DIRT ROAD
Cost: High
Capacity: Low

IMPROVED ROAD
Cost: Medium
Capacity: Medium

RIVER
Cost: Low
Capacity: Medium/High

RAILWAY
Cost: Very Low
Capacity: Very High

PORT / SHIPPING
Cost: Low
Capacity: dependent upon maritime infrastructure and shipping

This allows infrastructure congestion.

Example:

An industrial city expands from 2 factories to 15 factories without
improving its transport network.

Demand for:

- coal
- iron
- food
- industrial inputs

dramatically increases.

Finished-goods exports also increase.

If transport capacity is insufficient:

- routes become congested
- delivered input prices rise
- factories become less profitable
- goods struggle to reach consumers
- infrastructure investment becomes economically valuable

Infrastructure therefore becomes a real part of the economy rather
than simply providing percentage modifiers.


============================================================
8. HISTORICAL TRANSPORT EVOLUTION
============================================================

The game begins around 1760.

At game start, long-distance transportation should be substantially
more difficult and expensive than during later industrial periods.

Approximate evolution:

1760:
roads
wagons
rivers
coastal shipping
sailing ships

        ↓

improved roads
canals

        ↓

railways
steamships

        ↓

large rail networks
modern ports

        ↓

later modern transportation/logistics

The important emergent effect:

THE WORLD ECONOMY SHOULD GLOBALIZE OVER TIME.

Do NOT implement a simple:

"Global Market unlocked in YEAR X."

Instead, improving transportation technology and infrastructure should
make increasingly distant trade economically viable.

In 1760, markets should naturally be much more regional.

By the industrial era, continental markets become increasingly
integrated.

Later, large global supply networks become practical.


============================================================
9. RIVERS AND GEOGRAPHY
============================================================

Navigable rivers should eventually matter economically.

Moving bulk goods along a river should generally be substantially
cheaper than moving those same goods over poor land infrastructure.

Example:

IRON PROVINCE
     ↓
NAVIGABLE RIVER
     ↓
URBAN CENTER
     ↓
PORT
     ↓
SEA TRADE

This can help explain why cities and industries emerge in particular
locations.

Geography should create economic opportunities rather than merely
providing static modifiers.


============================================================
10. RAILWAYS
============================================================

Railways should eventually become actual economic infrastructure.

Do not conceptualize them solely as:

"+X% RGO output"

A railway should create a:

LOWER-COST
HIGHER-CAPACITY

transport connection.

Example:

BEFORE RAIL:

Coal province
     ↓
Industrial city

Transport cost: high
Capacity: limited

AFTER RAIL:

Coal province
══════════════
Industrial city

Transport cost: dramatically lower
Capacity: dramatically higher

This can make previously unprofitable industries profitable.

Railways should eventually be shared by both:

ECONOMIC LOGISTICS

and

MILITARY LOGISTICS.

This is an important architectural connection to Foundry's future
military system.


============================================================
11. INTERNATIONAL TRADE
============================================================

Foreign trade should depend upon political relationships and trade
policy.

Possible factors:

- Free trade policy
- Protectionist policy
- Tariffs
- Trade agreements
- Customs unions
- Preferential trade agreements
- Embargoes
- Sanctions
- Market-access treaties
- Transit rights

Exact diplomatic mechanics are TBD.

Core rule:

A country should NOT automatically have unrestricted access to every
good produced anywhere on Earth.


============================================================
12. TRADE RIGHTS
============================================================

Countries should be able to trade where policy and diplomatic
relationships permit it.

Example:

Country A and Country B share a border.

If trade is open:

Goods may cross the border.

If tariffs exist:

Delivered price includes the tariff.

If a trade agreement exists:

Trade friction/tariffs may be reduced.

If an embargo exists:

That trade route is unavailable.

A country's trade network therefore emerges from:

GEOGRAPHY
+
INFRASTRUCTURE
+
POLICY
+
DIPLOMACY


============================================================
13. LANDLOCKED COUNTRIES / TRANSIT
============================================================

Eventually consider transit rights.

Example:

LANDLOCKED COUNTRY
        ↓
NEIGHBOR COUNTRY
        ↓
PORT
        ↓
INTERNATIONAL MARKET

Access to foreign infrastructure may therefore become strategically and
diplomatically important.

Do not implement this yet unless specifically requested.

Preserve the architecture so multi-country transport paths remain
possible later.


============================================================
14. PORTS VS SHIPYARDS
============================================================

Tentative design distinction:

PORT / HARBOR:

- Allows maritime goods movement.
- Handles loading/unloading.
- Reduces maritime trade handling cost.
- Provides maritime throughput.

SHIPYARD:

- Builds/repairs ships.
- May eventually contribute to merchant shipping capacity.

These should probably NOT be treated as the exact same economic
function.

This distinction is still subject to design iteration.


============================================================
15. MERCHANT SHIPPING
============================================================

Potential future system:

Countries possess merchant/shipping capacity.

International maritime trade consumes shipping capacity.

Example:

MERCHANT SHIPPING CAPACITY: 4,850

Export/import routes consume portions of that capacity.

Shipyards and maritime industries may help create/maintain it.

This would eventually allow:

- maritime trading powers
- shipping shortages
- blockade effects
- commerce raiding
- wartime disruption of overseas supply chains

This is NOT finalized for immediate implementation.

Keep it in mind architecturally.


============================================================
16. GREAT POWERS
============================================================

Foundry should eventually remove the artificial principle that:

"Great Power rank means first access to the world's goods."

Great Powers will likely remain economically powerful naturally because
they possess combinations of:

- industrial production
- rail infrastructure
- ports
- merchant shipping
- capital
- banking/finance
- trade agreements
- diplomatic influence
- colonies
- large domestic markets

Their economic dominance should emerge from these systems.

A small country located beside a producer should potentially be able to
buy a good more cheaply than a Great Power located thousands of miles
away.

COUNTRY RANK SHOULD NOT BE A GLOBAL SHOPPING QUEUE.


============================================================
17. CONNECTION TO URBANIZATION
============================================================

This system should eventually integrate with Foundry's Urban Center
system.

Cities should naturally become important where trade networks
intersect.

Examples:

RIVER
+
ROAD
+
PORT
=
TRADE HUB

Later:

RIVER
+
RAIL JUNCTION
+
PORT
+
FACTORIES
=
MAJOR INDUSTRIAL CITY

Trade access can encourage:

- workshop growth
- factory profitability
- migration
- urban expansion
- administrative growth
- financial activity

Urbanization should therefore partially emerge from economic geography.


============================================================
18. CONNECTION TO PRODUCTION
============================================================

Workshops and factories should care about the delivered price and
availability of their inputs.

Example:

STEEL WORKS requires:

IRON
COAL

Profitability should depend partly upon:

- where iron comes from
- where coal comes from
- transport cost
- route capacity
- tariffs
- market access

Finished steel then needs transportation to buyers.

This creates actual industrial geography.

A factory's location matters.


============================================================
19. CONNECTION TO POPS
============================================================

POPs should attempt to purchase goods through the same broader market
logic.

Local availability and delivered price therefore affect:

- POP consumption
- needs satisfaction
- cost of living
- producer profitability
- regional shortages

A grain-producing region may have cheap food.

A remote industrial city dependent upon imported grain may have higher
food costs.

Transportation infrastructure can reduce that difference.


============================================================
20. CONNECTION TO WARFARE
============================================================

The economic transport network should eventually become strategically
important during war.

The same broad network used for commerce may support:

- military supply
- mobilization
- reinforcement
- strategic rail movement

War may disrupt:

- rail connections
- ports
- rivers
- trade routes
- shipping
- border crossings

This connects directly to Foundry's future modern warfare system.

DO NOT build a completely separate fictional logistics network for the
military if the economic transport graph can sensibly be shared.


============================================================
21. PERFORMANCE REQUIREMENT — CRITICAL
============================================================

DO NOT IMPLEMENT THIS AS:

for every producer:
    for every consumer:
        calculate_route()

every game tick.

That will likely become computationally unacceptable.

Foundry may contain:

- thousands of provinces
- many goods
- many POPs
- workshops
- factories
- many countries
- potentially ~200 years of simulation

The market architecture must be designed for performance.


============================================================
22. POSSIBLE PERFORMANCE STRATEGY
============================================================

The following is a design direction, not a mandatory algorithm.

Consider:

TRANSPORT GRAPH
+
AGGREGATED SUPPLY/DEMAND
+
ROUTE CACHING
+
EVENT-DRIVEN / DIRTY-FLAG RECALCULATION

Rather than recalculating the entire world continuously.

Routes may need recalculation when meaningful changes occur, such as:

- railway constructed
- railway destroyed
- port expanded
- war begins
- border changes
- embargo imposed
- tariff changes
- trade agreement signed
- route becomes congested
- major supply/demand shift
- technology changes transport capability

Investigate efficient graph/pathfinding strategies before implementation.


============================================================
23. MARKET GRANULARITY
============================================================

Do not assume every individual POP must independently pathfind to every
producer.

The final implementation may aggregate supply/demand at sensible
levels.

Potential levels include:

PROVINCE
STATE
LOCAL MARKET REGION
TRADE HUB
NATIONAL MARKET

The correct granularity should be determined through profiling and
prototype testing.

The player-facing simulation should FEEL geographically grounded even
if internal calculations use aggregation for performance.


============================================================
24. PRICES
============================================================

Exact price formation remains TBD.

Important conceptual distinction:

PRODUCTION / LOCAL PRICE

is not necessarily identical to:

DELIVERED PRICE.

Transport, tariffs and trade friction can make the same good cost
different amounts in different places.

Avoid locking us into one universal global price if doing so would
prevent meaningful transportation economics.


============================================================
25. DESIGN EXAMPLE
============================================================

Province A produces:

100 units of cloth.

Local demand consumes:

20.

Remaining:

80.

State demand consumes:

30.

Remaining:

50.

Nearby state wants cloth.

Delivered price is competitive.

It purchases:

25.

Remaining:

25.

Neighboring foreign country has open trade access.

It purchases:

15.

Remaining:

10.

A distant overseas buyer exists, but transportation makes the delivered
price too expensive.

The remaining production may:

- seek another buyer
- remain unsold
- lower local prices
- create inventory depending upon final market mechanics

Exact inventory/price-response mechanics remain TBD.


============================================================
26. DESIGN GOAL
============================================================

The desired outcome is that a player can look at an industrial region
and understand WHY it succeeded.

Example:

"This city became an industrial powerhouse because it had nearby coal
and iron, access to a navigable river, later became a rail junction,
developed a major port, attracted workers, and gained access to foreign
markets through favorable trade agreements."

Not:

"This province had the correct +20% modifier."


============================================================
27. FOUNDATIONAL RULE
============================================================

The market system should ultimately follow this principle:

LOCAL PRODUCTION
        ↓
LOCAL DEMAND
        ↓
REGIONAL TRADE
        ↓
DOMESTIC TRADE
        ↓
INTERNATIONAL TRADE
        ↓
GLOBAL ECONOMIC INTEGRATION

with each expansion outward made possible or economical through:

TRANSPORTATION
+
INFRASTRUCTURE
+
TECHNOLOGY
+
TRADE POLICY
+
DIPLOMACY.


============================================================
28. WHAT CODEX SHOULD DO NOW
============================================================

Do NOT begin a wholesale implementation from this document alone.

FIRST:

Inspect the current Project Alice / Foundry codebase.

Determine:

1. Where goods are produced.
2. How supply is currently aggregated.
3. How demand is calculated.
4. How POP purchases work.
5. How factories/artisans purchase inputs.
6. How goods enter the current market.
7. How prices are determined.
8. How country rank affects purchasing.
9. How tariffs currently work.
10. How spheres/market access currently work.
11. How railroads and infrastructure currently affect the economy.
12. Which systems assume one global market.
13. Which data structures would need extension.
14. Which save-game structures would eventually need extension.
15. Which UI screens assume global market behavior.
16. Where an efficient transport graph could integrate with the existing
    simulation.

THEN REPORT BACK WITH:

A. Current Alice/Vic2 market architecture.

B. Files/classes/functions involved.

C. What must remain untouched while vanilla restoration is underway.

D. A proposed incremental Foundry architecture.

E. Performance risks.

F. Suggested prototype scope.

G. Which parts can be added modularly without breaking vanilla behavior.

DO NOT MODIFY THE MARKET SYSTEM YET unless explicitly instructed after
the architectural review.


============================================================
29. PROTOTYPE RECOMMENDATION
============================================================

Before replacing the world economy, create a small test environment.

Potential prototype:

3–5 states
2 countries
3–5 goods

Include:

- local production
- local demand
- neighboring-state demand
- one international border
- tariff/open-trade toggle
- road connection
- river connection
- railway connection
- port/sea connection

Observe:

- where goods flow
- delivered prices
- infrastructure effects
- shortages
- producer profitability
- transport congestion
- foreign trade

Only after the model behaves sensibly should it be scaled to the full
world.


============================================================
30. CORE FOUNDRY ECONOMIC PILLAR
============================================================

The target system can be summarized as:

LOCAL-FIRST MARKETS
+
PHYSICAL TRANSPORT COST
+
TRANSPORT CAPACITY
+
INFRASTRUCTURE
+
TRADE POLICY
+
DIPLOMATIC MARKET ACCESS
+
TECHNOLOGICAL GLOBALIZATION

The world should begin around 1760 as a collection of relatively
regional economies and gradually become an increasingly integrated
global economy because transportation, infrastructure, political
relationships and technology make that integration possible.

The economy should not begin globally integrated simply because every
good is inserted into one universal market pool.