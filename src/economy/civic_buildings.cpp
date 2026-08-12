#include "civic_buildings.hpp"
#include "system_state.hpp"
#include "triggers.hpp"

namespace civic_buildings {

void initialize_size_of_dcon_arrays(sys::state& state) {
	auto count = int32_t(state.world.civic_building_type_size());
	state.world.province_resize_civic_building_level(count);
	state.world.province_resize_civic_building_progress(count);
	state.world.province_resize_civic_building_active(count);
	state.world.province_resize_civic_building_purchased_goods(count);
}

bool upgrade_in_progress(sys::state& state, dcon::province_id province, dcon::civic_building_type_id type) {
	return province && type
		&& (state.world.province_get_civic_building_active(province, type.index()) != 0
			|| state.world.province_get_civic_building_progress(province, type.index()) > 0.f);
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
	state.world.province_set_civic_building_active(province, type.index(), uint8_t(1));
	state.world.province_set_civic_building_progress(province, type.index(), 0.f);
	state.world.province_set_civic_building_purchased_goods(province, type.index(), economy::commodity_set{});
}

void advance_upgrade(sys::state& state, dcon::province_id province, dcon::civic_building_type_id type,
		float progress_delta) {
	if(!upgrade_in_progress(state, province, type) || progress_delta <= 0.f)
		return;
	auto current_level = state.world.province_get_civic_building_level(province, type.index());
	auto level_count = state.world.civic_building_type_get_level_count(type);
	if(current_level >= level_count) {
		state.world.province_set_civic_building_progress(province, type.index(), 0.f);
		state.world.province_set_civic_building_active(province, type.index(), uint8_t(0));
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
	state.world.province_set_civic_building_active(province, type.index(), uint8_t(0));
	state.world.province_set_civic_building_purchased_goods(province, type.index(), economy::commodity_set{});
	if(levels[current_level].modifier)
		sys::add_modifier_to_province(state, province, levels[current_level].modifier, sys::date{});
	// Roads and other transport-facing civic buildings can change the cheapest
	// paths between markets. Recalculate cached route data after completion.
	if(state.world.civic_building_type_get_is_road_network(type))
		state.trade_route_cached_values_out_of_date = true;
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
			if(!upgrade_in_progress(state, p, type.id))
				return;
			auto progress = state.world.province_get_civic_building_progress(p, type.id.index());

			auto cur_level = state.world.province_get_civic_building_level(p, type.id.index());
			if(cur_level >= level_count) {
				state.world.province_set_civic_building_active(p, type.id.index(), uint8_t(0));
				state.world.province_set_civic_building_progress(p, type.id.index(), 0.f);
				return;
			}

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

int32_t province_urban_building_capacity(sys::state& state, dcon::province_id p) {
	int32_t capacity = 0;
	for(auto type : state.world.in_civic_building_type) {
		if(!type.get_is_urban_center())
			continue;
		auto level = state.world.province_get_civic_building_level(p, type.id.index());
		if(level == 0)
			continue;
		auto& levels = type.get_levels();
		capacity += levels[std::min<uint8_t>(level, type.get_level_count()) - 1].building_capacity;
	}
	return capacity;
}

int32_t province_used_urban_building_capacity(sys::state& state, dcon::province_id p) {
	int32_t used = int32_t(state.world.province_get_factory_location(p).end()
		- state.world.province_get_factory_location(p).begin());
	for(auto construction : state.world.province_get_factory_construction(p)) {
		// A new factory occupies a site immediately so parallel orders cannot
		// overbook the final slot. Upgrades/refits retain their existing site.
		if(!construction.get_is_upgrade() && !construction.get_refit_target())
			++used;
	}
	return used;
}

bool province_has_urban_building_capacity(sys::state& state, dcon::province_id p, int32_t required) {
	return province_used_urban_building_capacity(state, p) + std::max(0, required)
		<= province_urban_building_capacity(state, p);
}

}
