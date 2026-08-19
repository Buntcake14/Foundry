// Foundry web UI -- real game-map rendering (political-far mode).
//
// Flat textured plane in Three.js (not a 2D canvas library) so the perspective
// camera / globe-warp shader and real 3D unit models Alice's native map actually
// uses become later additions instead of a rewrite -- see the map-rendering plan
// doc for the reasoning.
//
// The map is composited in a custom ShaderMaterial rather than a single flat
// texture, mirroring (a simplified version of) Alice's own political-far map
// shader (assets/shaders/glsl/map_f.glsl's get_land_political_far /
// get_water_political): a terrain colormap wash, tinted by each province's
// real owner color, with borders drawn by comparing neighboring province ids
// in the fragment shader instead of porting Alice's separate vector
// border-line renderer.

import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import { LineSegmentsGeometry } from "three/addons/lines/LineSegmentsGeometry.js";
import { LineSegments2 } from "three/addons/lines/LineSegments2.js";
import { LineMaterial } from "three/addons/lines/LineMaterial.js";

// Root cause found 2026-08-18: this was never actually an orientation/flip bug
// at all. /map/province_at was indexing into the engine's internal province
// lookup array using raw browser pixel coordinates, but that array is taller
// than the served image (a legacy padding quirk in Alice's own .bmp map loader
// scales it by 1.3x) -- so every lookup was reading the wrong rows outright,
// which happened to look flip-shaped depending on where you clicked. Fixed
// properly server-side (controllers.hpp's /map/province_at now applies the same
// offset the engine's own loader does). No client-side flip needed for this.
const FLIP_Y = false;

// Every ~10th sim tick, not every tick -- ownership (and therefore province
// colors) changes rarely (wars/peace deals), so refetching+reuploading the
// color LUT on every single tick push would be wasted work.
const COLOR_REFRESH_TICK_INTERVAL = 10;

// The real vanilla Vic2 map-mode shortcuts, found in the actual game data
// (interface/menubar.gui's mapmode_N button shortcut fields, N = the mode's
// map_mode::mode enum value) -- not invented. 19 of the 22 native map modes
// have a keyboard shortcut in vanilla; the other three (relation, crisis,
// naval) are mouse-only there too, so they're left out here as well. `mode:
// null` (terrain) means "no political tint, just the plain wash" -- terrain
// has no X_map_from function on the backend, unlike every other mode.
const KEY_TO_MODE = {
	q: { mode: null, label: "Terrain" },
	w: { mode: "political", label: "Political" },
	e: { mode: "infrastructure", label: "Infrastructure" },
	r: { mode: "diplomatic", label: "Diplomatic" },
	t: { mode: "region", label: "Region" },
	y: { mode: "revolt", label: "Revolt Risk" },
	u: { mode: "admin", label: "Administration" },
	i: { mode: "colonial", label: "Colonial" },
	o: { mode: "recruitment", label: "Recruitment" },
	p: { mode: "national_focus", label: "National Focus" },
	a: { mode: "rgo_output", label: "RGO Output" },
	s: { mode: "population", label: "Population" },
	d: { mode: "nationality", label: "Nationality" },
	f: { mode: "sphere", label: "Sphere of Influence" },
	g: { mode: "supply", label: "Supply" },
	h: { mode: "party_loyalty", label: "Party Loyalty" },
	j: { mode: "rank", label: "Rank" },
	k: { mode: "migration", label: "Migration" },
	l: { mode: "civilization_level", label: "Civilization Level" },
};

const mapContainer = document.getElementById("map-container");
const panelContent = document.getElementById("panel-content");
const statusEl = document.getElementById("status");
const statusText = document.getElementById("status-text");
const modeText = document.getElementById("mode-text");

// World units scaled down from raw pixel dimensions just to keep camera
// distances/near-far planes in a comfortable range -- shared by the plane
// geometry and the border-line vertex positions, which must agree exactly.
const SCALE = 0.2;

