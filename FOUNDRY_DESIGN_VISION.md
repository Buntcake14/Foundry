FOUNDRY — RISE OF NATIONS
CODEX PROJECT HANDOFF / CORE DESIGN CONTEXT

============================================================
1. PROJECT OVERVIEW
============================================================

Foundry — Rise of Nations is being built from Project Alice, the
open-source Victoria II engine recreation.

IMPORTANT CURRENT DEVELOPMENT PHASE:

We are currently working toward a CLEAN BASELINE that behaves as
closely as practical to base/vanilla Victoria II.

Project Alice contains its own gameplay changes and additions. Those
are being removed/reverted where necessary before Foundry systems are
layered on top.

DO NOT prematurely implement the large systems described below unless
specifically instructed.

For now:

1. Restore/preserve vanilla Victoria II behavior.
2. Understand how Project Alice implements that behavior.
3. Avoid unnecessary rewrites.
4. Build future systems modularly on top of the baseline.
5. Preserve extensibility because we control the source code and are
   intentionally going beyond vanilla engine limitations.

The Foundry philosophy is NOT "make a Victoria II mod within vanilla
limitations."

It is:

"Use Victoria II as the simulation foundation and extend the engine
where necessary to build the game we actually want."


============================================================
2. CORE ALTERNATE-HISTORY PREMISE
============================================================

Target start date:

~1760

This replaces the earlier 1790 concept.

The world should NOT resemble historical 1760 geopolitically.

Europe never consolidated into the familiar giant nation-states.

Think of the fragmented Holy Roman Empire/German states as inspiration
for much of Europe.

Regions that historically became:

- France
- Great Britain
- Germany
- Italy
- etc.

instead contain numerous smaller states, principalities, kingdoms,
republics and regional identities.

These states can potentially consolidate during gameplay.

Historical countries are POSSIBLE outcomes, not predetermined outcomes.

France might form.

A different French state might form.

Several competing French identities might survive.

France might never form.

The same philosophy applies elsewhere.

The player should watch national identities, states and empires emerge
from simulation rather than replaying a predetermined historical map.


============================================================
3. THE NEW WORLD / COLONIAL PREMISE
============================================================

Because Europe remained politically fragmented, European powers had
less ability to finance and sustain enormous overseas colonial systems.

Therefore the New World is much less comprehensively colonized at the
1760 start.

There may be coastal colonies, trading settlements, enclaves and
regional possessions, but enormous portions of the Americas remain
controlled by indigenous peoples or otherwise outside European control.

This is intentional.

A major design goal is allowing radically different powers to emerge.

Examples:

- Indigenous American states becoming major powers.
- Indigenous states urbanizing and industrializing.
- Morocco establishing colonies in Brazil.
- Unexpected European microstates becoming maritime powers.
- Historical colonial powers never emerging.
- Completely different global Great Powers forming.

Two campaigns should be capable of producing dramatically different
world histories.


============================================================
4. WHY THE GAME STARTS AROUND 1760
============================================================

1760 allows the player to participate in creating the CONDITIONS for
industrialization instead of entering the game after industrialization
has already begun.

The long-term campaign transformation should roughly resemble:

1760
Agrarian / workshop / sailing-ship world

↓

Proto-industrialization
Urban growth
Political consolidation

↓

Mechanization
Steam
Factories
Railways

↓

Mass industrialization
Steel
Electricity
Chemicals
Telecommunications

↓

Modern industry
Oil
Automobiles
Aircraft
Radio
Mass production

↓

Potential post-1936 continuation

The campaign may eventually extend toward approximately 1960 or beyond.

The architecture should NOT assume 1936 is necessarily the permanent
end date.


============================================================
5. 1760 IS NOT "PRIMITIVE"
============================================================

Do not model the starting world like a medieval economy.

By 1760 we already have:

- mature gunpowder warfare
- muskets
- artillery
- cavalry
- fortifications
- professional armies
- sailing navies
- naval artillery
- global maritime trade
- commercial agriculture
- banking/finance
- bureaucratic states
- manufactories
- universities
- scientific institutions
- substantial cities

