# Foundry — GUI Toolkit & UI Design Standard

## Status

**Core UI design specification**

This document defines the visual language, reusable components, layout principles, and implementation expectations for the Foundry user interface.

Coding agents should reference this document before creating or substantially modifying Foundry UI.

The goal is not simply to reskin Victoria II.

The goal is to create a coherent Foundry interface capable of supporting both vanilla Victoria II systems and Foundry's expanded mechanics without new controls appearing bolted onto old layouts.

---

# 1. Core Design Philosophy

Foundry retains the character of a classic Victoria-era grand strategy interface while making information substantially easier to read and navigate.

The desired visual identity is:

- Victorian
- Burgundy / dark red
- Antique gold
- Warm parchment
- Dark wood / near-black backgrounds
- Ornamental without becoming excessively decorative
- Information dense
- Strong visual hierarchy
- Consistent across every game system

The interface should feel like an evolution of Victoria II rather than a modern flat UI.

However, Foundry should NOT preserve vanilla layouts merely for familiarity.

When Foundry adds mechanics that no longer fit naturally inside a vanilla window, redesign the window around the complete feature set.

### Core principle

> New mechanics should have intentional homes in the interface rather than being placed wherever unused space happens to exist.

---

# 2. UI Architecture

Foundry UI should be built from reusable components.

Do NOT create every window as a unique collection of unrelated artwork.

The toolkit should provide common components for:

- Window frames
- Title bars
- Inner panels
- Section panels
- Buttons
- Icon buttons
- Tabs
- Tables
- Listboxes
- Scrollbars
- Sliders
- Progress bars
- Dropdowns
- Checkboxes
- Radio buttons
- Status indicators
- Tooltips
- Notifications
- Dividers
- Resource cards
- Headers
- Footer/action bars

New windows should assemble these components wherever practical.

---

# 3. Color Language

The Foundry interface uses a restrained Victorian palette.

## Primary Burgundy

Used for:

- Window title bars
- Major buttons
- Selected navigation
- Section headers
- Major interface framing

Approximate family:

`#7A1E2B`

Exact production colors should be sampled/finalized from approved assets rather than assuming this provisional value is authoritative.

---

## Dark Burgundy

Used for:

- Deep backgrounds
- Recessed panels
- Secondary navigation
- Shadowed interface regions

Approximate family:

`#5B111B`

---

## Antique Gold

Used for:

- Borders
- Icons
- Important separators
- Decorative trim
- Selected states
- Major headings

Approximate family:

`#C9A24D`

Gold should communicate interface hierarchy.

Do not cover the entire UI in gold.

---

## Parchment

Used for:

- Main information surfaces
- Tables
- Lists
- Data panels
- Reading areas

The parchment should remain light enough for dark text to be highly legible.

Avoid excessive texture behind dense numerical information.

---

## Dark Background

Used behind windows and in recessed interface areas.

This should generally be a very dark brown/burgundy rather than pure black.

---

# 4. Typography

The interface should use three broad typographic roles.

## Title Typeface

Used for:

- Major window titles
- Major category headings

Desired character:

Classical/Victorian serif.

The toolkit concept used Trajan-style typography as visual inspiration.

Do not introduce external font dependencies without verifying licensing and engine support.

---

## Header Typeface

Used for:

- Section headings
- Tabs
- Buttons
- Table headers

Should be highly readable at relatively small sizes.

---

## Body Typeface

Used for:

- Tables
- Tooltips
- Descriptions
- Statistics
- Lists

Readability takes priority over decoration.

Dense simulation information should never use elaborate display lettering.

---

# 5. Icon Language

Foundry uses two visually distinct icon families.

## Interface Icons

Interface/action icons should generally be:

- Antique gold
- Strong silhouette
- Transparent background
- Readable at small size
- Consistent lighting and line weight

Examples:

- Economy
- Treasury
- Technology
- Government
- Population
- Markets
- Diplomacy
- Military
- States
- Port
- Railway
- Industry
- Agriculture
- Research
- Reports
- Alerts

These icons represent systems or actions.

---

## Commodity / Resource Icons

