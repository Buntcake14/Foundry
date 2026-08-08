#pragma once
// Standalone "State Ledger" window: at-a-glance economic health for the player's
// own states, reachable via its own topbar button (see gui_minimap.hpp's
// minimap_state_ledger_button) -- deliberately NOT nested inside the vanilla
// Ledger window, so it can be redesigned/reskinned independently later.

#include "gui_common_elements.hpp"
#include "gui_element_types.hpp"
#include "gui_listbox_templates.hpp"
#include "gui_province_window.hpp"
#include "province_templates.hpp"
#include "system_state.hpp"
#include "economy.hpp"

namespace ui {

class state_ledger_name_text : public simple_text_element_base {
public:
	void on_update(sys::state& state) noexcept override {
		auto sid = retrieve<dcon::state_instance_id>(state, parent);
		auto def = state.world.state_instance_get_definition(sid);
		set_text(state, text::get_name_as_string(state, dcon::fatten(state.world, def)));
	}
};

// Population-weighted life-needs satisfaction across pop types actually present in the state.
inline float state_ledger_life_needs_satisfaction(sys::state& state, dcon::state_instance_id sid) {
	auto market = state.world.state_instance_get_market_from_local_market(sid);
	float weighted_sum = 0.f;
	float total_pop = 0.f;
	state.world.for_each_pop_type([&](dcon::pop_type_id pt) {
		auto pop_amount = state.world.state_instance_get_demographics(sid, demographics::to_key(state, pt));
		if(pop_amount <= 0.f)
			return;
		auto ratio = state.world.market_get_satisfied_ratio_of_demanded_life_needs(market, pt);
		weighted_sum += ratio * pop_amount;
		total_pop += pop_amount;
	});
	return total_pop > 0.f ? weighted_sum / total_pop : 1.f;
}

class state_ledger_unmet_demand_text : public simple_text_element_base {
public:
	void on_update(sys::state& state) noexcept override {
		auto sid = retrieve<dcon::state_instance_id>(state, parent);
		auto ratio = state_ledger_life_needs_satisfaction(state, sid);
		auto pct = int32_t(ratio * 100.f);
		set_text(state, pct < 70 ? (std::to_string(pct) + "% LOW") : (std::to_string(pct) + "%"));
	}
};

// Status dot frames: 0 = green, 1 = yellow, 2 = red (see GFX_state_ledger_status_dots).
class state_ledger_life_needs_dot : public image_element_base {
public:
	void on_update(sys::state& state) noexcept override {
		auto sid = retrieve<dcon::state_instance_id>(state, parent);
		auto ratio = state_ledger_life_needs_satisfaction(state, sid);
		frame = ratio < 0.5f ? 2 : (ratio < 0.7f ? 1 : 0);
	}
};

class state_ledger_market_balance_text : public simple_text_element_base {
public:
	void on_update(sys::state& state) noexcept override {
		auto sid = retrieve<dcon::state_instance_id>(state, parent);
		auto market = state.world.state_instance_get_market_from_local_market(sid);
		auto balance = state.world.market_get_stockpile(market, economy::money);
		set_text(state, balance < 0.f ? (text::format_money(balance) + " DEFICIT") : text::format_money(balance));
	}
};

class state_ledger_market_dot : public image_element_base {
public:
	void on_update(sys::state& state) noexcept override {
		auto sid = retrieve<dcon::state_instance_id>(state, parent);
		auto market = state.world.state_instance_get_market_from_local_market(sid);
		auto balance = state.world.market_get_stockpile(market, economy::money);
		frame = balance < 0.f ? 2 : 0;
	}
};

// Fraction of this state's total RGO potential actually being worked, across all goods.
inline float state_ledger_rgo_utilization(sys::state& state, dcon::state_instance_id sid) {
	float total_size = 0.f;
	float total_potential = 0.f;
	province::for_each_province_in_state_instance(state, sid, [&](dcon::province_id p) {
		state.world.for_each_commodity([&](dcon::commodity_id c) {
			total_size += state.world.province_get_rgo_size(p, c);
			total_potential += state.world.province_get_rgo_potential(p, c);
		});
	});
	return total_potential > 0.f ? total_size / total_potential : 1.f;
}

class state_ledger_idle_capacity_text : public simple_text_element_base {
public:
	void on_update(sys::state& state) noexcept override {
		auto sid = retrieve<dcon::state_instance_id>(state, parent);
		auto ratio = state_ledger_rgo_utilization(state, sid);
		auto pct = int32_t(ratio * 100.f);
		set_text(state, pct < 60 ? (std::to_string(pct) + "% IDLE") : (std::to_string(pct) + "%"));
	}
};

class state_ledger_rgo_dot : public image_element_base {
public:
	void on_update(sys::state& state) noexcept override {
		auto sid = retrieve<dcon::state_instance_id>(state, parent);
		auto ratio = state_ledger_rgo_utilization(state, sid);
		frame = ratio < 0.5f ? 2 : (ratio < 0.7f ? 1 : 0);
	}
};

class state_ledger_view_button : public button_element_base {
public:
	void button_action(sys::state& state) noexcept override {
		auto sid = retrieve<dcon::state_instance_id>(state, parent);
		auto capital = state.world.state_instance_get_capital(sid);
		if(!bool(capital))
			return;
		static_cast<ui::province_view_window*>(state.ui_state.province_window)->set_active_province(state, capital);
		send<province_subtab_toggle_signal>(state, state.ui_state.province_window, province_subtab_toggle_signal::market);
	}