The important distinction is that INDUSTRIAL MECHANIZATION has not yet
transformed society.

This should still fundamentally feel like a Victoria-style simulation,
not Europa Universalis.


============================================================
6. ECONOMIC TRANSFORMATION
============================================================

A major Foundry pillar is:

ARTISANS
    ↓
WORKSHOPS / MANUFACTORIES
    ↓
MECHANIZED FACTORIES
    ↓
MASS INDUSTRY
    ↓
MODERN INDUSTRY

Workshops are especially important.

They should represent organized pre-industrial / proto-industrial
production capable of producing goods at larger scale than individual
artisans without having the productivity of later factories.

Examples could include:

- textile manufactories
- ironworks
- cannon foundries
- armories/gunsmiths
- sawmills
- furniture workshops
- breweries/distilleries
- paper mills
- pottery
- tanneries
- shipyards

Artillery absolutely exists in 1760.

Do not structure the economy as if artillery or sophisticated metal
goods only appear during industrialization.

Where vanilla Victoria II requires later industrial goods for certain
production chains, Foundry may use earlier production methods with:

- earlier input goods
- lower efficiency
- lower output
- higher labor requirements

Later technologies then improve HOW goods are produced rather than
necessarily inventing every good from scratch.


============================================================
7. PRODUCTION METHODS / TECHNOLOGY PHILOSOPHY
============================================================

Technology should increasingly unlock:

- capabilities
- production methods
- power sources
- institutions
- organizational structures
- transportation systems
- new methods of making existing goods

rather than primarily:

"+5% output"

Example:

An iron-related good might initially be produced using:

manual labor + charcoal + iron

Later:

water-powered machinery

Later:

coal-powered machinery

Later:

steam-powered machinery

Later:

modern industrial processes

The good can remain conceptually similar while the production system
evolves.


============================================================
8. BROAD TECHNOLOGICAL ERAS
============================================================

These are design scaffolds, NOT rigid hard-date switches.

~1760–1790
ENLIGHTENMENT / MANUFACTORY ERA

- agricultural improvements
- administration
- commerce
- navigation
- metallurgy
- workshops/manufactories
- manual/animal/water power

~1790–1820
AGE OF REVOLUTION / PROTO-INDUSTRIAL TRANSITION

- political thought
- military organization
- early mechanization
- nationalism/state formation

~1820–1870
STEAM / MECHANIZED INDUSTRIAL ERA

- steam power
- factories
- coal
- machine production
- railways
- steamships
- rapid urbanization

~1870–1920
SECOND INDUSTRIAL / MASS ERA

- steel
- chemicals
- electricity
- telecommunications
- mass production

~1920–1960+

- oil
- automobiles
- aviation
- radio
- assembly production
- increasingly modern industrial systems

Leave architecture extensible beyond this.


============================================================
9. URBAN CENTER SYSTEM
============================================================

Foundry should distinguish rural provinces from meaningful urban
development.

A rural province should generally need an URBAN CENTER before advanced
urban buildings can be constructed.

Conceptually:

RURAL PROVINCE
      ↓
ESTABLISH URBAN CENTER
      ↓
URBAN CAPACITY
      ↓
WORKSHOPS
ADMINISTRATION
SOCIAL BUILDINGS
INFRASTRUCTURE
      ↓
POPULATION / INFRASTRUCTURE GROWTH
      ↓
EXPAND URBAN CENTER
      ↓
MORE CAPACITY


============================================================
10. URBAN CAPACITY
============================================================

Urban Centers provide finite URBAN CAPACITY.

Buildings consume capacity.

Examples:

- workshops
- factories
- government offices
- administrative buildings
- social/institutional buildings
- infrastructure-related urban buildings

This prevents infinite urban development simply because the player has
money.

Urban development becomes something the province must physically and
socially support.

CORE RULE:

"Buildings create capacity. POPs make that capacity function."