Actual goods/resources should remain colorful.

Examples:

- Iron
- Coal
- Grain
- Cotton
- Timber
- Artillery
- Fabric
- Machinery

### Important distinction

> GOLD ICON = interface/system/action

> COLORFUL ICON = physical good/resource

Do not unnecessarily convert commodity icons into monochrome gold UI symbols.

The resource icon library should eventually be visually updated to match Foundry while preserving rapid commodity recognition.

---

# 6. Standard Button System

Buttons require consistent interaction states.

Every major reusable button should support:

1. Default
2. Hover
3. Pressed
4. Disabled

Where appropriate:

5. Alert
6. Selected / active

---

## Primary Button

Appearance:

- Burgundy surface
- Antique-gold border
- Light/gold text

Used for major actions.

Examples:

- Confirm
- Build
- Expand
- Manage
- Open State Ledger

---

## Secondary Button

Appearance:

- Dark brown / recessed surface
- Gold or parchment text
- Less visual weight than primary buttons

Used for:

- Cancel
- Secondary navigation
- Minor actions

---

## Icon Button

Small square controls containing a single clear symbol.

Examples:

- Close
- Add
- Remove
- Search
- Move up/down
- Reports
- Map modes

Icons should remain readable at their actual in-game size.

---

# 7. Tabs

Tabs are the preferred solution for grouping related systems inside one major window.

Each tab requires:

- Active state
- Inactive state
- Hover state where practical

The active tab should visually merge into the content panel.

Example:

Economy

`Overview | Production | Industries | RGOs | Infrastructure`

Markets

`Overview | Goods | Prices | Trade Routes | Transport`

States

`Overview | State Ledger | Urban Centers | Infrastructure | Administration`

Tabs should reduce the temptation to create excessive permanent top-level buttons.

---

# 8. Window Frames

Major windows should share a common structural language.

Typical structure:

    TITLE BAR
    TAB BAR
    MAIN CONTENT
    CONTEXT / ACTION AREA

Window characteristics:

- Burgundy title bar
- Antique-gold outer trim
- Parchment information surface
- Subtle corner ornamentation
- Strong but thin section separators

The frame should look finished without overwhelming the information.

---

# 9. Title Bars

Title bars should contain:

- Window title
- Optional system icon
- Dynamically positioned close control

### Important implementation rule

Do NOT bake dynamic buttons such as the close `X` directly into background artwork when Project Alice places those controls dynamically.

Background artwork and interactive controls should remain separate whenever possible.

---

# 10. Inner Panels

Use inner panels to create hierarchy without creating dozens of separate windows.

Examples:

    MARKET OVERVIEW

    ┌─────────────────┐
    │ Market Balance  │
    │ +£420           │
    └─────────────────┘

    ┌─────────────────┐
    │ Top Imports     │
    │ ...             │
    └─────────────────┘

Panel styles may include:

- Standard information panel
- List panel
- Section panel
- Highlight/summary panel

Use consistent corner radius/ornamentation and borders.

---

# 11. Tables

Tables are a major Foundry information component.

They should support:

- Header row
- Normal row
- Alternate row where useful
- Hover row
- Selected row
- Disabled row
- Positive values
- Negative values
- Sortable columns where appropriate

Example:

    GOOD       PRICE     TREND

    Iron       £18.6      ↓
    Coal        £9.2      ↓
    Timber      £6.8      ↑

Tables should prioritize numerical alignment and readability.

---

# 12. Lists

Listboxes should visually belong to the same family as tables.

Typical components:

- Burgundy header
- Parchment rows
- Thin separators
- Gold/dark scrollbar
- Optional row icon
- Optional status marker

List backgrounds should support dynamic content rather than having text baked into artwork.

---

# 13. Sliders

Foundry sliders should use:

- Dark recessed track
- Burgundy/gold active region
- Gold handle
- Clearly readable value

Support:

- Standard slider
- Two-sided slider
- Budget-style sliders
- Range controls where necessary

The interaction target may be larger than the visible decorative handle if required for usability.

---

# 14. Progress Bars

Progress bars should communicate state at a glance.

