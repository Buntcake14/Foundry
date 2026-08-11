# Foundry UI Window Construction Guide

This guide records the practical rules learned while building Foundry's Urban
Center window. Follow it for new native-looking Victoria II windows and when
changing existing windows.

## The three layers of a window

A Foundry/Alice window is normally composed of three independent layers:

1. **Window container (`.gui`)** — establishes the logical position, size,
   orientation, interaction area, and child definitions.
2. **Raster graphics (`.gfx` plus PNG/DDS)** — supplies the frame, parchment,
   headers, dividers, image apertures, buttons, and progress-bar textures.
3. **C++ elements** — instantiate the named children and update live text,
   charts, visibility, tooltips, and button behavior.

Changing one layer does not automatically resize or reposition the others.
Most visual alignment bugs are disagreements between these layers rather than
an incorrect coordinate on a single element.

## Match the reference window's real geometry

Do not assume the logical window size is the background image size. The base
province window is the important example:

- logical `windowType` size: `380 x 490`
- `province_bg.dds` size: `416 x 548`
- background position: `{ 0 -19 }`

The raster deliberately extends beyond the logical container to draw its gold
frame, lower edge, shadow, and transparent padding. A custom adjacent window
that should match the province window must use the same geometry. Making a
`380 x 490` or `380 x 509` background and placing it at `{ 0 0 }` will produce
different top and bottom edges even if the live content looks approximately
correct.

Before building a matching window:

1. Locate its `.gui` `windowType`.
2. Locate the referenced `.gfx` sprite.
3. Inspect the source image's exact pixel dimensions and alpha channel.
4. Record the background offset.
5. Reproduce all four values before positioning child elements.

## Preserve alpha

Victoria II DDS backgrounds may contain transparent padding. Converting DDS to
RGB and saving as PNG destroys that transparency and turns the padding into
black, white, or otherwise visible blocks.

Always convert matching UI backgrounds as RGBA and preserve or restore the
source alpha channel. Validate transparent edges before testing in game.

## Fit artwork to the aperture instead of chasing offsets

If moving an image up hides a gap at the top but creates one at the bottom, the
image and aperture have different dimensions. Position changes cannot solve a
size mismatch.

Measure the aperture's inner rectangle, resize every sprite frame to that exact
size, and anchor the sprite to the aperture's inner top-left pixel. For sprite
sheets, every frame must have identical dimensions and `noOfFrames` must match.

The Urban Center density strip currently demonstrates this rule:

- nine frames
- each fitted frame: `382 x 101`
- sheet: `3438 x 101`
- child position: `{ 8 46 }`

## Use the province-window visual grammar

Native-looking Vic2 windows are not one uninterrupted parchment field. Use:

- textured burgundy headers with white text;
- cool ivory-grey parchment rather than saturated tan;
- alternating light and slightly darker content sections;
- thin dark divider lines, optionally paired with a subtle gold highlight;
- light inset boxes for compact statistics;
- left-aligned labels and right-aligned numeric values;
- consistent gold outer framing and close-button placement.

Do not duplicate information already visible in the adjacent parent window.
Every chart or inset should answer a new gameplay question. Empty space is
preferable to a redundant chart.

## Treat `.gui` child names as an interface contract

Every live child name in `.gui` must match a branch in the window's
`make_child` implementation. If C++ stores a child pointer for later updates,
the pointer remains null when the names disagree. Calling `set_text`,
`set_visible`, or another method on that pointer will crash when the window is
opened.

When renaming or splitting a child:

1. Update the `.gui` definition.
2. Update the `make_child` name comparison.
3. Update the stored pointer name.
4. Update every use in `on_update`.
5. Add a null check where a missing optional child should not be fatal.

Static labels can use `simple_text_element_base`. Dynamic values should have
their own named definitions and stored pointers. This makes left-label/right-
value layouts straightforward.

## Visibility-driven states