An administrative office without Bureaucrats should not provide its
full effect.

A workshop without its workforce should not operate properly.

A factory without Craftsmen should be underutilized.


============================================================
11. NEW PROTO-INDUSTRIAL POP CONCEPT
============================================================

Vanilla Craftsmen are designed around factory employment.

Foundry should have an earlier urban/proto-industrial worker POP
concept, currently referred to as:

TRADESMEN

Conceptually:

Farmers / Laborers
        ↓
Urban migration
        ↓
Tradesmen
        ↓
Workshop employment
        ↓
Mechanization / industrialization
        ↓
Craftsmen
        ↓
Factory employment

Tradesmen should therefore help bridge the economic and demographic
transition from an agrarian society to an industrial society.

Urban environments should encourage formation/migration of this POP.


============================================================
12. URBAN ADMINISTRATION
============================================================

New administrative/government buildings should exist within Urban
Centers alongside workshops/factories.

These can provide state capacity / administrative effects.

Bureaucrat POPs should staff these buildings.

The building itself should not simply provide an unconditional bonus.

Again:

INFRASTRUCTURE PROVIDES POTENTIAL.
POPS MAKE IT FUNCTION.


============================================================
13. MILITARY — LONG-TERM DESIGN PILLAR
============================================================

DO NOT IMPLEMENT THIS DURING INITIAL VANILLA RESTORATION.

However, future military architecture should remain extensible enough
to support it.

Foundry warfare should itself evolve technologically.

Approximate progression:

1760:
REGIMENTS → FIELD ARMIES

19th century:
DIVISIONS → ARMIES

later:
DIVISIONS → CORPS → ARMIES

industrial warfare:
DIVISIONS → CORPS → ARMIES → FRONT COMMANDS

modern warfare:
HIGH COMMAND
    ↓
THEATER
    ↓
FRONT
    ↓
ARMY HQ
    ↓
CORPS HQ
    ↓
DIVISIONS

The inspiration for mature command structure is Hearts of Iron III.

IMPORTANT:

Do not replace actual units with an abstract Victoria 3-style front
system.

Divisions remain real selectable formations.


============================================================
14. OPTIONAL MILITARY AUTOMATION
============================================================

Core principle:

"AUTOMATION CHANGES WHO GIVES THE ORDERS, NOT WHETHER THE UNITS EXIST."

The player should eventually be able to operate military hierarchy at
different automation levels.

FULL MANUAL

Player directly controls individual formations.

ASSISTED

Player creates:

- defensive lines
- fallback lines
- offensive objectives
- advance axes
- etc.

Units execute the plan.

DELEGATED

Player assigns commanders and gives them high-level objectives.

AI commanders manage subordinate formations.

Different fronts can use different levels simultaneously.


============================================================
15. COMMANDERS AND PLAYER CONTROL
============================================================

A formation being attached to a Front does NOT automatically mean AI
control.

Important distinction:

FRONT ASSIGNMENT = organizational relationship.

COMMANDER ASSIGNMENT = permission for AI operational command.

If an Army/command has NO COMMANDER:

The game assumes the PLAYER is commanding it.

The player is responsible for moving the army into position.

The AI should NOT magically rail the formation to the front.

If a commander IS assigned:

The commander may:

- determine deployment
- use available rail routes
- move subordinate formations
- position divisions
- maintain reserves
- respond to attacks
- execute high-level orders

Command automation should follow the hierarchy.


============================================================
16. HOI3-STYLE ORDER OF BATTLE
============================================================

Target mature structure:

NATIONAL HIGH COMMAND
        ↓
THEATER COMMAND
        ↓
FRONT COMMAND
        ↓
ARMY HQ
        ↓
CORPS HQ
        ↓
DIVISIONS

Each level should eventually have actual gameplay meaning rather than
being a cosmetic folder.

Command ranges, communications, logistics and organizational capability
can affect how well these structures operate.


============================================================
17. STAFF SUBORDINATES
============================================================

A major usability feature:

