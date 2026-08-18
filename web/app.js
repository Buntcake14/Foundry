// Foundry web UI -- Phase 1 proof of concept.
//
// Flat textured plane in Three.js (not a 2D canvas library) so the perspective
// camera / globe-warp shader and real 3D unit models Alice's native map actually
// uses become later additions instead of a rewrite -- see the Phase 1 plan doc
// for the reasoning. This file intentionally does none of that yet: just prove
// click-to-select + live push works end to end.

import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";

// Root cause found 2026-08-18: this was never actually an orientation/flip bug
// at all. /map/province_at was indexing into the engine's internal province
// lookup array using raw browser pixel coordinates, but that array is taller
// than the served image (a legacy padding quirk in Alice's own .bmp map loader
// scales it by 1.3x) -- so every lookup was reading the wrong rows outright,
// which happened to look flip-shaped depending on where you clicked. Fixed
// properly server-side (controllers.hpp's /map/province_at now applies the same
// offset the engine's own loader does). No client-side flip needed for this.
const FLIP_Y = false;

const mapContainer = document.getElementById("map-container");
const panelContent = document.getElementById("panel-content");
const statusEl = document.getElementById("status");
const statusText = document.getElementById("status-text");

let textureWidth = 0;
let textureHeight = 0;
let selectedProvinceId = null;

// --- Three.js scene setup ---

const scene = new THREE.Scene();
scene.background = new THREE.Color(0x111111);

const camera = new THREE.PerspectiveCamera(45, 1, 0.1, 10000);
camera.position.set(0, 0, 800);

const renderer = new THREE.WebGLRenderer({ antialias: true });
mapContainer.appendChild(renderer.domElement);

const controls = new OrbitControls(camera, renderer.domElement);
controls.enableRotate = false; // Phase 1: pan/zoom only, straight-down view
controls.screenSpacePanning = true;

function resize() {
	const w = mapContainer.clientWidth;
	const h = mapContainer.clientHeight;
	renderer.setSize(w, h);
	camera.aspect = w / h;
	camera.updateProjectionMatrix();
}
window.addEventListener("resize", resize);

let planeMesh = null;

new THREE.TextureLoader().load("/map/provinces.png", (texture) => {
	texture.colorSpace = THREE.SRGBColorSpace;
	texture.magFilter = THREE.NearestFilter; // province colors must stay crisp/exact, no blending
	texture.minFilter = THREE.NearestFilter;
	// Three.js's TextureLoader defaults flipY to true (compensating for how
	// browsers decode images vs. WebGL's texture row order), which was inverting
	// this map on screen -- confirmed by live testing 2026-08-18.
	texture.flipY = false;

	textureWidth = texture.image.width;
	textureHeight = texture.image.height;

	// World units scaled down from raw pixel dimensions just to keep camera
	// distances/near-far planes in a comfortable range.
	const scale = 0.2;
	const planeWidth = textureWidth * scale;
	const planeHeight = textureHeight * scale;

	const geometry = new THREE.PlaneGeometry(planeWidth, planeHeight);
	const material = new THREE.MeshBasicMaterial({ map: texture });
	planeMesh = new THREE.Mesh(geometry, material);
	scene.add(planeMesh);

	camera.position.set(0, 0, Math.max(planeWidth, planeHeight) * 0.7);
	controls.target.set(0, 0, 0);
	controls.update();

	resize();
	animate();
});

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
	const px = Math.min(textureWidth - 1, Math.max(0, Math.round(uv.x * (textureWidth - 1))));
	const rawPy = Math.round(uv.y * (textureHeight - 1));
	const py = FLIP_Y ? (textureHeight - 1 - rawPy) : rawPy;

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

// --- WebSocket push: refetch whatever's open when the sim ticks ---

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
		}
	};
}
connectWebSocket();
