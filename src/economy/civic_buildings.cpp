#include "civic_buildings.hpp"
#include "system_state.hpp"
#include "triggers.hpp"

namespace civic_buildings {

void initialize_size_of_dcon_arrays(sys::state& state) {
	auto count = int32_t(state.world.civic_building_type_size());
	state.world.province_resize_civic_building_level(count);
	state.world.province_resize_civic_building_progress(count);
}

// Generic leveling for every civic_building_type: once a level's requirement
// trigger is satisfied, progress accumulates over that level's construction_time
// (in days) until it completes, then the active province modifier is swapped
// for the new level's. Urban-center-specific effects (RGO penalty, gating
// factory/admin construction) and RGO leveling's own private/national split
// are layered on top of this in their own dedicated update functions -- this
// function only owns level/progress/modifier bookkeeping.
void update_leveling(sys::state& state) {
	for(auto type : state.world.in_civic_building_type) {
		auto level_count = state.world.civic_building_type_get_level_count(type);
		if(level_count == 0)
			continue;
		auto& levels = state.world.civic_building_type_get_levels(type);

		state.world.for_each_province([&](dcon::province_id p) {
			auto owner = state.world.province_get_nation_from_province_ownership(p);
			if(!owner)
				return;

			auto cur_level = state.world.province_get_civic_building_level(p, type.id.index());
			if(cur_level >= level_count)
				return;

			auto& next_level = levels[cur_level];
			bool requirement_met = !bool(next_level.requirement)
				|| trigger::evaluate(state, next_level.requirement, trigger::to_generic(p), trigger::to_generic(p), trigger::to_generic(p));
			if(!requirement_met)
				return;

			auto build_time = std::max<int16_t>(1, next_level.construction_time);
			auto progress = state.world.province_get_civic_building_progress(p, type.id.index()) + 1.0f / float(build_time);

			if(progress >= 1.0f) {
				state.world.province_set_civic_building_level(p, type.id.index(), uint8_t(cur_level + 1));
				state.world.province_set_civic_building_progress(p, type.id.index(), 0.0f);

				if(cur_level > 0 && levels[cur_level - 1].modifier)
					sys::remove_modifier_from_province(state, p, levels[cur_level - 1].modifier);
				if(next_level.modifier)
					sys::add_modifier_to_province(state, p, next_level.modifier, sys::date{});
			} else {
				state.world.province_set_civic_building_progress(p, type.id.index(), progress);
			}
		});
	}
}

bool province_unlocks_factories(sys::state& state, dcon::province_id p) {
	for(auto type : state.world.in_civic_building_type) {
		if(!type.get_is_urban_center())
			continue;
		auto level_count = type.get_level_count();
		auto cur_level = state.world.province_get_civic_building_level(p, type.id.index());
		auto& levels = type.get_levels();
		for(uint8_t i = 0; i < cur_level && i < level_count; ++i) {
			if(levels[i].unlocks_factories)
				return true;
		}
	}
	return false;
}

bool province_unlocks_admin_buildings(sys::state& state, dcon::province_id p) {
	for(auto type : state.world.in_civic_building_type) {
		if(!type.get_is_urban_center())
			continue;
		auto level_count = type.get_level_count();
		auto cur_level = state.world.province_get_civic_building_level(p, type.id.index());
		auto& levels = type.get_levels();
		for(uint8_t i = 0; i < cur_level && i < level_count; ++i) {
			if(levels[i].unlocks_admin_buildings)
				return true;
		}
	}
	return false;
}

float province_urban_rgo_penalty(sys::state& state, dcon::province_id p) {
	float penalty = 0.0f;
	for(auto type : state.world.in_civic_building_type) {
		if(!type.get_is_urban_center())
			continue;
		auto cur_level = state.world.province_get_civic_building_level(p, type.id.index());
		if(cur_level == 0)
			continue;
		auto& levels = type.get_levels();
		penalty += levels[cur_level - 1].rgo_output_penalty;
	}
	return std::clamp(penalty, 0.0f, 1.0f);
}

}