let mapSizeX = 0;
let mapRawHeight = 0;
let provinceCount = 0;
let selectedProvinceId = null;
let provinceColorsTexture = null;
let ticksSinceColorRefresh = 0;
let activeMode = "political";
let borderRecords = null; // Uint16Array view: [x1,y1,x2,y2,provinceMapIdA,provinceMapIdB] per segment
let smoothedChains = null; // [{ a, b, points: [[x,y], ...] }, ...] -- built once from borderRecords
let nationalBorderLine = null;
let internalBorderLine = null;

// --- Three.js scene setup ---

const scene = new THREE.Scene();
scene.background = new THREE.Color(0x111111);

const camera = new THREE.PerspectiveCamera(45, 1, 0.1, 10000);
camera.position.set(0, 0, 800);

const renderer = new THREE.WebGLRenderer({ antialias: true });
mapContainer.appendChild(renderer.domElement);

const controls = new OrbitControls(camera, renderer.domElement);
controls.enableRotate = false; // pan/zoom only, straight-down view
controls.screenSpacePanning = true;

function resize() {
	const w = mapContainer.clientWidth;
	const h = mapContainer.clientHeight;
	renderer.setSize(w, h);
	camera.aspect = w / h;
	camera.updateProjectionMatrix();
	if(nationalBorderLine)
		nationalBorderLine.material.resolution.set(w, h);
	if(internalBorderLine)
		internalBorderLine.material.resolution.set(w, h);
}
window.addEventListener("resize", resize);

let planeMesh = null;

// Nearest-filtered, non-color-managed load -- these encode raw ids/lookup
// indices or need to stay pixel-crisp, not photographic color data.
function loadDataLikeTexture(url) {
	return new Promise((resolve, reject) => {
		new THREE.TextureLoader().load(
			url,
			(texture) => {
				texture.colorSpace = THREE.NoColorSpace;
				texture.magFilter = THREE.NearestFilter;
				texture.minFilter = THREE.NearestFilter;
				texture.generateMipmaps = false;
				texture.wrapS = THREE.ClampToEdgeWrapping;
				texture.wrapT = THREE.ClampToEdgeWrapping;
				// Three.js's TextureLoader defaults flipY to true (compensating for how
				// browsers decode images vs. WebGL's texture row order), which inverts
				// the map on screen -- confirmed by live testing.
				texture.flipY = false;
				resolve(texture);
			},
			undefined,
			reject
		);
	});
}

// LINEAR-filtered but ClampToEdge (not Repeat, unlike the wash textures) --
// addressed by the same cropped vUv space as provinceIds/terrainIds, just a
// smooth binary mask rather than categorical id data, so ordinary bilinear
// sampling gives a genuinely anti-aliased land/water edge for free instead of
// province_ids.png's hard-pixel staircase.
function loadMaskTexture(url) {
	return new Promise((resolve, reject) => {
		new THREE.TextureLoader().load(
			url,
			(texture) => {
				texture.colorSpace = THREE.NoColorSpace;
				texture.magFilter = THREE.LinearFilter;
				texture.minFilter = THREE.LinearFilter;
				texture.generateMipmaps = false;
				texture.wrapS = THREE.ClampToEdgeWrapping;
				texture.wrapT = THREE.ClampToEdgeWrapping;
				texture.flipY = false;
				resolve(texture);
			},
			undefined,
			reject
		);
	});
}

function loadWashTexture(url) {
	return new Promise((resolve, reject) => {
		new THREE.TextureLoader().load(
			url,
			(texture) => {
				// NoColorSpace, not SRGBColorSpace: this custom ShaderMaterial reads texels
				// with plain texture2D() and blends them by hand (political mix, border
				// darkening) -- an sRGB colorSpace tag makes WebGL upload the texture with an
				// sRGB GPU internal format, which silently gamma-decodes every texture2D()
				// sample at the hardware level, before our own math ever sees it.
				texture.colorSpace = THREE.NoColorSpace;
				texture.magFilter = THREE.LinearFilter;
				texture.minFilter = THREE.LinearFilter;
				texture.wrapS = THREE.RepeatWrapping;
				texture.wrapT = THREE.RepeatWrapping;
				texture.flipY = false;
				resolve(texture);
			},
			undefined,
			reject
		);
	});
}