Every appropriate command level should eventually have:

[ STAFF SUBORDINATES ]

Example:

National High Command commander can be told:

"Staff everyone beneath you."

The AI fills eligible vacant command positions through the subordinate
command tree.

The player may instead manually appoint officers wherever desired.

Potential commander-slot states:

PLAYER CONTROL
- intentionally no commander
- do not auto-fill

VACANT / AUTO-STAFF ELIGIBLE
- may be filled by superior commander

ASSIGNED
- named commander currently occupies post

Potential officer lock:

LOCKED APPOINTMENT
- player-selected officer cannot be moved by automated reorganization

Possible actions:

FILL VACANCIES
- safely fills eligible empty positions

REORGANIZE COMMAND
- superior commander may reshuffle unlocked subordinate officers

AI staffing should eventually consider commander traits and suitability,
not randomly assign officers.


============================================================
18. COMMANDER QUALITY
============================================================

Generals should eventually be more than flat stat modifiers.

Possible behaviors/traits include:

- aggressiveness
- defensive tendency
- reserve management
- logistics ability
- force concentration
- artillery coordination
- withdrawal judgment
- risk tolerance
- breakthrough/exploitation ability

Different command levels can emphasize different abilities.

Delegating a front therefore reduces micromanagement but introduces a
real tradeoff:

You are trusting that officer to run the war.


============================================================
19. MODERN WARFARE SHOULD NOT START ON JANUARY 1, 1900
============================================================

There should NOT be:

if year >= 1900:
    enable_ww1_warfare()

Modern military organization should be CAPABILITY DRIVEN.

Possible requirements:

- divisional organization
- permanent general staff
- mass mobilization
- railway logistics
- telegraph/communications
- modern operational doctrine
- sufficient officers
- sufficient administrative/command capacity

Therefore in 1910 one country might have modern Front Warfare while
another still operates older field armies.

This asymmetry is intentional.


============================================================
20. MILITARY MODERNIZATION EVENT/UI
============================================================

When a country becomes capable of modern Front Warfare, present the
player with a clear notification/tutorial.

Conceptually:

"THE AGE OF FRONT WARFARE"

Explain that the country's military can now establish Front Commands,
modern hierarchy, continuous defensive lines, reserves, etc.

Do NOT force immediate conversion.

The player may:

- reorganize now
- reorganize later
- continue using traditional formations

A modernization/readiness indicator may eventually show missing
requirements before the unlock.


============================================================
21. MILITARY CONTROL ZONES
============================================================

This is currently a DESIGN DIRECTION requiring prototyping.

Do not treat exact counts/math as finalized.

The underlying concept:

Victoria II provinces remain the fundamental:

- political
- economic
- POP
- RGO
- ownership

units.

However, provinces gain a lighter MILITARY CONTROL layer underneath
them.

A province can contain several military control zones.

Example conceptual maximum:

[1][2][3]
[4][5][6]

These are NOT six new full Victoria II provinces.

They are lightweight military geography.


============================================================
22. COMMAND RESOLUTION / FRONTAGE
============================================================

The latest preferred concept is that every province has fine-grained
underlying military control zones, but military organization determines
how finely a country can COMMAND them.

Conceptually:

LEVEL 1 — FIELD ARMY

Player effectively sees/commands the province as one military area.

[ 1+2+3+4+5+6 ]

Order:
"Capture Province."


LEVEL 2 — DIVISIONAL WARFARE

The same underlying zones are grouped into approximately two command
areas.

[   AREA A   ][   AREA B   ]

Order:
"Capture Area A."


LEVEL 3 — CORPS WARFARE

The country receives finer command resolution, perhaps approximately
four operational areas.


LEVEL 4 — MODERN FRONT WARFARE

Full underlying control-zone resolution becomes directly commandable.

[1][2][3]
[4][5][6]

IMPORTANT:

Exact number and geometry of zones is NOT finalized.

Real geography should eventually determine shapes/adjacency.

Small provinces may need fewer.

