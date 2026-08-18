# Foundry Building Maintenance and Operating Methods

Status: **approved design direction; not yet implemented**

This specification reserves the economic, simulation, AI, and UI architecture
needed for persistent building inputs across Foundry's 1760–1960 timeline.
Exact commodity quantities and modifiers remain balancing data rather than
hard-coded design promises.

## 1. Design goals

Every constructed asset should remain connected to the economy after its
construction finishes. Infrastructure must create continuing demand, advanced
systems must require advanced supply chains, and a temporary shortage must
degrade performance without making the asset instantly disappear.

The system should:

- distinguish construction costs from continuing operating costs;
- create durable demand for basic and late-game commodities;
- make infrastructure expansion an economic commitment;
- allow technological transitions without replacing the entire interface;
- give shortages understandable, proportional consequences;
- remain computationally affordable on a more detailed world map.

## 2. Two distinct input systems

### Maintenance inputs

Maintenance inputs preserve condition and ordinary effectiveness. They are
consumed continuously while the building exists. Examples include lumber and
cement for roads, or steel and machinery for railways.

### Operating methods

An operating method is a player- or AI-selected package of inputs and effects.
It describes *how* an asset operates, not merely whether it is repaired. New
methods are unlocked by technology and may offer stronger benefits at greater
cost or with more sophisticated supply requirements.

Maintenance and operating methods may coexist. An electrified Urban Center can
still require ordinary structural maintenance in addition to electricity.

## 3. Initial building coverage

| Asset | Illustrative maintenance | Illustrative operating choices |
|---|---|---|
| Roads | Lumber, stone/cement | Basic roads, improved roads, surfaced roads |
| Railways | Steel, machinery, lumber | Steam/coal, improved steam, electrified rail |
| Forts | Cement, small arms, ammunition | Garrison posture or fortification doctrine |
| Naval bases | Lumber, cement, steel, machinery | Sail support, coaling station, oil-fuel support |
| Urban Centers | Structural maintenance | Candles, gas lighting, electricity |
| Administration | Paper, furniture, later communications goods | Administrative intensity/method |
| Factories | Existing production inputs plus maintenance where appropriate | Factory production methods may be expanded later |

This table defines categories, not final recipes. Goods such as tools,
electricity, or communications equipment may be added later without changing
the overall architecture.

## 4. Urban Center service progression

Urban Centers are the clearest first consumer of operating methods:

1. **Candles / basic lighting** — available early, inexpensive, modest service
   and capacity effects.
2. **Gas lighting** — technology-gated, stronger urban services and capacity,
   with a more developed input chain.
3. **Electricity** — late-game, high service quality, administration,
   productivity, and building-capacity benefits, but dependent on reliable
   electrical supply.

The player selects the method from a dropdown in the Urban Center portion of
the province interface. Only unlocked methods appear as selectable. The UI must
show each method's required inputs, fulfilled supply, effects at full supply,
and current effective effects.

This choice is independent of Urban Center level and visual era. A large city
may retain an obsolete operating method, while a smaller city may modernize
early if its nation can supply it.

## 5. Fulfillment and effects

Input fulfillment is a continuous percentage from 0% to 100%.

- At 100%, the building provides its full configured effects.
- Partial fulfillment proportionally reduces supply-dependent effects.
- At 0%, supply-dependent effects cease, but the building and its level remain.
- Effects that logically do not require operation may be marked as structural
  and remain active despite an operating shortage.

The initial implementation should favor readable linear scaling. Nonlinear
thresholds may be added only where they improve gameplay and are clearly
communicated.

## 6. Condition and deferred maintenance

Short-term shortages should reduce current effectiveness before causing
permanent harm. A later phase may add building condition:

- unmet maintenance gradually lowers condition;
- lower condition limits maximum effectiveness;
- restored supply repairs condition over time;
- severe neglect never silently deletes a building level;
- repair demand may temporarily exceed normal maintenance demand.

Condition is deliberately deferred until the basic continuous-input model is
stable and understandable.

## 7. Purchasing and payment

- Government-owned assets purchase inputs from the responsible government's
  budget.
- Privately owned assets pay from their appropriate private/investor funds.
- Foreign-owned or subsidized assets follow explicit ownership and subsidy
  rules rather than receiving free goods.
- Purchases use Foundry's market-access model and ordinary commodity prices.
- Maintenance demand must not bypass shortages or create goods from nothing.

Budget screens must eventually separate ordinary expenses, construction
stockpiling, and recurring building maintenance.

## 8. Performance architecture

Foundry must not run a separate route search and purchase auction for every
building in every province each day. Maintenance demand should be aggregated
into state-level commodity baskets, processed through the existing state market
access calculation, then distributed back to buildings as fulfillment ratios.

Recommended flow:

1. Provinces report required quantities by commodity, ownership, and priority.
2. The state aggregates those requirements into a small number of demand
   baskets.
3. The market resolves availability and cost once per basket.
4. Fulfillment is distributed proportionally or by an explicit priority rule.
5. Province UI reads cached fulfillment values; it does not rerun market logic.

Recalculation should use the economy cadence and dirty flags for relevant
changes: a new building, changed method, completed technology, ownership
change, market-access change, or material price/supply change.

## 9. Priority and player control

The first version should use a stable default priority order and avoid
micromanaging every province. Later controls may permit national or state-level
priority policies such as military infrastructure, transport, administration,
or urban services.

Operating-method changes are explicit player commands and must be synchronized
in multiplayer. AI nations may change their own methods; AI must never change
the local player's methods.

## 10. AI requirements

AI evaluation must consider:

- whether the method is technologically unlocked;
- expected input availability and price;
- recurring cost relative to treasury and income;
- the value of the resulting effects in that province/state;
- existing shortages and market access;
- the cost and disruption of switching;
- a cooldown and hysteresis to prevent method-flipping.

AI should not select the newest method automatically. It should modernize when
the expected benefit is affordable and reasonably supportable.

## 11. UI requirements reserved before overhaul

The new Foundry interface must reserve reusable patterns for:

- maintenance fulfillment percentage and trend;
- compact required-good cards;
- current operating-method selector;
- locked-method explanations;
- full-supply versus current effective modifiers;
- payer/owner identification;
- estimated recurring daily cost;
- shortage and deferred-maintenance alerts;
- tooltip explanations of what is missing and why.

Likely consumers are the Province/Urban Center interface, States and
Infrastructure modules, Projects, Budget, Markets, and relevant tooltips. These
are production UI requirements, not a reason to keep the temporary standalone
Urban Center laboratory window.

## 12. Construction interaction

Construction remains a separate two-stage process:

1. required construction goods are stockpiled;
2. the project occupies construction capacity for its build time.

Maintenance begins only when the new level becomes operational. A completed
building without adequate maintenance may start below full effectiveness; it
does not retroactively consume maintenance during construction.

## 13. Implementation sequence

1. Add data-driven maintenance recipes and cached state demand totals.
2. Apply recurring government/private payments and fulfillment scaling.
3. Implement Urban Center operating methods as the first selectable method.
4. Add AI evaluation with affordability, supply confidence, cooldowns, and
   local-player exclusion.
5. Expose the reserved UI fields in the redesigned province and module windows.
6. Extend coverage to roads, railways, forts, naval bases, and administration.
7. Balance scenario starting methods and production capacity.
8. Consider condition/deferred maintenance only after the core model is stable.

## 14. Non-goals for the first implementation

- Per-building daily route searches.
- Instant destruction from a temporary shortage.
- Automatic forced modernization for the player.
- A unique bespoke window for every building type.
- Final commodity recipes before the expanded resource list is settled.