// The terrain-sheet atlas is real tileable texture detail (not ID data or a
// flat wash), so unlike every other texture here it should actually filter and
// mipmap -- matching how the native client's own terrainsheet_texture_sampler
// is set up (GL_LINEAR_MIPMAP_LINEAR, map.cpp). Tile boundaries are handled by
// hand in sampleTerrainTile's UV math, so ClampToEdge (not Repeat) is correct
// here -- it just prevents bleeding across the outer edge of the 2048x2048 sheet.
function loadTerrainSheetTexture(url) {
	return new Promise((resolve, reject) => {
		new THREE.TextureLoader().load(
			url,
			(texture) => {
				texture.colorSpace = THREE.NoColorSpace;
				texture.magFilter = THREE.LinearFilter;
				texture.minFilter = THREE.LinearMipmapLinearFilter;
				texture.generateMipmaps = true;
				texture.wrapS = THREE.ClampToEdgeWrapping;
				texture.wrapT = THREE.ClampToEdgeWrapping;
				texture.flipY = false;
				resolve(texture);
			},
			undefined,
			reject
		);
	});
}

// Builds (or rebuilds) the province-id -> owner-color lookup texture from
// /map/province_colors's flat [r,g,b, r,g,b, ...] array, one triple per
// province index (province_id.index(), i.e. map_id - 1).
function buildProvinceColorsTexture(flatRgb, count) {
	const data = new Uint8Array(count * 4);
	for(let i = 0; i < count; i++) {
		data[i * 4 + 0] = flatRgb[i * 3 + 0];
		data[i * 4 + 1] = flatRgb[i * 3 + 1];
		data[i * 4 + 2] = flatRgb[i * 3 + 2];
		data[i * 4 + 3] = 255;
	}
	const texture = new THREE.DataTexture(data, count, 1, THREE.RGBAFormat, THREE.UnsignedByteType);
	texture.colorSpace = THREE.NoColorSpace;
	texture.magFilter = THREE.NearestFilter;
	texture.minFilter = THREE.NearestFilter;
	texture.generateMipmaps = false;
	texture.wrapS = THREE.ClampToEdgeWrapping;
	texture.wrapT = THREE.ClampToEdgeWrapping;
	texture.needsUpdate = true;
	return texture;
}

async function refreshProvinceColors(material, mode) {
	if(mode == null) {
		// Terrain mode: no X_map_from function on the backend, nothing to fetch --
		// the useTint uniform (set by setActiveMode) already tells the shader to skip
		// the political mix entirely, so the stale LUT contents just go unused.
		return;
	}
	const res = await fetch(`/map/province_colors?mode=${encodeURIComponent(mode)}`);
	if(!res.ok)
		return;
	const flatRgb = await res.json();
	if(!provinceColorsTexture) {
		provinceColorsTexture = buildProvinceColorsTexture(flatRgb, provinceCount);
		material.uniforms.provinceColors.value = provinceColorsTexture;
	} else {
		const data = provinceColorsTexture.image.data;
		for(let i = 0; i < provinceCount; i++) {
			data[i * 4 + 0] = flatRgb[i * 3 + 0];
			data[i * 4 + 1] = flatRgb[i * 3 + 1];
			data[i * 4 + 2] = flatRgb[i * 3 + 2];
		}
		provinceColorsTexture.needsUpdate = true;
	}
	rebuildBorderLines();
}

// Fetched once and cached client-side for the rest of the session -- border
// geometry never changes while the process is running, only which pairs count
// as "national" (which depends on the current color LUT, reclassified by
// rebuildBorderLines on every mode switch / periodic color refresh instead).
async function loadBorderSegments() {
	const buf = await (await fetch("/map/border_segments")).arrayBuffer();
	// Both the server (x86_64) and every realistic browser platform are
	// little-endian, so a plain typed-array view (not DataView) over the raw
	// bytes is safe and avoids a manual per-field unpack of 128K records.
	const count = new Uint32Array(buf, 0, 1)[0];
	borderRecords = new Uint16Array(buf, 4, count * 6);
	smoothedChains = buildSmoothedChains(borderRecords);
}