Large provinces may need more.

This needs prototyping.


============================================================
23. THINK OF THIS LIKE FRONTAGE RESOLUTION
============================================================

A useful conceptual analogy is combat frontage.

A technologically advanced military does not automatically receive
"more battlefield."

Instead:

IT CAN DIVIDE THE SAME FRONTAGE INTO SMALLER INDEPENDENTLY COMMANDABLE
PIECES.

Example:

A Level 2 military might only coordinate two major sections of a front.

A Level 4 military might coordinate three separate active sectors along
that same physical border.

The advantage is precision, not a magic combat bonus.

Modern organization allows:

- better distribution
- concentration
- reserves
- local reinforcement
- exploitation of weak sectors
- coordinated attacks
- finer defensive deployment


============================================================
24. OLD VS MODERN ARMIES
============================================================

Both armies fight on the SAME underlying military geography.

Example:

Underlying province:

[1][2][3]
[4][5][6]

Level 2 country sees:

[   A   ][   B   ]

where Area A and Area B each correspond to several underlying zones.

If Level 2 orders:

"CAPTURE AREA A"

the engine determines which underlying zones must be taken.

The operation may contain several local engagements.

If Area A consists of zones 1, 2 and 4:

CAPTURE AREA A
    ↓
Fight Zone 4
Fight Zone 1
Fight Zone 2
    ↓
AREA A CAPTURED

The Level 2 player sees:

AREA A — OCCUPIED

The Level 4 opponent sees:

ZONE 1 — LOST
ZONE 2 — LOST
ZONE 4 — LOST

Same battlefield state.

Different command resolution.


============================================================
25. HOW LOWER-LEVEL ARMIES ATTACK FINER ZONES
============================================================

When a lower-resolution army attacks a larger command area, the engine
chooses legal underlying entry zones according to:

1. physical adjacency
2. coarse objective
3. terrain
4. enemy strength
5. roads/rail
6. rivers
7. fortifications
8. supply
9. commander quality/doctrine

The lower-level player commands the LARGE objective.

The military simulation handles tactical choices below that player's
available command resolution.

Potential lower-level intent options could eventually include:

- direct attack
- left flank
- right flank
- cautious advance

Higher military organization progressively exposes more direct control.


============================================================
26. COMBAT WIDTH VS COMMAND RESOLUTION
============================================================

Keep these concepts separate.

COMMAND / FRONTAGE RESOLUTION:

How many portions of the battlefield can the military independently
coordinate?

COMBAT WIDTH:

How many troops can effectively fight within a given active portion of
the battlefield?

Example:

Plains sector:
Combat width = large

Mountain sector:
Combat width = small

Therefore an advanced army can have excellent command resolution but
still cannot cram unlimited troops through a mountain pass.


============================================================
27. TERRAIN / STRATEGIC GEOGRAPHY
============================================================

Control zones could eventually represent meaningful military terrain:

- city
- hills
- forest
- river crossing
- plains
- rail junction
- port
- mountain pass
- etc.

This would allow strategic objectives within provinces.

Example:

Capturing the rail-junction zone may disrupt supply even before the
whole province falls.

Capturing an Urban Center zone may occupy factories/government
infrastructure while countryside remains contested.

Again, this requires prototyping before final implementation.


============================================================
28. PARTIAL OCCUPATION
============================================================

Political ownership remains province-based.

Military control can become zone-based.

Example:

Province X
Legal owner: Burgundy

Military control:
Burgundy 67%
Enemy 33%

Potentially:

Urban Center: Burgundy control
Rail junction: Enemy control
Resource region: mixed

This may eventually affect:

- supply
- RGO production
- factories
- market access
- rail movement
- occupation effects

Do not implement these effects until their design is finalized.


============================================================
29. LOGISTICS
============================================================

Modern warfare should increasingly connect directly to Foundry's
economy.

Early armies might consume primarily:

- manpower
- food
- small arms
- artillery

Industrial armies increasingly require:

