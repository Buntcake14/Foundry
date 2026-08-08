#pragma once
#include "constants_dcon.hpp"
#include "container_types_dcon.hpp"
#include "dcon_generated_ids.hpp"
#include "system_state_forward.hpp"

// Generic tiered, content-file-defined province buildings.
// Unlike advanced_province_buildings (hardcoded enum of exactly 3 fixed types),
// civic_building_type is a real scenario-loaded registry: mod content defines
// how many types exist and how many levels each has, up to max_levels.
//
// A level's `requirement` is evaluated as a normal province-scope trigger, so
// leveling conditions (population growth, other buildings present, etc.) are
// just script content, not engine code. A level's `modifier` references a
// normal modifier entry (same as event_modifiers.txt), applied to the
// province while that level is the building's current level, swapped out on
// level change exactly like Government Administration's old event-synced
// province modifiers -- except delivered directly by the engine instead of a
// country_event workaround, since Alice's dcon/trigger/modifier systems make
// that workaround unnecessary here.
//
// rgo_output_penalty/unlocks_factories/unlocks_admin_buildings are special
// hooks specific to the urban-center use of this system (see economy.cpp's
// RGO size calculation and construction.cpp's factory/admin build checks) --
// they're inert (0 / false) for civic buildings that are pure stat buffs.

namespace civic_buildings {

inline constexpr int32_t max_levels = 8;

struct level_definition {
	dcon::trigger_key requirement;
	dcon::modifier_id modifier;
	economy::commodity_set cost;
	int16_t construction_time = 0;
	float rgo_output_penalty = 0.f;
	bool unlocks_factories = false;
	bool unlocks_admin_buildings = false;
};

void initialize_size_of_dcon_arrays(sys::state& state);
void update_leveling(sys::state& state);

// True once any is_urban_center civic building in this province has reached a
// level whose unlocks_factories/unlocks_admin_buildings flag is set. New
// factory construction and (once a real admin-building system exists) admin
// building construction should both gate on these.
bool province_unlocks_factories(sys::state& state, dcon::province_id p);
bool province_unlocks_admin_buildings(sys::state& state, dcon::province_id p);

// Cumulative RGO output penalty from urban development eating into a
// province's resource-extraction land, scaled by the urban center's current
// level -- read directly by economy.cpp's RGO size calculation.
float province_urban_rgo_penalty(sys::state& state, dcon::province_id p);

}