Potential uses:

- Research
- Administrative reach
- Urban capacity
- Military readiness
- Supply
- Construction
- Reform progress
- Literacy/education progress where appropriate

The bar should remain legible even when small.

Do not rely exclusively on color.

Always provide a number/percentage or tooltip when the value matters.

---

# 15. Dropdowns & Selectors

Dropdowns should consist of:

- Burgundy/dark closed control
- Gold border
- Clear arrow
- Parchment expanded list
- Highlighted hovered/selected row

Used for choices where permanent buttons would create clutter.

---

# 16. Toggle Controls

Supported patterns:

- Active / inactive button
- Checkbox
- Radio button

Use:

Checkbox = independent option.

Radio = one selection from a mutually exclusive set.

Toggle button = frequently changed mode/state.

Do not use different control types arbitrarily for the same interaction.

---

# 17. Status Indicators

Status indicators should have standardized semantic meaning.

## Positive

Green.

Examples:

- Growing
- Profitable
- Improving
- Surplus

## Negative

Red.

Examples:

- Declining
- Deficit
- Losing
- Shortage

## Neutral

Gold/cream.

Examples:

- Stable
- No change
- Balanced

## Warning

Amber/orange.

Something needs attention.

## Critical

Red alert.

Immediate/significant problem.

The same severity colors should mean the same thing everywhere.

---

# 18. Notifications

Notifications should use the same design language as windows.

Structure:

    [ICON] Notification Title
           Short explanation
                               [X]

Potential categories:

- Information
- Economic
- Diplomatic
- Military
- Political
- Critical alert

Example:

    New Trade Agreement

    We have signed a trade agreement
    with Burgundy.

Avoid excessively large notifications for routine events.

---

# 19. Tooltips

Tooltips should be useful simulation explanations, not merely labels.

Standard structure:

    [ICON] TITLE

    Short explanation.

    Relevant numerical information.

    Why this is happening / what affects it.

Example:

    TRANSPORT CAPACITY LOW

    The regional transport network is
    operating near its available capacity.

    Used: 92%

    Consider expanding railways, roads,
    ports, or other transport infrastructure.

Tooltips are especially important for Foundry's new simulation systems.

---

# 20. Information Hierarchy

Foundry should make important information readable at a glance.

Recommended pattern:

**Primary value**

Large or emphasized.

Example:

`£18.42K`

**Change**

Smaller contextual value.

Example:

`+£84.1/day`

**Capacity**

Used / total.

Example:

`210 / 210`

**Progress**

Value plus visual bar.

Example:

`72%`

**Status**

Text or icon.

Example:

`At Peace`

Do not show every available statistic simultaneously.

Use tabs and tooltips for deeper information.

---

# 21. Foundry Main Topbar

The main navigation should evolve vanilla Victoria II's topbar rather than merely reskinning it.

Current planned high-level modules:

1. Economy
2. Treasury
3. Technology
4. Government
5. Population
6. Markets
7. Diplomacy
8. Military
9. States

The topbar should function as both:

**navigation**

and

**national dashboard**.

Each module may show one to three important live indicators.

---

# 22. Economy Module

Primary purpose:

Production and industrial organization.

Suggested tabs:

- Overview
- Production
- Industries
- RGOs
- Infrastructure

Overview may contain:

- Industrial score
- Factory count
- Workshop count
- RGO count
- Employment
- Idle capacity
- Top goods by output
- Key input prices
- Factory/workshop status

The Economy screen must support Foundry's eventual progression from:

Artisans → Workshops → Factories → Modern Industry.

---

# 23. Treasury Module

Suggested tabs:

- Overview
- Budget
- Taxes
- Loans
- Investments

Overview may contain:

- Daily balance
- Treasury
- National debt
- Credit rating
- Interest
- Major income categories
- Major expense categories
- Projected balance

This replaces/expands the vanilla Budget concept.

---

# 24. Technology Module

Suggested tabs:

- Research
- Inventions
- Culture
- Production Methods

May contain:

- Current research
- Research progress
- Available technologies
- Research priorities
- Technology effects
- Production-method unlocks