- ammunition
- artillery shells
- uniforms/supplies
- machine guns/heavy weapons
- fuel
- vehicles
- aircraft
- radios
- etc.

Exact goods remain TBD.

Railroads should eventually become real strategic infrastructure rather
than merely province modifiers.

Potential supply chain:

FACTORY
   ↓
NATIONAL TRANSPORT NETWORK
   ↓
RAIL HUB
   ↓
DEPOT
   ↓
ARMY / FRONT

Rail networks should have throughput/capacity.

Moving 800,000 men should not be instantaneous simply because a railroad
exists.

Commanders assigned to armies/fronts should eventually be capable of
choosing efficient rail routes and deployment locations.


============================================================
30. PERFORMANCE PHILOSOPHY
============================================================

Do NOT simulate individual soldiers.

Use:

- formations
- divisions
- corps
- armies
- fronts
- sectors

Detailed combat processing should primarily occur around active
engagements/fronts.

Quiet fronts should be represented with relatively inexpensive state:

- strength
- organization
- entrenchment
- supply
- frontage
- reconnaissance
- etc.

Modern hardware gives us more room than original Victoria II had, but
performance still requires sensible architecture.


============================================================
31. CRITICAL MILITARY ARCHITECTURE RULE
============================================================

DO NOT BUILD TWO COMPLETELY SEPARATE COMBAT ENGINES.

The desired conceptual architecture is:

ONE UNDERLYING MILITARY SIMULATION

with progressively more sophisticated COMMAND LAYERS exposed through
technology/institutions.

That allows:

1760 field army
vs
1910 modern front army

to fight without switching between incompatible systems.

Modern military technology gives finer command/control over the same
underlying battlefield.


============================================================
32. MILITARY PROTOTYPE BEFORE FULL IMPLEMENTATION
============================================================

Before implementing control zones globally, create a small isolated
prototype/test scenario.

Ideal test:

Two countries
~4–6 provinces
Several armies

Debug toggle allowing military command levels:

Level 1
Level 2
Level 3
Level 4

Test:

L1 vs L1
L1 vs L4
L2 vs L4
L3 vs L4
L4 vs L4

We should SEE this operating on the actual Foundry map before finalizing
combat math.

The design philosophy is established.

Exact zone splitting, combat calculations and UX should remain flexible
until prototyped.


============================================================
33. UI / ART WORKFLOW
============================================================

ChatGPT will be heavily used for Foundry's visual/UI design.

Claude Code and Codex should NOT independently redesign approved UI art.

Preferred workflow:

1. Design feature with ChatGPT.
2. Generate/mock up UI visually.
3. Iterate until approved.
4. Produce exact PNG assets.
5. Produce exact dimensions/layout specification.
6. Give assets/spec to coding agent.
7. Coding agent inspects actual Project Alice implementation.
8. Implement using real engine conventions.
9. Run in game.
10. Screenshot result.
11. Return screenshot to ChatGPT for visual refinement.
12. Repeat.

Coding agents should treat approved visual assets as source-of-truth
unless explicitly instructed otherwise.


============================================================
34. UI ART FORMAT
============================================================

For custom Foundry UI work we have already established that PNG works
reliably.

Use:

32-bit PNG with alpha transparency

where transparency is needed.

There is known concern in this project around DX10-compressed DDS UI
textures silently failing, whereas PNG has been confirmed working for
the relevant UI use cases.

Do not arbitrarily convert approved PNG UI assets to DDS unless the
specific implementation requires it and has been tested.


============================================================
35. STATE LEDGER UI FEATURE
============================================================

A custom State Ledger topbar window has already been designed.

Purpose:

Display every state owned by the player, one row per state.

At-a-glance information:

- State name
- Life-needs satisfaction
- Market balance
- RGO utilization
- View/action

The View button opens/navigates toward more detailed state/resource
information.

Original target window:

800 × 600

Listbox:

760 × 520
at approximately 20,60

Rows:

760 × 28

Five approximately equal columns:

State
Life Needs
Market
RGO Use
View