Prefer mutually exclusive elements over changing one element into several
different visual roles. For the Urban Center construction status:

- idle: show `NO CONSTRUCTION IN PROGRESS`, hide the progress bar;
- active: hide the idle text, show the progress bar in the same area;
- the disabled action button confirms that construction is already active.

This is clearer and avoids redundant percentage text beside a graphical bar.

## Progress bars

Progress-bar dimensions come from the `.gfx` `progressbartype`, not from the
`.gui` icon. To make one wider, define or modify a `progressbartype` with the
desired `size`, then recenter its `.gui` position. Moving the icon cannot change
its rendered width.

## Scenario cache versus executable rebuilds

There are two separate stale-state problems.

### New GUI or GFX definition names

Alice stores GUI definition IDs in the compiled scenario. Adding or renaming a
`.gui`/`.gfx` definition may require a clean scenario rebuild. Old cached IDs
can produce invalid `gui_def_id` access or missing children.

Back up stale scenario files rather than deleting them permanently. Scenario
caches are stored under:

`~/.local/share/Alice/scenarios/`

Changing only positions, text, or the texture path of an existing definition
normally does not require a scenario rebuild.

### C++ UI changes

Foundry's launcher and game are separate targets:

- `launch_alice` is the launcher.
- `AliceIncremental` is the game executable started by the launcher.

Rebuilding only the launcher can pair new GUI files with stale C++ child names.
That caused the Urban Center null-pointer crash. `launch.sh` therefore builds
both targets before launching:

```bash
cmake --build build --target AliceIncremental launch_alice -j2
```

Do not report a C++ UI change as ready until both targets build successfully.

## Runtime asset synchronization

Project files are not necessarily loaded directly from the repository. Foundry
currently synchronizes maintained UI files into the Victoria II runtime tree in
`launch.sh`. Every newly referenced project asset must be added to that sync
list or the runtime will continue showing an older asset or a missing sprite.

Prefer reusing an existing `.gfx` definition name when only replacing its
texture. This avoids unnecessary GUI-ID churn and scenario rebuilds.

## Debugging a window-open crash

Do not keep adjusting layout after a crash. Obtain the backtrace first.

Useful Linux commands:

```bash
coredumpctl --reverse list
coredumpctl info <PID>
coredumpctl dump <PID> --output=/tmp/foundry-ui.core
gdb -q -batch -ex 'thread apply all bt 20' build/AliceIncremental /tmp/foundry-ui.core
```

A backtrace showing `simple_text_element_base::set_text(this=0x0)` almost
always means a stored dynamic-text pointer was never populated, commonly due to
a `.gui`/C++ name mismatch or a stale game executable.

## Implementation checklist

Before play-testing a new window:

- [ ] Identify the reference window's logical size.
- [ ] Inspect the background's true pixel size and alpha.
- [ ] Match its background offset and orientation.
- [ ] Define visual sections before placing live data.
- [ ] Fit images to their aperture dimensions.
- [ ] Confirm every `.gui` name has the intended `make_child` handler.
- [ ] Keep dynamic child pointers null-safe where appropriate.
- [ ] Add every new asset to `launch.sh` synchronization.
- [ ] Rebuild both `AliceIncremental` and `launch_alice`.
- [ ] Rebuild the scenario only if definition IDs or parsed scenario data changed.
- [ ] Test the window idle, active, disabled, zero-value, and maximum-value states.
- [ ] Hover every chart, button, value, and progress indicator.
- [ ] Verify top, bottom, and adjacent-window alignment at the target resolution.
- [ ] Run `git diff --check` before handoff.

## Urban Center reference files

The current implementation is a useful working example:

- layout and element definitions: `assets/alice.gui`
- sprite and progress definitions: `assets/alice.gfx`
- live behavior: `src/gui/gui_province_window.cpp`
- runtime synchronization and builds: `launch.sh`
- design rules: `FOUNDRY_URBAN_CENTER_DESIGN.md`