	tooltip_behavior has_tooltip(sys::state& state) noexcept override {
		return tooltip_behavior::tooltip;
	}
	void update_tooltip(sys::state& state, int32_t x, int32_t y, text::columnar_layout& contents) noexcept override {
		text::add_line(state, contents, "ledger_state_view_tooltip");
	}
};

class state_ledger_entry : public listbox_row_element_base<dcon::state_instance_id> {
	image_element_base* light_background = nullptr;
	image_element_base* dark_background = nullptr;

public:
	std::unique_ptr<element_base> make_child(sys::state& state, std::string_view name, dcon::gui_def_id id) noexcept override {
		if(name == "state_ledger_row_background") {
			auto ptr = make_element_by_type<image_element_base>(state, id);
			light_background = ptr.get();
			return ptr;
		}
		if(name == "state_ledger_row_background_dark") {
			auto ptr = make_element_by_type<image_element_base>(state, id);
			dark_background = ptr.get();
			return ptr;
		}
		return nullptr;
	}

	// Called once by the listbox right after the row pool is created, so alternating rows get a
	// slightly darker background (matching the striped-grid look of the reference mockup). Only
	// one of the two is ever visible, so stacking order between them doesn't matter.
	void set_striped(sys::state& state, bool dark) noexcept {
		if(light_background)
			light_background->set_visible(state, !dark);
		if(dark_background)
			dark_background->set_visible(state, dark);
	}

