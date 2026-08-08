#include "parsers_declarations.hpp"
#include "civic_buildings.hpp"
#include "text.hpp"

namespace parsers {

dcon::trigger_key make_civic_building_trigger(token_generator& gen, error_handler& err, scenario_building_context& context) {
	trigger_building_context t_context{context, trigger::slot_contents::province, trigger::slot_contents::nation,
			trigger::slot_contents::empty};
	return make_trigger(gen, err, t_context);
}

void civic_building_level::modifier(association_type, std::string_view value, error_handler& err, int32_t line, scenario_building_context& context) {
	if(auto it = context.map_of_modifiers.find(std::string(value)); it != context.map_of_modifiers.end()) {
		modifier_id_ = it->second;
	} else {
		err.accumulated_errors +=
			"Invalid modifier " + std::string(value) + " (" + err.file_name + " line " + std::to_string(line) + ")\n";
	}
}

void civic_building_definition::level(civic_building_level&& value, error_handler& err, int32_t line, scenario_building_context& context) {
	if(levels.size() >= size_t(civic_buildings::max_levels)) {
		err.accumulated_errors += "Too many levels on a civic building (" + err.file_name + " line " + std::to_string(line) + ")\n";
		return;
	}
	levels.push_back(std::move(value));
}

void civic_buildings_file::result(std::string_view name, civic_building_definition&& res, error_handler& err, int32_t line,
		scenario_building_context& context) {
	auto new_id = context.state.world.create_civic_building_type();
	context.state.world.civic_building_type_set_name(new_id, text::find_or_add_key(context.state, name, false));
	context.state.world.civic_building_type_set_level_count(new_id, uint8_t(res.levels.size()));
	context.state.world.civic_building_type_set_is_urban_center(new_id, res.is_urban_center);
	context.state.world.civic_building_type_set_is_rgo_level(new_id, res.is_rgo_level);

	auto& stored_levels = context.state.world.civic_building_type_get_levels(new_id);
	for(size_t i = 0; i < res.levels.size(); ++i) {
		auto& parsed = res.levels[i];
		auto& stored = stored_levels[i];
		stored.requirement = parsed.requirement;
		stored.modifier = parsed.modifier_id_;
		stored.construction_time = int16_t(parsed.time);
		stored.rgo_output_penalty = parsed.rgo_output_penalty;
		stored.unlocks_factories = parsed.unlocks_factories;
		stored.unlocks_admin_buildings = parsed.unlocks_admin_buildings;

		uint32_t added = 0;
		context.state.world.for_each_commodity([&](dcon::commodity_id id) {
			auto amount = parsed.cost.data.safe_get(id);
			if(amount > 0) {
				if(added >= economy::commodity_set::set_size) {
					err.accumulated_warnings += "Too many civic building cost goods in " + std::string(name) + " (" + err.file_name + ")\n";
				} else {
					stored.cost.commodity_type[added] = id;
					stored.cost.commodity_amounts[added] = amount;
					++added;
				}
			}
		});
	}
}

}