// The raw segments are purely axis-aligned (the server-side merge only walks
// horizontal/vertical pixel-grid boundaries), so a genuinely diagonal
// real-world border rasterizes into a staircase -- and that staircase is
// exactly what used to render. Fixed here in two steps: group by province
// pair and trace each pair's segments into ordered polylines, then smooth
// each one with Chaikin's corner-cutting algorithm (see chaikinSmooth).
function buildSmoothedChains(records) {
	const segmentCount = records.length / 6;
	const byPair = new Map(); // "a-b" -> { a, b, segments: [[x1,y1,x2,y2], ...] }
	for(let i = 0; i < segmentCount; i++) {
		const o = i * 6;
		const entry = { a: records[o + 4], b: records[o + 5] };
		const key = entry.a + "-" + entry.b;
		let group = byPair.get(key);
		if(!group) {
			group = { a: entry.a, b: entry.b, segments: [] };
			byPair.set(key, group);
		}
		group.segments.push([records[o], records[o + 1], records[o + 2], records[o + 3]]);
	}

	const chains = [];
	for(const { a, b, segments } of byPair.values()) {
		for(const chain of chainSegments(segments)) {
			chains.push({ a, b, points: chaikinSmooth(chain) });
		}
	}
	return chains;
}

// Traces a set of axis-aligned segments (each [x1,y1,x2,y2]) into ordered
// point-chains by matching shared endpoints. A pair's own edge set generally
// forms simple paths with no branching -- a real 3-province junction is
// exactly where this pair's chain ends and a *different* pair's chain begins
// -- so walking each unvisited segment outward in both directions until no
// unused segment shares the current endpoint is sufficient; a chain that
// wraps back to its own start (first point === last point) is a closed loop
// (a fully-enclosed enclave).
function chainSegments(segments) {
	const pointKey = (x, y) => x * 100000 + y; // coords always < 6000, safe
	const adjacency = new Map(); // pointKey -> [segment indices]
	segments.forEach((s, i) => {
		const k1 = pointKey(s[0], s[1]), k2 = pointKey(s[2], s[3]);
		if(!adjacency.has(k1)) adjacency.set(k1, []);
		if(!adjacency.has(k2)) adjacency.set(k2, []);
		adjacency.get(k1).push(i);
		adjacency.get(k2).push(i);
	});

	const used = new Uint8Array(segments.length);
	const otherEnd = (segIdx, fromKey) => {
		const s = segments[segIdx];
		return fromKey === pointKey(s[0], s[1]) ? [s[2], s[3]] : [s[0], s[1]];
	};
	const unusedAt = (k) => (adjacency.get(k) || []).filter((idx) => !used[idx]);

	function walk(out, point) {
		for(;;) {
			const options = unusedAt(pointKey(point[0], point[1]));
			if(options.length !== 1) // dead end, or (rare) a branch -- stop either way
				return;
			const segIdx = options[0];
			used[segIdx] = 1;
			point = otherEnd(segIdx, pointKey(point[0], point[1]));
			out.push(point);
		}
	}

	const chains = [];
	for(let i = 0; i < segments.length; i++) {
		if(used[i])
			continue;
		used[i] = 1;
		const s = segments[i];
		const chain = [[s[0], s[1]], [s[2], s[3]]];
		walk(chain, chain[chain.length - 1]); // extend forward from the seed's end
		chain.reverse();
		walk(chain, chain[chain.length - 1]); // extend forward from the seed's start (chain now reversed)
		chains.push(chain);
	}
	return chains;
}