	void on_create(sys::state& state) noexcept override {
		listbox_row_element_base::on_create(state);

		xy_pair cell_offset{ int16_t(0), 0 };
		auto cell_width = int16_t(660 / 5);
		auto apply_offset = [&](auto& ptr) {
			ptr->base_data.position = cell_offset;
			ptr->base_data.size.x = cell_width;
			cell_offset.x += cell_width;
		};
		auto dot_def = state.ui_state.defs_by_name.find(state.lookup_key("state_ledger_status_dot"))->second.definition;
		auto dot_width = int16_t(20);
		// A status dot sits at the left of a metric cell; the cell's text is shifted right and
		// narrowed to make room for it, then the cell offset advances as normal.
		auto apply_metric_cell = [&](auto& dot_ptr, auto& text_ptr) {
			dot_ptr->base_data.position = xy_pair{ int16_t(cell_offset.x + 4), int16_t(6) };
			dot_ptr->base_data.size = xy_pair{ dot_width, int16_t(16) };
			add_child_to_front(std::move(dot_ptr));

			text_ptr->base_data.position = xy_pair{ int16_t(cell_offset.x + dot_width + 8), int16_t(6) };
			text_ptr->base_data.size.x = int16_t(cell_width - dot_width - 8);
			add_child_to_front(std::move(text_ptr));

			cell_offset.x += cell_width;
		};
		{
			auto ptr = make_element_by_type<state_ledger_name_text>(state,
					state.ui_state.defs_by_name.find(state.lookup_key("ledger_default_textbox"))->second.definition);
			apply_offset(ptr);
			ptr->base_data.position.x += int16_t(20);
			ptr->base_data.size.x -= int16_t(20);
			ptr->base_data.position.y = int16_t(6);
			add_child_to_front(std::move(ptr));
		}
		{
			auto dot = make_element_by_type<state_ledger_life_needs_dot>(state, dot_def);
			auto ptr = make_element_by_type<state_ledger_unmet_demand_text>(state,
					state.ui_state.defs_by_name.find(state.lookup_key("ledger_default_textbox"))->second.definition);
			apply_metric_cell(dot, ptr);
		}
		{
			auto dot = make_element_by_type<state_ledger_market_dot>(state, dot_def);
			auto ptr = make_element_by_type<state_ledger_market_balance_text>(state,
					state.ui_state.defs_by_name.find(state.lookup_key("ledger_default_textbox"))->second.definition);
			apply_metric_cell(dot, ptr);
		}
		{
			auto dot = make_element_by_type<state_ledger_rgo_dot>(state, dot_def);
			auto ptr = make_element_by_type<state_ledger_idle_capacity_text>(state,
					state.ui_state.defs_by_name.find(state.lookup_key("ledger_default_textbox"))->second.definition);
			apply_metric_cell(dot, ptr);
		}
		{
			// The button's own art is a fixed 152px-wide graphic; center it in the cell rather
			// than stretching it to the full (wider) cell width like the plain text cells.
			auto ptr = make_element_by_type<state_ledger_view_button>(state,
					state.ui_state.defs_by_name.find(state.lookup_key("state_ledger_view_button_bg"))->second.definition);
			auto button_width = int16_t(152);
			ptr->base_data.position = xy_pair{ int16_t(cell_offset.x + (cell_width - button_width) / 2), 0 };
			cell_offset.x += cell_width;
			add_child_to_front(std::move(ptr));
		}
	}

	message_result get(sys::state& state, Cyto::Any& payload) noexcept override {
		if(payload.holds_type<dcon::state_instance_id>()) {
			payload.emplace<dcon::state_instance_id>(content);
			return message_result::consumed;
		}
		return listbox_row_element_base::get(state, payload);
	}

	void on_update(sys::state& state) noexcept override {
		Cyto::Any payload = content;
		impl_set(state, payload);
	}
};

class state_ledger_listbox : public listbox_element_base<state_ledger_entry, dcon::state_instance_id> {
protected:
	std::string_view get_row_element_name() override {
		return "state_ledger_row";
	}

public:
	void on_create(sys::state& state) noexcept override {
		listbox_element_base::on_create(state);
		for(size_t i = 0; i < row_windows.size(); ++i)
			row_windows[i]->set_striped(state, (i % 2) == 1);
	}

	void on_update(sys::state& state) noexcept override {
		row_contents.clear();
		auto player_nation = state.local_player_nation;
		state.world.nation_for_each_state_ownership(player_nation, [&](auto soid) {
			row_contents.push_back(state.world.state_ownership_get_state(soid));
		});
		update(state);
	}
};

class state_ledger_close_button : public button_element_base {
public:
	void button_action(sys::state& state) noexcept override {
		parent->set_visible(state, false);
	}
};

class state_ledger_window : public window_element_base {
	dcon::gui_def_id listbox_def_id{};
	state_ledger_listbox* listbox = nullptr;

public:
	void on_create(sys::state& state) noexcept override {
		window_element_base::on_create(state);
		auto ptr = make_element_by_type<state_ledger_listbox>(state, listbox_def_id);
		listbox = ptr.get();
		add_child_to_front(std::move(ptr));
	}

	std::unique_ptr<element_base> make_child(sys::state& state, std::string_view name, dcon::gui_def_id id) noexcept override {
		if(name == "background") {
			return make_element_by_type<draggable_target>(state, id);
		} else if(name == "close_button") {
			return make_element_by_type<state_ledger_close_button>(state, id);
		} else if(name == "state_ledger_listbox_area") {
			listbox_def_id = id;
			return make_element_by_type<invisible_element>(state, id);
		} else {
			return nullptr;
		}
	}

	void on_update(sys::state& state) noexcept override {
		listbox->impl_on_update(state);
	}
};

}
