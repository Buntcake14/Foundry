#include "civic_buildings.hpp"
#include "system_state.hpp"
#include "triggers.hpp"

namespace civic_buildings {

void initialize_size_of_dcon_arrays(sys::state& state) {
	auto count = int32_t(state.world.civic_building_type_size());
	state.world.province_resize_civic_building_level(count);
	state.world.province_resize_civic_building_progress(count);
}

bool upgrade_in_progress(sys::state& state, dcon::province_id province, dcon::civic_building_type_id type) {
	return province && type
		&& state.world.province_get_civic_building_progress(province, type.index()) > 0.f;
}

bool can_begin_upgrade(sys::state& state, dcon::nation_id nation, dcon::province_id province,
		dcon::civic_building_type_id type) {
	if(!nation || !province || !type)
		return false;
	if(state.world.province_get_nation_from_province_ownership(province) != nation
			|| state.world.province_get_nation_from_province_control(province) != nation)
		return false;
	if(upgrade_in_progress(state, province, type))
		return false;

	auto level_count = state.world.civic_building_type_get_level_count(type);
	auto current_level = state.world.province_get_civic_building_level(province, type.index());
	if(current_level >= level_count)
		return false;

	auto& next_level = state.world.civic_building_type_get_levels(type)[current_level];
	return !next_level.requirement
		|| trigger::evaluate(state, next_level.requirement, trigger::to_generic(province),
			trigger::to_generic(province), trigger::to_generic(province));
}

void begin_upgrade(sys::state& state, dcon::province_id province, dcon::civic_building_type_id type) {
	// A tiny positive sentinel distinguishes a newly-started project from idle
	// without granting a visible amount of free construction progress.
	state.world.province_set_civic_building_progress(province, type.index(), 0.000001f);
}

void advance_upgrade(sys::state& state, dcon::province_id province, dcon::civic_building_type_id type,
		float progress_delta) {
	if(!upgrade_in_progress(state, province, type) || progress_delta <= 0.f)
		return;
	auto current_level = state.world.province_get_civic_building_level(province, type.index());
	auto level_count = state.world.civic_building_type_get_level_count(type);
	if(current_level >= level_count) {
		state.world.province_set_civic_building_progress(province, type.index(), 0.f);
		return;
	}
	auto progress = state.world.province_get_civic_building_progress(province, type.index()) + progress_delta;
	if(progress < 1.f) {
		state.world.province_set_civic_building_progress(province, type.index(), progress);
		return;
	}

	auto& levels = state.world.civic_building_type_get_levels(type);
	if(current_level > 0 && levels[current_level - 1].modifier)
		sys::remove_modifier_from_province(state, province, levels[current_level - 1].modifier);
	state.world.province_set_civic_building_level(province, type.index(), uint8_t(current_level + 1));
	state.world.province_set_civic_building_progress(province, type.index(), 0.f);
	if(levels[current_level].modifier)
		sys::add_modifier_to_province(state, province, levels[current_level].modifier, sys::date{});
}

// Civic construction is advanced by construction.cpp only after its market
// goods have actually been delivered. Keep this daily hook as a defensive
// cleanup for invalid/maxed projects; it must never grant free progress.
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
			auto progress = state.world.province_get_civic_building_progress(p, type.id.index());
			if(progress <= 0.f)
				return;

			auto cur_level = state.world.province_get_civic_building_level(p, type.id.index());
			if(cur_level >= level_count)
				return;

			if(progress >= 1.f)
				advance_upgrade(state, p, type.id, 0.000001f);
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