// Standard Chaikin corner-cutting: each pass replaces every interior corner
// with two points 25%/75% along its adjacent edges, converging toward a
// smooth curve (a segment with no corner on either side is left untouched, so
// genuinely straight borders stay straight). An open chain's first and last
// point are pinned -- they're exactly the points shared with a neighboring
// pair's chain at a real province junction, and moving them would pull this
// pair's line out of alignment there, opening a visible crack. A closed loop
// (first point === last point) has no such junction and is cut all the way
// around.
function chaikinSmooth(points, iterations = 2) {
	if(points.length < 3)
		return points;
	const closed = points[0][0] === points[points.length - 1][0] && points[0][1] === points[points.length - 1][1];

	let pts = points;
	for(let iter = 0; iter < iterations; iter++) {
		const n = pts.length;
		const segCount = n - 1;
		const out = [];
		if(!closed)
			out.push(pts[0]);
		for(let i = 0; i < segCount; i++) {
			const p = pts[i], q = pts[i + 1];
			const Q = [p[0] * 0.75 + q[0] * 0.25, p[1] * 0.75 + q[1] * 0.25];
			const R = [p[0] * 0.25 + q[0] * 0.75, p[1] * 0.25 + q[1] * 0.75];
			if(!closed && i === 0) {
				out.push(R);
			} else if(!closed && i === segCount - 1) {
				out.push(Q);
			} else {
				out.push(Q, R);
			}
		}
		if(!closed)
			out.push(pts[n - 1]);
		else
			out.push(out[0]); // re-close the loop
		pts = out;
	}
	return pts;
}

function pixelToLocal(px, py) {
	return [(px - mapSizeX / 2) * SCALE, (py - mapRawHeight / 2) * SCALE];
}

function makeBorderLine(color, linewidth) {
	const geometry = new LineSegmentsGeometry();
	const material = new LineMaterial({ color, linewidth, worldUnits: false });
	material.resolution.set(mapContainer.clientWidth, mapContainer.clientHeight);
	const line = new LineSegments2(geometry, material);
	line.position.z = 0.5; // above the terrain plane (z=0), avoids z-fighting
	return line;
}

// Reclassifies every unique province-pair as national (different political
// color, i.e. different owner) or internal (same owner) using the same
// color-equality trick the old shader-based border code used, then rebuilds
// both LineSegments2 geometries. Cheap relative to the one-time segment fetch
// -- run on every provinceColors refresh (mode switch, periodic tick).
function rebuildBorderLines() {
	if(!smoothedChains || !planeMesh)
		return;

	const colorData = provinceColorsTexture ? provinceColorsTexture.image.data : null;
	const useColors = activeMode != null && colorData != null;
	const pairIsNational = new Map(); // "a-b" -> bool, avoids recoloring the same pair repeatedly

	const nationalPositions = [];
	const internalPositions = [];
	for(const { a, b, points } of smoothedChains) {
		let national = false;
		if(useColors) {
			const key = a < b ? a + "-" + b : b + "-" + a;
			if(pairIsNational.has(key)) {
				national = pairIsNational.get(key);
			} else {
				const ia = (a - 1) * 4, ib = (b - 1) * 4;
				national = colorData[ia] !== colorData[ib] || colorData[ia + 1] !== colorData[ib + 1] || colorData[ia + 2] !== colorData[ib + 2];
				pairIsNational.set(key, national);
			}
		}

		const target = national ? nationalPositions : internalPositions;
		for(let i = 0; i < points.length - 1; i++) {
			const [lx1, ly1] = pixelToLocal(points[i][0], points[i][1]);
			const [lx2, ly2] = pixelToLocal(points[i + 1][0], points[i + 1][1]);
			target.push(lx1, ly1, 0, lx2, ly2, 0);
		}
	}

	if(!nationalBorderLine) {
		// Warm gold-tan, not dark brown/black -- matches the soft, low-contrast
		// border look in Alice's own nation-select screen (the reference the user
		// pointed at), not a harsh cartographic outline.
		nationalBorderLine = makeBorderLine(0x6b4a1f, 1.8);
		internalBorderLine = makeBorderLine(0x8a7550, 0.8);
		scene.add(nationalBorderLine);
		scene.add(internalBorderLine);
	}
	nationalBorderLine.geometry.dispose();
	nationalBorderLine.geometry = new LineSegmentsGeometry().setPositions(nationalPositions);
	internalBorderLine.geometry.dispose();
	internalBorderLine.geometry = new LineSegmentsGeometry().setPositions(internalPositions);
}