The window art has been iterated visually.

Important:

The close X/button is dynamically placed by the engine and should NOT
be baked into the window background art.

The "Your States" heading graphic belongs in the window art/design.

Topbar icon and heading were deliberately scaled down after initial
iterations because they appeared too large in game.


============================================================
36. STATE RESOURCE GRID
============================================================

The State Ledger View action should eventually open a state-specific
resource/production window.

This window displays resources in a grid.

Each resource entry should show:

- resource icon
- resource name
- amount produced in the state
- demand for that resource in the state

This is part of the larger goal of making state economic information
more visible and understandable.


============================================================
37. CODEX + CLAUDE WORKFLOW
============================================================

Claude Code is currently doing significant Project Alice / vanilla
baseline work.

Codex may be used alongside Claude.

Possible division:

CLAUDE:
- major ongoing engine work
- baseline restoration
- systems it already understands/contextualizes

CODEX:
- independent review
- isolated implementations
- UI integration
- debugging
- architecture verification
- source inspection
- implementation from approved visual specs

CHATGPT:
- game design
- system architecture discussion
- UI/UX design
- image generation
- visual assets
- implementation specifications
- review of screenshots/results

USER:
- game director / final design authority

Avoid Claude and Codex simultaneously modifying the same files unless
deliberately using separate branches/worktrees.


============================================================
38. DEVELOPMENT PHILOSOPHY
============================================================

Several rules should guide Foundry development.

RULE 1:
Restore a clean Victoria II baseline first.

RULE 2:
Do not chase every Project Alice design choice and build Foundry around
it accidentally.

RULE 3:
Do not remain constrained by Victoria II engine limitations simply
because Victoria II had them.

RULE 4:
Prefer systems that produce emergent history over scripted historical
outcomes.

RULE 5:
Technology should change what societies CAN DO, not merely add bigger
numbers.

RULE 6:
Infrastructure provides capability; POPs make it function.

RULE 7:
Complex simulation does not require mandatory micromanagement.
Delegation/automation should allow players to choose their desired
level of control.

RULE 8:
Do not remove underlying simulation merely to make automation easier.

RULE 9:
Prototype extremely ambitious systems in isolation before committing
them to the global simulation.

RULE 10:
Do not implement speculative details as permanent architecture simply
because they appear in this document. Separate CORE PRINCIPLES from
TBD IMPLEMENTATION DETAILS.


============================================================
39. CURRENT PRIORITY
============================================================

The immediate priority remains:

CLEAN PROJECT ALICE → VANILLA VICTORIA II BASELINE

Do NOT begin implementing:

- Front Warfare
- control zones
- modern OOB
- 1960 technology
- full Urban Capacity
- every workshop chain
- etc.

simply because they are described here.

They are the architectural destination.

When modifying baseline systems today, however, avoid unnecessary
decisions that would make these future extensions impossible or require
a complete rewrite.

Once the baseline is stable, Foundry systems can be scaffolded
deliberately and incrementally.


============================================================
40. THE CORE FOUNDRY VISION
============================================================

Foundry should simulate approximately two centuries of transformation.

The player begins around 1760 in a world of:

- fragmented states
- contested identities
- predominantly agrarian economies
- workshops/manufactories
- sailing ships
- field armies
- limited administrative reach
- limited transportation
- large areas outside European colonial control

The simulation can eventually produce:

- consolidated nations OR continued fragmentation
- unexpected empires
- powerful indigenous states
- alternative colonial systems
- industrial cities
- mass factories
- rail networks
- electricity
- automobiles
- aircraft
- modern administration
- mass armies
- continuous fronts
- mechanized warfare

None of those political outcomes should be predetermined.

The guiding idea is:

THE PLAYER IS NOT PLAYING THROUGH A PREDETERMINED VICTORIAN WORLD.

THE SIMULATION CREATES WHATEVER VICTORIAN — AND EVENTUALLY MODERN —
WORLD EMERGES FROM THE CONDITIONS OF 1760.