Technology should increasingly communicate new capabilities rather than only percentage modifiers.

---

# 25. Government Module

Suggested tabs:

- Overview
- Laws
- Institutions
- Reforms
- Movements

May contain:

- Government type
- Ruling party
- Election information
- Legitimacy
- Party support
- Laws
- Reform movements
- Institutional capacity

Foundry administrative mechanics can eventually integrate here.

---

# 26. Population Module

Suggested tabs:

- Overview
- POP Groups
- Needs
- Migration
- Culture

May contain:

- Total population
- Growth
- Literacy
- Militancy
- Consciousness
- POP composition
- Living standards
- Culture
- Migration
- Employment

The UI must eventually support Foundry's additional proto-industrial POP concepts such as Tradesmen.

---

# 27. Markets Module

Suggested tabs:

- Overview
- Goods
- Prices
- Trade Routes
- Transport

May contain:

- Market balance
- Import dependence
- Export share
- Transport capacity
- Top imports
- Top exports
- Prices
- Trends
- Active trade routes
- Route profitability
- Market access

This module is especially important for Foundry's future local-first market and transport system.

---

# 28. Diplomacy Module

Suggested tabs:

- Overview
- Relations
- Spheres / Influence
- Treaties
- Diplomatic Actions

The exact "Spheres" concept may change as Foundry diplomacy develops.

May contain:

- International standing
- Great Powers
- Current relations
- Influence
- Treaties
- Trade agreements
- Alliances
- Diplomatic actions

---

# 29. Military Module

Suggested initial tabs:

- Overview
- Armies
- Navy
- Logistics
- Military Doctrines

Later systems may add or transform tabs for:

- Command hierarchy
- Fronts
- Theaters
- Operations
- Mobilization

The interface must be extensible enough to support Foundry's eventual HOI3-inspired military hierarchy without replacing the entire navigation system.

Early-era military UI should remain appropriate for 1760 warfare.

Later-era capability should progressively expand the module.

---

# 30. States Module

This is a new high-level Foundry module.

Suggested tabs:

- Overview
- State Ledger
- Urban Centers
- Infrastructure
- Administration

May contain:

- State count
- Total population
- Urbanization
- Average prosperity
- State revenue
- State alerts
- Urban Center information
- Infrastructure
- Administrative capacity/reach

This module exists because states become substantially more important in Foundry than they are in vanilla Victoria II.

---

# 31. State Ledger

A custom State Ledger window has already been prototyped.

Purpose:

Display every owned state in a compact list.

Information includes:

- State
- Life-needs satisfaction
- Market balance
- RGO utilization
- View/action

The View action can lead into state-specific economic information.

The design should use the common Foundry table/list/button components rather than a one-off visual system.

---

# 32. State Resource Grid

The State Ledger can lead to a resource/production view.

Each resource card should support:

- Commodity icon
- Commodity name
- Amount produced
- State demand
- Surplus/deficit
- Potentially price/market information later

Commodity art should remain colorful while its surrounding UI uses the Foundry burgundy/gold/parchment language.

---

# 33. Province Window Philosophy

The province window should eventually be redesigned around Foundry's actual systems rather than continuing to attach additional buttons to the vanilla layout.

Potential structure:

    PROVINCE HEADER
    Name / State / Owner / Population / Quick Actions

    PROVINCE VISUAL AREA

    RGO | WORKFORCE | URBAN CENTER

    CONTEXT TABS

    Economy
    Population
    Administration
    Infrastructure
    Military

    CONTEXTUAL ACTIONS

Potential information:

### Economy
- RGO
- Production
- Prices
- Workshops
- Factories

### Population
- POP composition
- Employment
- Migration
- Needs

### Administration
- Bureaucrats
- Administrative reach
- Government buildings
- Tax collection

### Infrastructure
- Roads
- Rail
- Rivers
- Ports
- Transport capacity

### Military
- Fortifications
- Supply
- Depots
- Mobilization
- Eventually military control-zone information

This avoids "finding somewhere to put another button" whenever Foundry gains a mechanic.

---

# 34. Responsive Information Density