function setActiveMode(key) {
	const entry = KEY_TO_MODE[key];
	if(!entry || !planeMesh)
		return;
	activeMode = entry.mode;
	modeText.textContent = entry.label;
	planeMesh.userData.material.uniforms.useTint.value = entry.mode == null ? 0.0 : 1.0;
	if(entry.mode != null)
		refreshProvinceColors(planeMesh.userData.material, entry.mode);
	else
		rebuildBorderLines(); // terrain mode: refreshProvinceColors short-circuits, so reclassify (all-internal) here instead
}

window.addEventListener("keydown", (event) => {
	// Ignore modified presses (Ctrl/Alt/Meta+letter is almost always a browser
	// shortcut, not a map-mode switch) and don't fight normal typing if a future
	// panel ever adds a text input.
	if(event.ctrlKey || event.altKey || event.metaKey)
		return;
	if(document.activeElement && (document.activeElement.tagName === "INPUT" || document.activeElement.tagName === "TEXTAREA"))
		return;
	setActiveMode(event.key.toLowerCase());
});

const VERTEX_SHADER = `
varying vec2 vUv;
void main() {
	vUv = uv;
	gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
}
`;

// Port of map_f.glsl's get_land_political_far / get_water_political /
// get_terrain_mix / get_corrected_coords, adapted to this pipeline's own
// cropped coordinate space (see /map/meta's size_y/top_free_space additions
// and the mapping notes below). Borders are drawn via neighbor comparison in
// the fragment shader instead of porting Alice's separate vector border-line
// renderer (map_borders.cpp) -- a real v2 candidate, not built here.
const FRAGMENT_SHADER = `
varying vec2 vUv;

uniform sampler2D provinceIds;
uniform sampler2D provinceColors;
uniform sampler2D colormapLand;
uniform sampler2D colormapWater;
uniform sampler2D terrainIds;
uniform sampler2D terrainSheet;
uniform sampler2D coastlineMask;
uniform vec2 mapSize;
uniform float sizeY;
uniform float topFreeSpace;
uniform float firstSeaMapId;
uniform float provinceColorCount;
uniform float useTint;

float decodeMapId(vec2 uv) {
	vec4 texel = texture2D(provinceIds, uv);
	return floor(texel.r * 255.0 + 0.5) * 256.0 + floor(texel.g * 255.0 + 0.5);
}

vec3 politicalColorAt(float mapId) {
	float lutX = (mapId - 1.0 + 0.5) / provinceColorCount;
	return texture2D(provinceColors, vec2(lutX, 0.5)).rgb;
}

// colormap.dds/colormap_water.dds are separate equirectangular-world-space
// assets, not aligned 1:1 with the province bitmap's cropped pixel space vUv
// lives in -- this converts vUv into Alice's own padded-space coordinate and
// applies the same remap map_f.glsl's get_corrected_coords does.
vec2 correctedColormapUv(vec2 uv) {
	float paddedY = (topFreeSpace + uv.y * mapSize.y) / sizeY;
	vec2 coords = vec2(uv.x, paddedY);
	coords.y -= (1.0 - 1.0 / 1.3) * 3.0 / 5.0;
	coords.y *= 1.3;
	return coords;
}

float terrainIndexAt(vec2 corner) {
	vec2 uv = (floor(vUv * mapSize + vec2(0.5)) + 0.5 * corner) / mapSize;
	return floor(texture2D(terrainIds, uv).r * 255.0 + 0.5);
}

vec3 terrainTileAt(float index, vec2 offset) {
	vec2 sampleFrom = (mod(offset, 1.0) * (255.0 / 256.0) + (1.0 / 256.0 / 2.0)) / 8.0
		+ vec2(mod(index, 8.0), floor(index / 8.0)) / 8.0;
	return texture2D(terrainSheet, sampleFrom).rgb;
}

// Bilinear blend of the 4 surrounding 16-texel terrain-tile cells, exactly like
// map_f.glsl's get_terrain_mix -- this is what gives land its textured,
// hand-painted look instead of a flat color.
vec3 terrainMix() {
	vec2 scaling = fract(vUv * mapSize + vec2(0.5));
	vec2 offset = vUv * mapSize / 16.0;

	vec3 lu = terrainTileAt(terrainIndexAt(vec2(-1.0, -1.0)), offset);
	vec3 ld = terrainTileAt(terrainIndexAt(vec2(-1.0, 1.0)), offset);
	vec3 ru = terrainTileAt(terrainIndexAt(vec2(1.0, -1.0)), offset);
	vec3 rd = terrainTileAt(terrainIndexAt(vec2(1.0, 1.0)), offset);

	vec3 u = mix(lu, ru, scaling.x);
	vec3 d = mix(ld, rd, scaling.x);
	return mix(u, d, scaling.y);
}

void main() {
	float centerId = decodeMapId(vUv);
	bool isWater = centerId == 0.0 || centerId >= firstSeaMapId;
	vec2 colormapUv = correctedColormapUv(vUv);

	// coastlineMask is LINEAR-filtered (unlike provinceIds, which has to stay
	// NEAREST -- averaging two different province ids is meaningless), so this
	// value is already a smooth 0..1 ramp across the coast, not a hard step. That's
	// what turns the coastline from a hard pixel staircase into a soft edge.
	float landCoverage = texture2D(coastlineMask, vUv).r;

	vec3 waterColor = texture2D(colormapWater, colormapUv).rgb;
	// Same 2:1 ratio as map_f.glsl's get_terrain_mix: mostly the tiled terrain
	// texture, with a light wash of the broader regional color underneath so the
	// tiling doesn't read as too repetitive.
	vec3 landColor = mix(terrainMix(), texture2D(colormapLand, colormapUv).rgb, 1.0 / 3.0);

	// political_map_from's sea-province entries are a totally different thing from
	// land's: black (0,0,0) unless a nearby coastal nation projects influence onto
	// that sea zone (vanilla's "sphere of influence over sea" tinting), not a real
	// "this water belongs to no one" fallback -- open ocean far from any coast is
	// black there by design. Water always just shows the plain wash; only land
	// gets the political tint.
	if(!isWater && centerId > 0.0 && useTint > 0.5) {
		landColor = mix(landColor, politicalColorAt(centerId), 0.7);
	}

	vec3 color = mix(waterColor, landColor, landCoverage);

	// Borders are no longer drawn here -- real vector line geometry (LineSegments2,
	// built from /map/border_segments) renders on top of this plane instead of a
	// per-pixel raster comparison. See rebuildBorderLines() below.

	gl_FragColor = vec4(color, 1.0);
}
`;

