#pragma once

#include <variant>
#include "gui_element_types.hpp"
#include "gui_project_investment_window.hpp"
#include "gui_production_enum.hpp"
#include "economy_stats.hpp"
#include "construction.hpp"
#include "civic_buildings.hpp"

namespace ui {

class foundry_construction_capacity_text : public simple_text_element_base {
public:
	void on_update(sys::state& state) noexcept override {
		auto capacity = economy::national_construction_capacity(state, state.local_player_nation);
		auto active = economy::active_civil_construction_projects(state, state.local_player_nation);
		auto queued = economy::queued_civil_construction_projects(state, state.local_player_nation);
		auto stockpiling = economy::stockpiling_civil_construction_projects(state, state.local_player_nation);
		set_text(state, "Construction Capacity: " + text::format_float(capacity.total, 1)
			+ "     Stockpiling: " + std::to_string(stockpiling)
			+ "     Active: " + std::to_string(active)
			+ "     Queued: " + std::to_string(queued));
	}

	tooltip_behavior has_tooltip(sys::state& state) noexcept override {
		return tooltip_behavior::variable_tooltip;
	}

	void update_tooltip(sys::state& state, int32_t, int32_t, text::columnar_layout& contents) noexcept override {
		auto capacity = economy::national_construction_capacity(state, state.local_player_nation);
		auto add_breakdown = [&](std::string const& label, float value) {
			auto box = text::open_layout_box(contents, 0);
			text::add_unparsed_text_to_layout_box(state, contents, box, label + ": " + text::format_float(value, 1));
			text::close_layout_box(contents, box);
		};
		add_breakdown("Unskilled labor capacity", capacity.unskilled);
		add_breakdown("Skilled labor capacity", capacity.skilled);
		add_breakdown("Educated labor capacity", capacity.educated);
		add_breakdown("Total after local urban and infrastructure modifiers", capacity.total);
		auto box = text::open_layout_box(contents, 0);
		text::add_unparsed_text_to_layout_box(state, contents, box,
			"Projects may stockpile goods without using construction capacity. Once fully supplied, each capacity point supports one civil project at normal speed. A fractional remainder partially supports the next project; later supplied projects wait in the queue.");
		text::close_layout_box(contents, box);
		box = text::open_layout_box(contents, 0);
		text::add_unparsed_text_to_layout_box(state, contents, box,
			"Current allocation order follows this list: factories first, then provincial buildings. Military construction does not consume civil capacity.");
		text::close_layout_box(contents, box);
	}
};

typedef std::variant<dcon::province_building_construction_id, dcon::factory_construction_id,
	economy::civic_construction_project, economy::rgo_construction_project> production_project_data;

struct production_project_input_data {
	dcon::commodity_id cid{};
	float satisfied = 0.f;
	float needed = 0.f;
};

inline std::string format_project_quantity(float value) {
	value = std::max(0.f, value);
	if(value < 0.05f)
		value = 0.f;
	if(value >= 1'000.f)
		return text::format_float(value / 1'000.f, value >= 10'000.f ? 0 : 1) + "K";
	return text::format_float(value, value >= 100.f ? 0 : 1);
}

class production_project_input_item : public listbox_row_element_base<production_project_input_data> {
	simple_text_element_base* amount_text = nullptr;
	element_base* commodity_icon = nullptr;

public:
	void on_create(sys::state& state) noexcept override {
		listbox_row_element_base<production_project_input_data>::on_create(state);
		amount_text->base_data.position.y = commodity_icon->base_data.position.y + commodity_icon->base_data.size.y - 4;
	}

	std::unique_ptr<element_base> make_child(sys::state& state, std::string_view name, dcon::gui_def_id id) noexcept override {
		if(name == "goods_type") {
			auto ptr = make_element_by_type<commodity_image>(state, id);
			commodity_icon = ptr.get();
			return ptr;
		} else if(name == "goods_amount") {
			auto ptr = make_element_by_type<simple_text_element_base>(state, id);
			amount_text = ptr.get();
			return ptr;
		} else {
			return nullptr;
		}
	}

	void on_update(sys::state& state) noexcept override {
		amount_text->set_text(state, format_project_quantity(content.satisfied) + "/" + format_project_quantity(content.needed));
	}

	message_result get(sys::state& state, Cyto::Any& payload) noexcept override {
		if(payload.holds_type<dcon::commodity_id>()) {
			payload.emplace<dcon::commodity_id>(content.cid);
			return message_result::consumed;
		}
		return listbox_row_element_base<production_project_input_data>::get(state, payload);
	}
};

class production_project_input_listbox
		: public overlapping_listbox_element_base<production_project_input_item, production_project_input_data> {
protected:
	std::string_view get_row_element_name() override {
		return "goods_need_template";
	}
public:
	void on_create(sys::state& state) noexcept override {
		overlapping_listbox_element_base<production_project_input_item, production_project_input_data>::on_create(state);
		// Keep even five-good construction recipes inside the Completion column
		// instead of allowing the legacy wide spacing to cover Investors.
		base_data.position.x = 550;
		base_data.size.x = 310;
		base_data.data.overlapping.spacing = 12.f;
	}
};

class production_project_invest_button : public button_element_base {
public:
	void on_create(sys::state& state) noexcept override {
		button_element_base::on_create(state);
		set_visible(state, false);
	}
};

class production_project_info : public listbox_row_element_base<production_project_data> {
	image_element_base* building_icon = nullptr;
	image_element_base* factory_icon = nullptr;
	simple_text_element_base* name_text = nullptr;
	simple_text_element_base* cost_text = nullptr;
	simple_text_element_base* funder_text = nullptr;
	simple_text_element_base* capacity_status_text = nullptr;
	production_project_input_listbox* input_listbox = nullptr;

	dcon::state_instance_id get_state_instance_id(sys::state& state) {
		if(std::holds_alternative<dcon::province_building_construction_id>(content)) {
			auto fat_id = dcon::fatten(state.world, std::get<dcon::province_building_construction_id>(content));
			return fat_id.get_province().get_state_membership().id;
		} else if(std::holds_alternative<dcon::factory_construction_id>(content)) {
			auto fat_id = dcon::fatten(state.world, std::get<dcon::factory_construction_id>(content));
			return fat_id.get_province().get_state_membership().id;
		} else if(std::holds_alternative<economy::civic_construction_project>(content)) {
			auto project = std::get<economy::civic_construction_project>(content);
			return state.world.province_get_state_membership(project.province);
		} else if(std::holds_alternative<economy::rgo_construction_project>(content)) {
			return state.world.province_get_state_membership(
				std::get<economy::rgo_construction_project>(content).province);
		}
		return dcon::state_instance_id{};
	}

	float get_cost(sys::state& state, economy::commodity_set const& cset) {
		float total = 0.f;
		auto s = get_state_instance_id(state);
		for(uint32_t i = 0; i < economy::commodity_set::set_size; ++i) {
			dcon::commodity_id cid = cset.commodity_type[i];
			if(bool(cid))
				total += economy::price(state, s, cid) * cset.commodity_amounts[i];
		}
		return total;
	}

public:
	std::unique_ptr<element_base> make_child(sys::state& state, std::string_view name, dcon::gui_def_id id) noexcept override {
		if(name == "state_bg") {
			return make_element_by_type<image_element_base>(state, id);
		} else if(name == "state_name") {
			return make_element_by_type<state_name_text>(state, id);
		} else if(name == "project_icon") {
			auto ptr = make_element_by_type<image_element_base>(state, id);
			factory_icon = ptr.get();
			return ptr;
		} else if(name == "infra") {
			auto ptr = make_element_by_type<image_element_base>(state, id);
			building_icon = ptr.get();
			return ptr;
		} else if(name == "project_name") {
			auto ptr = make_element_by_type<simple_text_element_base>(state, id);
			name_text = ptr.get();
			return ptr;
		} else if(name == "project_cost") {
			auto ptr = make_element_by_type<simple_text_element_base>(state, id);
			cost_text = ptr.get();
			return ptr;
		} else if(name == "pop_icon") {
			return make_element_by_type<invisible_element>(state, id);
		} else if(name == "pop_amount") {
			auto ptr = make_element_by_type<simple_text_element_base>(state, id);
			ptr->base_data.position.x = 865;
			ptr->base_data.size.x = 95;
			funder_text = ptr.get();
			return ptr;
		} else if(name == "capacity_status") {
			auto ptr = make_element_by_type<simple_text_element_base>(state, id);
			capacity_status_text = ptr.get();
			return ptr;
		} else if(name == "invest_project") {
			return make_element_by_type<production_project_invest_button>(state, id);
		} else if(name == "input_goods") {
			auto ptr = make_element_by_type<production_project_input_listbox>(state, id);
			input_listbox = ptr.get();
			return ptr;
		} else {
			return nullptr;
		}
	}

	void on_update(sys::state& state) noexcept override {
		economy::commodity_set satisfied_commodities{};
		economy::commodity_set needed_commodities{};
		bool private_project = false;
		float capacity_share = 0.f;
		float build_progress = 0.f;
		if(std::holds_alternative<dcon::province_building_construction_id>(content)) {
			factory_icon->set_visible(state, false);
			building_icon->set_visible(state, true);
			auto fat_id = dcon::fatten(state.world, std::get<dcon::province_building_construction_id>(content));
			capacity_share = economy::civil_construction_capacity_share(state, fat_id.id);
			private_project = fat_id.get_is_pop_project();
			auto type = economy::province_building_type(fat_id.get_type());
			uint16_t project_frame = 6;
			switch(type) {
			case economy::province_building_type::railroad: project_frame = 0; break;
			case economy::province_building_type::fort: project_frame = 1; break;
			case economy::province_building_type::naval_base: project_frame = 2; break;
			default: break;
			}
			auto it = state.ui_state.gfx_by_name.find(state.lookup_key("GFX_foundry_project_icons"));
			if(it != state.ui_state.gfx_by_name.end())
				building_icon->base_data.data.image.gfx_object = it->second;
			building_icon->frame = project_frame;
			name_text->set_text(state, text::produce_simple_string(state,  province_building_type_get_name(economy::province_building_type(fat_id.get_type()))));
			
			needed_commodities = state.economy_definitions.building_definitions[int32_t(type)].cost;

			satisfied_commodities = fat_id.get_purchased_goods();
			build_progress = fat_id.get_build_progress();
		} else if(std::holds_alternative<dcon::factory_construction_id>(content)) {
			factory_icon->set_visible(state, true);
			building_icon->set_visible(state, false);
			auto fat_id = dcon::fatten(state.world, std::get<dcon::factory_construction_id>(content));
			capacity_share = economy::civil_construction_capacity_share(state, fat_id.id);
			private_project = fat_id.get_is_pop_project();
			auto it = state.ui_state.gfx_by_name.find(state.lookup_key("GFX_foundry_project_icons"));
			if(it != state.ui_state.gfx_by_name.end())
				factory_icon->base_data.data.image.gfx_object = it->second;
			factory_icon->frame = 3;
			name_text->set_text(state, text::produce_simple_string(state, fat_id.get_type().get_name()));
			needed_commodities = fat_id.get_type().get_construction_costs();
			satisfied_commodities = fat_id.get_purchased_goods();
			build_progress = fat_id.get_build_progress();

			float factory_mod = economy::factory_build_cost_multiplier(state, state.local_player_nation, fat_id.get_province(), fat_id.get_is_pop_project());
			float refit_discount = (fat_id.get_refit_target()) ? state.defines.alice_factory_refit_cost_modifier : 1.0f;

			for(uint32_t i = 0; i < economy::commodity_set::set_size; ++i) {
				needed_commodities.commodity_amounts[i] *= factory_mod * refit_discount;
			}
		} else if(std::holds_alternative<economy::civic_construction_project>(content)) {
			auto project = std::get<economy::civic_construction_project>(content);
			capacity_share = economy::civil_construction_capacity_share(state, project);
			private_project = false;
			factory_icon->set_visible(state, true);
			building_icon->set_visible(state, false);
			auto it = state.ui_state.gfx_by_name.find(state.lookup_key("GFX_foundry_project_icons"));
			if(it != state.ui_state.gfx_by_name.end())
				factory_icon->base_data.data.image.gfx_object = it->second;
			bool const urban_center = state.world.civic_building_type_get_is_urban_center(project.type);
			bool const road_network = state.world.civic_building_type_get_is_road_network(project.type);
			factory_icon->frame = urban_center ? 5 : (road_network ? 0 : 5);
			auto current_level = state.world.province_get_civic_building_level(project.province, project.type.index());
			name_text->set_text(state, std::string(urban_center ? "Urban Center Level "
				: (road_network ? "Road Network Level " : "Civic Building Level "))
				+ std::to_string(int32_t(current_level) + 1));
			auto& definition = state.world.civic_building_type_get_levels(project.type)[current_level];
			needed_commodities = definition.cost;
			satisfied_commodities = state.world.province_get_civic_building_purchased_goods(
				project.province, project.type.index());
			build_progress = state.world.province_get_civic_building_progress(
				project.province, project.type.index());
		} else if(std::holds_alternative<economy::rgo_construction_project>(content)) {
			auto project = std::get<economy::rgo_construction_project>(content);
			capacity_share = economy::civil_construction_capacity_share(state, project);
			private_project = state.world.province_get_rgo_upgrade_sponsor(project.province, project.commodity)
				== uint8_t(economy::rgo_upgrade_sponsor::private_investors);
			factory_icon->set_visible(state, true);
			building_icon->set_visible(state, false);
			auto it = state.ui_state.gfx_by_name.find(state.lookup_key("GFX_foundry_project_icons"));
			if(it != state.ui_state.gfx_by_name.end()) factory_icon->base_data.data.image.gfx_object = it->second;
			factory_icon->frame = 4;
			name_text->set_text(state, text::produce_simple_string(state,
				state.world.commodity_get_name(project.commodity)) + " RGO Upgrade");
			needed_commodities = economy::rgo_upgrade_goods_cost(state, project.province, project.commodity);
			satisfied_commodities = state.world.province_get_rgo_upgrade_purchased_goods(
				project.province, project.commodity);
			build_progress = state.world.province_get_rgo_level_progress(project.province, project.commodity);
		}
		if(funder_text)
			funder_text->set_text(state, private_project ? "Private" : "Government");
		if(capacity_status_text) {
			bool goods_ready = true;
			for(uint32_t i = 0; i < economy::commodity_set::set_size; ++i) {
				if(!needed_commodities.commodity_type[i]) break;
				if(satisfied_commodities.commodity_amounts[i] + 0.0001f < needed_commodities.commodity_amounts[i]) {
					goods_ready = false;
					break;
				}
			}
			if(!goods_ready)
				capacity_status_text->set_text(state, "Acquiring Goods");
			else if(capacity_share <= 0.f)
				capacity_status_text->set_text(state, "Supplied - Queued");
			else if(capacity_share >= 0.999f)
				capacity_status_text->set_text(state,
					"Building " + text::format_percentage(std::clamp(build_progress, 0.f, 1.f), 0));
			else
				capacity_status_text->set_text(state,
					"Building " + text::format_percentage(std::clamp(build_progress, 0.f, 1.f), 0)
					+ " (" + std::to_string(int32_t(capacity_share * 100.f + 0.5f)) + "% Capacity)");
		}

		if(input_listbox) {
			input_listbox->row_contents.clear();
			for(uint32_t i = 0; i < economy::commodity_set::set_size; ++i)
				if(bool(needed_commodities.commodity_type[i]))
					input_listbox->row_contents.push_back(production_project_input_data{
							needed_commodities.commodity_type[i],				// cid
							satisfied_commodities.commodity_amounts[i], // satisfied
							needed_commodities.commodity_amounts[i]			// needed
					});
			input_listbox->update(state);
		}

		auto s = get_state_instance_id(state);

		float purchased_cost = 0.0f;
		for(uint32_t i = 0; i < economy::commodity_set::set_size; ++i) {
			dcon::commodity_id cid = needed_commodities.commodity_type[i];
			if(bool(cid))
				purchased_cost += economy::price(state, s, cid) * satisfied_commodities.commodity_amounts[i];
		}
		float total_cost = get_cost(state, needed_commodities);
		cost_text->set_text(state, text::format_money(purchased_cost) + "/" + text::format_money(total_cost));
	}

	message_result get(sys::state& state, Cyto::Any& payload) noexcept override {
		if(payload.holds_type<dcon::state_instance_id>()) {
			payload.emplace<dcon::state_instance_id>(get_state_instance_id(state));
			return message_result::consumed;
		}
		return listbox_row_element_base<production_project_data>::get(state, payload);
	}
};

class production_project_listbox : public listbox_element_base<production_project_info, production_project_data> {
protected:
	std::string_view get_row_element_name() override {
		return "project_info";
	}

public:
	void on_update(sys::state& state) noexcept override {
		row_contents.clear();
		state.world.nation_for_each_factory_construction_as_nation(state.local_player_nation,
				[&](dcon::factory_construction_id id) {
					row_contents.push_back(production_project_data(id));
				});
		state.world.nation_for_each_province_building_construction_as_nation(state.local_player_nation,
				[&](dcon::province_building_construction_id id) {
					row_contents.push_back(production_project_data(id));
				});
		state.world.nation_for_each_province_ownership(state.local_player_nation, [&](auto ownership) {
			auto province = state.world.province_ownership_get_province(ownership);
			for(auto type : state.world.in_civic_building_type)
				if(civic_buildings::upgrade_in_progress(state, province, type.id))
					row_contents.push_back(production_project_data(
						economy::civic_construction_project{province, type.id}));
			for(auto commodity : state.world.in_commodity)
				if(economy::rgo_upgrade_in_progress(state, province, commodity))
					row_contents.push_back(production_project_data(
						economy::rgo_construction_project{province, commodity}));
		});
		update(state);
	}
};
} // namespace ui