Foundry is a desktop grand-strategy game.

Information density is desirable.

However:

> Dense does not mean cluttered.

Use hierarchy:

Window
→ Tabs
→ Sections
→ Primary values
→ Secondary values
→ Tooltips

Avoid exposing every simulation value at the same visual level.

---

# 35. Dynamic Elements vs Artwork

This distinction is critical for Project Alice implementation.

### Artwork should provide:

- Frames
- Borders
- Panel surfaces
- Button surfaces
- Icons
- Decorative elements
- Dividers

### Engine/UI code should provide:

- Text
- Dynamic values
- Lists
- Tables
- Progress
- Tooltips
- Interactive states
- Dynamic buttons
- Conditional elements

Do not bake dynamic game information into background images.

---

# 36. Asset Strategy

Prefer reusable asset atlases/sheets where supported and practical.

Conceptual organization:

    gfx/
      foundry_ui/
        frames/
        buttons/
        tabs/
        controls/
        icons/
        status/
        notifications/
        resources/
        decorations/

Exact location should follow the actual Foundry/Project Alice asset architecture discovered in the repository.

Do not invent incompatible asset-loading conventions merely to match this document.

---

# 37. Asset Format

For Foundry custom UI assets, PNG has already been confirmed as useful/reliable in this project.

Preferred where appropriate:

**32-bit PNG with alpha transparency**

Do not automatically convert working UI assets to DDS.

There is known concern in this project around DX10-compressed DDS textures failing to load correctly in some UI circumstances.

Use the format appropriate to the actual tested Project Alice pipeline.

---

# 38. Standard Asset Sizes

Exact dimensions should be determined by actual in-game requirements.

However, consistency matters.

Examples from current design direction:

- Small icons: approximately 24×24
- Icon interaction area: approximately 28×28
- Close controls: approximately 24×24
- Standard row height: approximately 28–32 px
- Minimum common button height: approximately 32 px

These are guidelines, not immutable engine requirements.

Always test at actual gameplay resolution.

---

# 39. Visual Scaling

Do not judge UI assets solely from large generated concept sheets.

A component that looks beautiful at 4K may become unreadable in-game.

Every reusable component should be tested at its actual rendered size.

This is especially important for:

- Gold ornamentation
- Icons
- Small text
- Button borders
- Resource icons
- Status markers

Simplify artwork where necessary for small-scale readability.

---

# 40. Alerts

Do not make every problem visually urgent.

Suggested severity hierarchy:

### Normal
No intervention needed.

### Warning
Something deserves attention.

### Alert
Meaningful problem requiring likely action.

### Critical
Immediate or severe issue.

Use notifications sparingly enough that alerts retain meaning.

---

# 41. Cross-Window Consistency

The same interaction should look the same everywhere.

Examples:

A positive number should not be green in Markets but gold in Treasury.

A selected row should not use three unrelated visual treatments across different screens.

A Build button should use the same primary-button language in Province and State views.

A warning icon should have consistent semantic meaning.

Consistency is more important than making each window individually decorative.

---

# 42. UI Should Explain the Simulation

Foundry is adding systems beyond vanilla Victoria II.

The interface must help players understand WHY something is happening.

For important simulation values, provide tooltips explaining:

- What the value represents
- What increases it
- What decreases it
- What consequences it has
- Relevant current bottlenecks

Example:

Instead of:

    Transport: 72%

prefer:

    Transport Capacity: 72%

Tooltip:

    Regional transport networks are using
    72% of available freight capacity.

    Major users:
    Coal shipments       28%
    Industrial inputs   19%
    Food distribution   12%

This is especially important for new systems without established Victoria II player intuition.

---

# 43. Implementation Philosophy for Codex / Claude

When implementing a new UI feature:

1. Read this toolkit.
2. Inspect existing Project Alice UI architecture.
3. Identify reusable Foundry components.
4. Use approved visual assets where available.
5. Do not independently redesign approved artwork.
6. Keep dynamic information separate from artwork.
7. Implement using actual Alice conventions.
8. Test in game.
9. Capture screenshot.
10. Compare screenshot against approved design.
11. Iterate.