async function init() {
	const metaRes = await fetch("/map/meta");
	const meta = await metaRes.json();
	mapSizeX = meta.size_x;
	mapRawHeight = meta.raw_height;
	provinceCount = meta.province_count;

	const [provinceIdsTex, colormapLandTex, colormapWaterTex, terrainIdsTex, terrainSheetTex, coastlineMaskTex, colorsRes] = await Promise.all([
		loadDataLikeTexture("/map/province_ids.png"),
		loadWashTexture("/app/map_assets/colormap.png"),
		loadWashTexture("/app/map_assets/colormap_water.png"),
		loadDataLikeTexture("/map/terrain_ids.png"),
		loadTerrainSheetTexture("/app/map_assets/texturesheet.png"),
		loadMaskTexture("/map/coastline_mask.png"),
		fetch(`/map/province_colors?mode=${encodeURIComponent(activeMode)}`),
		loadBorderSegments(),
	]);
	const flatRgb = await colorsRes.json();
	provinceColorsTexture = buildProvinceColorsTexture(flatRgb, provinceCount);

	const material = new THREE.ShaderMaterial({
		vertexShader: VERTEX_SHADER,
		fragmentShader: FRAGMENT_SHADER,
		uniforms: {
			provinceIds: { value: provinceIdsTex },
			provinceColors: { value: provinceColorsTexture },
			colormapLand: { value: colormapLandTex },
			colormapWater: { value: colormapWaterTex },
			terrainIds: { value: terrainIdsTex },
			terrainSheet: { value: terrainSheetTex },
			coastlineMask: { value: coastlineMaskTex },
			mapSize: { value: new THREE.Vector2(mapSizeX, mapRawHeight) },
			sizeY: { value: meta.size_y },
			topFreeSpace: { value: meta.top_free_space },
			firstSeaMapId: { value: meta.first_sea_province_map_id },
			provinceColorCount: { value: provinceCount },
			useTint: { value: 1.0 },
		},
	});

	const planeWidth = mapSizeX * SCALE;
	const planeHeight = mapRawHeight * SCALE;

	const geometry = new THREE.PlaneGeometry(planeWidth, planeHeight);
	planeMesh = new THREE.Mesh(geometry, material);
	planeMesh.userData.material = material;
	scene.add(planeMesh);

	rebuildBorderLines();
	modeText.textContent = "Political";

	camera.position.set(0, 0, Math.max(planeWidth, planeHeight) * 0.7);
	controls.target.set(0, 0, 0);
	controls.update();

	resize();
	animate();
	connectWebSocket();
}

function animate() {
	requestAnimationFrame(animate);
	controls.update();
	renderer.render(scene, camera);
}

// --- Click to select a province ---

const raycaster = new THREE.Raycaster();
const pointerNdc = new THREE.Vector2();

mapContainer.addEventListener("click", (event) => {
	if(!planeMesh)
		return;

	const rect = renderer.domElement.getBoundingClientRect();
	pointerNdc.x = ((event.clientX - rect.left) / rect.width) * 2 - 1;
	pointerNdc.y = -((event.clientY - rect.top) / rect.height) * 2 + 1;

	raycaster.setFromCamera(pointerNdc, camera);
	const hits = raycaster.intersectObject(planeMesh);
	if(hits.length === 0 || !hits[0].uv)
		return;

	const uv = hits[0].uv;
	const px = Math.min(mapSizeX - 1, Math.max(0, Math.round(uv.x * (mapSizeX - 1))));
	const rawPy = Math.round(uv.y * (mapRawHeight - 1));
	const py = FLIP_Y ? (mapRawHeight - 1 - rawPy) : rawPy;

	selectProvinceAt(px, py);
});

async function selectProvinceAt(x, y) {
	try {
		const atRes = await fetch(`/map/province_at?x=${x}&y=${y}`);
		if(!atRes.ok)
			return;
		const at = await atRes.json();
		// province_id is -1 (dcon's invalid-handle sentinel) for pixels with no
		// mapped province (ocean, image padding) -- and -1 is truthy in JS, so this
		// must be an explicit numeric check, not `!at.province_id`.
		if(at.province_id == null || at.province_id < 0) {
			panelContent.textContent = "No province here (sea/unowned).";
			selectedProvinceId = null;
			return;
		}
		selectedProvinceId = at.province_id;
		await refreshSelectedProvince();
	} catch(err) {
		panelContent.textContent = "Error: " + err;
	}
}

async function refreshSelectedProvince() {
	if(selectedProvinceId == null)
		return;
	const res = await fetch(`/province/${selectedProvinceId}`);
	if(!res.ok)
		return;
	const data = await res.json();
	panelContent.innerHTML = `<pre>${JSON.stringify(data, null, 2)}</pre>`;
}

// --- WebSocket push: refetch whatever's open, and periodically refresh the
// political color LUT, when the sim ticks ---

function connectWebSocket() {
	const ws = new WebSocket(`ws://${location.hostname}:1235`);

	ws.onopen = () => {
		statusEl.classList.add("connected");
		statusText.textContent = "live";
	};
	ws.onclose = () => {
		statusEl.classList.remove("connected");
		statusText.textContent = "disconnected, retrying...";
		setTimeout(connectWebSocket, 2000);
	};
	ws.onerror = () => ws.close();
	ws.onmessage = (event) => {
		let msg;
		try {
			msg = JSON.parse(event.data);
		} catch(err) {
			return;
		}
		if(msg.type === "tick") {
			statusText.textContent = `live (tick ${msg.seq})`;
			refreshSelectedProvince();

			ticksSinceColorRefresh++;
			if(planeMesh && ticksSinceColorRefresh >= COLOR_REFRESH_TICK_INTERVAL) {
				ticksSinceColorRefresh = 0;
				refreshProvinceColors(planeMesh.userData.material, activeMode);
			}
		}
	};
}

init();