Do not attempt to reproduce visual mockups through arbitrary code-generated approximations when approved assets exist.

---

# 44. Recommended UI Development Workflow

Preferred pipeline:

    DESIGN SYSTEM
          ↓
    WINDOW MOCKUP
          ↓
    USER APPROVAL
          ↓
    ASSET EXTRACTION / CREATION
          ↓
    IMPLEMENTATION SPEC
          ↓
    CODEX / CLAUDE IMPLEMENTATION
          ↓
    IN-GAME SCREENSHOT
          ↓
    VISUAL REVIEW
          ↓
    ITERATION

The in-game screenshot is the final truth.

Generated concept art is a target, not proof that the interface works.

---

# 45. Reuse Before Expansion

Before creating a new UI component, determine whether an existing toolkit component can handle the requirement.

Prefer:

    Existing Button + New Label

over:

    Entirely New Button Design

Prefer:

    Existing Panel + Different Contents

over:

    New Panel Style

Add toolkit components when the interaction genuinely requires something new.

This prevents visual fragmentation as Foundry grows.

---

# 46. Top-Level Navigation Rule

Do not add a permanent main topbar module every time Foundry gains a mechanic.

A feature should belong under one of the major national pillars whenever practical.

Current pillars:

    ECONOMY
    TREASURY
    TECHNOLOGY
    GOVERNMENT
    POPULATION
    MARKETS
    DIPLOMACY
    MILITARY
    STATES

Examples:

Urbanization → States

Administration → Government / States

Transport → Markets / Economy / States depending on context

Production Methods → Economy / Technology

Trade Agreements → Markets / Diplomacy

Military Logistics → Military

Multiple windows may expose different views into the same underlying simulation.

---

# 47. Contextual UI

Foundry should prefer contextual controls over permanently visible controls where appropriate.

Example:

A province without an Urban Center does not need to display fifteen disabled urban-building buttons.

Instead:

    Establish Urban Center

After establishment, the Urban Center interface becomes available.

Similarly, advanced military controls should appear as the country's organizational capability develops.

This helps the UI itself communicate technological and institutional progression.

---

# 48. Long-Term UI Evolution

Foundry spans approximately two centuries of transformation.

The UI architecture should support that progression without requiring a completely different game interface every era.

Example:

The Military module exists in 1760 and 1920.

In 1760 it primarily exposes:

- Field armies
- Regiments
- Manpower
- Supply
- Navy

Later it may expose:

- Divisions
- Corps
- Armies
- Logistics
- Front commands
- Theaters
- Operations

The top-level module remains familiar while its internal capabilities evolve.

The same philosophy applies to:

- Economy
- Markets
- Technology
- States
- Administration

---

# 49. Toolkit Source of Truth

The approved Foundry GUI Toolkit concept art and approved module/window mockups should be retained as visual references alongside this specification.

This Markdown document defines:

- Behavior
- Hierarchy
- Component roles
- Semantic meaning
- Implementation philosophy

The approved artwork defines:

- Exact visual character
- Ornamentation
- Surface treatment
- Icon rendering
- Border appearance
- Visual proportions

Neither should be treated in isolation.

---

# 50. Core Rule

The Foundry UI should always answer three questions:

1. **What is happening?**
2. **Why is it happening?**
3. **What can I do about it?**

And it should do so through a consistent visual language that feels deliberately designed for Foundry rather than assembled from a growing collection of Victoria II modifications.

---

# Summary

The Foundry GUI system is built around:

**BURGUNDY + GOLD + PARCHMENT**

combined with:

**REUSABLE COMPONENTS**

**INFORMATION-DENSE LAYOUTS**

**CLEAR VISUAL HIERARCHY**

**COLORFUL COMMODITY ART**

**CONSISTENT SYSTEM ICONOGRAPHY**

**CONTEXTUAL CONTROLS**

**EXPLANATORY TOOLTIPS**

**MODULAR WINDOWS**

and:

> **UI layouts designed around Foundry's mechanics rather than forcing Foundry's mechanics into vanilla Victoria II layouts.**