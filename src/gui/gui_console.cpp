#include <string>
#include <string_view>
#include <variant>
#include <filesystem>
#include "gui_console.hpp"
#include "gui_fps_counter.hpp"
#include "nations.hpp"
#include "fif.hpp"
#include "fif_dcon_generated.hpp"
#include "fif_common.hpp"
#include "gui_element_base.hpp"
#include "gui_templates.hpp"
#include "constants_ui.hpp"
#include "foundry_transport_shadow.hpp"
#include "economy_stats.hpp"
#include "construction.hpp"
#include "ai_economy.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION 1
#include "stb_image_write.h"


void ui::console_window::on_create(sys::state& state) noexcept {
	window_element_base::on_create(state);
	set_visible(state, false);
}

std::unique_ptr<ui::element_base> ui::console_window::make_child(sys::state& state, std::string_view name, dcon::gui_def_id id) noexcept {
	if(name == "console_list") {
		auto ptr = make_element_by_type<console_list>(state, id);
		console_output_list = ptr.get();
		return ptr;
	} else if(name == "console_edit") {
		auto ptr = make_element_by_type<console_edit>(state, id);
		edit_box = ptr.get();
		return ptr;
	} else {
		return nullptr;
	}
}

ui::message_result ui::console_window::get(sys::state& state, Cyto::Any& payload) noexcept {
	if(payload.holds_type<std::string>()) {
		auto entry = any_cast<std::string>(payload);
		console_output_list->raw_text += entry + "\\n";
		console_output_list->text_pending = true;
		console_output_list->impl_on_update(state);
		return message_result::consumed;
	} else if(payload.holds_type<console_edit*>()) {
		//console_output_list->scroll_to_bottom(state);
		return message_result::consumed;
	} else {
		return message_result::unseen;
	}
}

void ui::console_window::clear_list(sys::state& state) noexcept {
	console_output_list->raw_text.clear();
	console_output_list->impl_on_update(state);
}

void ui::console_window::replace_list(sys::state& state, std::string text) noexcept {
	console_output_list->raw_text = std::move(text);
	console_output_list->text_pending = true;
	// Console commands run while the edit box is still handling keyboard input.
	// Rebuilding the shared HarfBuzz layout synchronously here can re-enter text
	// shaping and corrupt HarfBuzz's temporary buffer state. Let the ordinary UI
	// update pass shape the replacement on the following frame instead.
}

void ui::console_window::on_visible(sys::state& state) noexcept {
	//console_output_list->scroll_to_bottom(state);
	state.ui_state.set_focus_target(state, edit_box);
}
void ui::console_window::on_hide(sys::state& state) noexcept {
	state.ui_state.set_focus_target(state, nullptr);
}

void ui::console_list::on_update(sys::state & state) noexcept {
	constexpr size_t maximum_console_history_bytes = 32768;
	std::string new_content;
	{
		std::lock_guard lg{ state.lock_console_strings };
		new_content = state.console_command_result;
		state.console_command_result.clear();
	}
	if(new_content.size() > 0) {
		raw_text += new_content;
		if(raw_text.size() > maximum_console_history_bytes) {
			auto trim_from = raw_text.find('\n', raw_text.size() - maximum_console_history_bytes);
			if(trim_from != std::string::npos) raw_text.erase(0, trim_from + 1);
			else raw_text.erase(0, raw_text.size() - maximum_console_history_bytes);
		}
		text_pending = true;
	}
	if(text_pending) {
		text_pending = false;
		auto contents = text::create_endless_layout(state, delegate->internal_layout,
			text::layout_parameters{ 10, 10, int16_t(base_data.size.x), int16_t(base_data.size.y),
			base_data.data.text.font_handle, 0, text::alignment::left,
			text::is_black_from_font_id(base_data.data.text.font_handle) ? text::text_color::black : text::text_color::white, false });
		auto box = text::open_layout_box(contents);
		text::add_unparsed_text_to_layout_box(state, contents, box, raw_text);
		text::close_layout_box(contents, box);
		calibrate_scrollbar(state);
	}
}

void log_to_console(sys::state& state, ui::element_base* parent, std::u16string_view s) noexcept {
	Cyto::Any output = simple_fs::utf16_to_utf8(s);
	parent->impl_get(state, output);
}

void ui::console_edit::render(sys::state& state, int32_t x, int32_t y) noexcept {
	ui::edit_box_element_base::render(state, x, y);
}

void ui::console_edit::edit_box_update(sys::state& state, std::u16string_view s) noexcept {
}

void ui::console_edit::on_edit_command(sys::state& state, edit_command command, sys::key_modifiers mods) noexcept {
	if(command == ui::edit_command::cursor_up) {
		std::string up = up_history();
		if(!up.empty()) {
			set_text(state, simple_fs::utf8_to_utf16(up));
		}
		return;
	} else if(command == ui::edit_command::cursor_down) {
		std::string down = down_history();
		if(!down.empty()) {
			set_text(state, simple_fs::utf8_to_utf16(down));
		}
		return;
	} else if(command == ui::edit_command::new_line) {
		if(cached_text.empty()) {
			return;
		}

		if(state.ui_state.shift_held_down) {
			log_to_console(state, parent, cached_text);

			std::lock_guard lg{ state.lock_console_strings };
			state.console_command_pending += simple_fs::utf16_to_utf8(cached_text);
			state.console_command_pending += " ";
		} else {
			log_to_console(state, parent, cached_text);

			std::lock_guard lg{ state.lock_console_strings };
			state.console_command_pending += simple_fs::utf16_to_utf8(cached_text);
			add_to_history(state, state.console_command_pending);
			command::notify_console_command(state);
			set_text(state, u"");
		}
		return;
	}
	edit_box_element_base::on_edit_command(state, command, mods);
}

void ui::console_edit::on_text(sys::state& state, char32_t ch) noexcept {
	if(ch == u'`' || ch == u'\\') {
		// Console visibility is controlled by the key-down hotkey. Treat the
		// corresponding text event only as a character to discard; toggling here
		// makes one physical key press open and immediately close the window.
		return;
	}
	edit_box_element_base::on_text(state, ch);
}


template<typename F>
void write_single_component(sys::state& state, native_string_view filename, F&& func) {
	auto sdir = simple_fs::get_or_create_oos_directory();
	auto buffer = std::unique_ptr<uint8_t[]>(new uint8_t[sys::sizeof_scenario_section(state).total_size]);
	auto buffer_position = func(buffer.get(), state);
	size_t total_size_used = reinterpret_cast<uint8_t*>(buffer_position) - buffer.get();
	simple_fs::write_file(sdir, filename, reinterpret_cast<char*>(buffer.get()), uint32_t(total_size_used));
}

int32_t* f_clear(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode))
		return p + 2;

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	if(state->ui_state.console_window) {
		static_cast<ui::console_window*>(state->ui_state.console_window)->clear_list(*state);
	}

	return p + 2;
}

int32_t* f_fps(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	if(!state->ui_state.fps_counter) {
		auto fps_counter = ui::make_element_by_type<ui::fps_counter_text_box>(*state, "fps_counter");
		state->ui_state.fps_counter = fps_counter.get();
		state->ui_state.root->add_child_to_front(std::move(fps_counter));
	}

	if(s.main_data_back(0) != 0) {
		state->ui_state.fps_counter->set_visible(*state, true);
		state->ui_state.root->move_child_to_front(state->ui_state.fps_counter);
	} else {
		state->ui_state.fps_counter->set_visible(*state, false);
	}

	s.pop_main();
	return p + 2;
}

int32_t* f_change_tag(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	dcon::nation_id to_nation;
	to_nation.value = dcon::nation_id::value_base_t(s.main_data_back(0));
	if(to_nation && to_nation != state->local_player_nation && to_nation != state->world.national_identity_get_nation_from_identity_holder(state->national_definitions.rebel_id)) {
		nations::switch_all_players(*state, to_nation, state->local_player_nation);
	}

	s.pop_main();
	return p + 2;
}

int32_t* f_spectate(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	dcon::nation_id to_nation = state->world.national_identity_get_nation_from_identity_holder(state->national_definitions.rebel_id);

	if(to_nation && to_nation != state->local_player_nation) {
		nations::switch_all_players(*state, to_nation, state->local_player_nation);
	}

	return p + 2;
}

int32_t* f_set_westernized(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	bool westernize_status = s.main_data_back(0) != 0;
	s.pop_main();

	dcon::nation_id to_nation;
	to_nation.value = dcon::nation_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	if(westernize_status && state->world.nation_get_is_civilized(to_nation) == false)
		nations::make_civilized(*state, to_nation);
	else if(!westernize_status && state->world.nation_get_is_civilized(to_nation) == true)
		nations::make_uncivilized(*state, to_nation);

	return p + 2;
}

int32_t* f_make_crisis(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	if(state->current_crisis_state == sys::crisis_state::inactive) {
		state->last_crisis_end_date = sys::date{};
		nations::update_flashpoint_tags(*state);
		nations::monthly_flashpoint_update(*state);
		nations::daily_update_flashpoint_tension(*state);
		float max_tension = 0.0f;
		dcon::state_instance_id max_state;
		for(auto si : state->world.in_state_instance) {
			if(si.get_flashpoint_tension() > max_tension && si.get_nation_from_state_ownership().get_is_at_war() == false && si.get_flashpoint_tag().get_nation_from_identity_holder().get_is_at_war() == false) {
				max_tension = si.get_flashpoint_tension();
				max_state = si;
			}
		}
		if(!max_state) {
			for(auto si : state->world.in_state_instance) {
				if(si.get_flashpoint_tag() && !si.get_nation_from_state_ownership().get_is_great_power() && si.get_nation_from_state_ownership().get_is_at_war() == false && si.get_flashpoint_tag().get_nation_from_identity_holder().get_is_at_war() == false) {
					max_state = si;
					break;
				}
			}
		}
		assert(max_state);
		state->world.state_instance_set_flashpoint_tension(max_state, 10000.0f / state->defines.crisis_base_chance);
		nations::update_crisis(*state);
	}

	return p + 2;
}

int32_t* f_set_mil(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	float mvalue = 0.0f;
	auto ivalue = s.main_data_back(0);
	memcpy(&mvalue, &ivalue, 4);
	s.pop_main();

	dcon::nation_id to_nation;
	to_nation.value = dcon::nation_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	for(auto pr : state->world.nation_get_province_ownership(to_nation))
		for(auto pop : pr.get_province().get_pop_location())
			pop_demographics::set_militancy(*state, pop.get_pop().id, mvalue);

	return p + 2;
}
int32_t* f_set_con(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	float mvalue = 0.0f;
	auto ivalue = s.main_data_back(0);
	memcpy(&mvalue, &ivalue, 4);
	s.pop_main();

	dcon::nation_id to_nation;
	to_nation.value = dcon::nation_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	for(auto pr : state->world.nation_get_province_ownership(to_nation))
		for(auto pop : pr.get_province().get_pop_location())
			pop_demographics::set_consciousness(*state, pop.get_pop().id, mvalue);

	return p + 2;
}

int32_t* f_make_allied(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	dcon::nation_id to_nation;
	to_nation.value = dcon::nation_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	dcon::nation_id to_nation_b;
	to_nation_b.value = dcon::nation_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	nations::make_alliance(*state, to_nation, to_nation_b);

	return p + 2;
}

int32_t* f_end_game(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	game_scene::switch_scene(*state, game_scene::scene_id::end_screen);

	return p + 2;
}

int32_t* f_dump_oos(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* stateptr = (sys::state*)(state_global->data);
	sys::state& state = *stateptr;

	window::change_cursor(state, window::cursor_type::busy);
	state.debug_save_oos_dump();
	state.debug_scenario_oos_dump();
	// Extneded data NOT included in normal dumps
	write_single_component(state, NATIVE("map_data.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::memcpy_serialize(ptr_in, state.map_state.map_data.size_x);
		ptr_in = sys::memcpy_serialize(ptr_in, state.map_state.map_data.size_y);
		ptr_in = sys::serialize(ptr_in, state.map_state.map_data.river_vertices);
		ptr_in = sys::serialize(ptr_in, state.map_state.map_data.river_starts);
		ptr_in = sys::serialize(ptr_in, state.map_state.map_data.river_counts);
		ptr_in = sys::serialize(ptr_in, state.map_state.map_data.coastal_vertices);
		ptr_in = sys::serialize(ptr_in, state.map_state.map_data.coastal_starts);
		ptr_in = sys::serialize(ptr_in, state.map_state.map_data.coastal_counts);
		ptr_in = sys::serialize(ptr_in, state.map_state.map_data.province_border_vertices);
		ptr_in = sys::serialize(ptr_in, state.map_state.map_data.province_border_starts);
		ptr_in = sys::serialize(ptr_in, state.map_state.map_data.province_border_counts);
		ptr_in = sys::serialize(ptr_in, state.map_state.map_data.terrain_id_map);
		ptr_in = sys::serialize(ptr_in, state.map_state.map_data.province_id_map);
		ptr_in = sys::serialize(ptr_in, state.map_state.map_data.province_area);
		ptr_in = sys::serialize(ptr_in, state.map_state.map_data.diagonal_borders);
		return ptr_in;
	});
	write_single_component(state, NATIVE("defines.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		memcpy(ptr_in, &(state.defines), sizeof(parsing::defines));
		ptr_in += sizeof(parsing::defines);
		return ptr_in;
	});
	write_single_component(state, NATIVE("economy_definitions.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		memcpy(ptr_in, &(state.economy_definitions), sizeof(economy::global_economy_state));
		ptr_in += sizeof(economy::global_economy_state);
		return ptr_in;
	});
	write_single_component(state, NATIVE("party_issues.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.culture_definitions.party_issues);
		return ptr_in;
	});
	write_single_component(state, NATIVE("political_issues.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.culture_definitions.political_issues);
		return ptr_in;
	});
	write_single_component(state, NATIVE("social_issues.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.culture_definitions.social_issues);
		return ptr_in;
	});
	write_single_component(state, NATIVE("military_issues.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.culture_definitions.military_issues);
		return ptr_in;
	});
	write_single_component(state, NATIVE("economic_issues.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.culture_definitions.economic_issues);
		return ptr_in;
	});
	write_single_component(state, NATIVE("tech_folders.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.culture_definitions.tech_folders);
		return ptr_in;
	});
	write_single_component(state, NATIVE("crimes.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.culture_definitions.crimes);
		return ptr_in;
	});
	write_single_component(state, NATIVE("culture_definitions.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.artisans);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.capitalists);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.farmers);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.laborers);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.clergy);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.soldiers);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.officers);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.slaves);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.bureaucrat);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.aristocrat);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.primary_factory_worker);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.secondary_factory_worker);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.officer_leadership_points);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.bureaucrat_tax_efficiency);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.conservative);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.jingoism);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.promotion_chance);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.demotion_chance);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.migration_chance);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.colonialmigration_chance);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.emigration_chance);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.assimilation_chance);
		ptr_in = sys::memcpy_serialize(ptr_in, state.culture_definitions.conversion_chance);
		return ptr_in;
	});
	write_single_component(state, NATIVE("unit_base_definitions.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.military_definitions.unit_base_definitions);
		return ptr_in;
	});
	write_single_component(state, NATIVE("military_definitions.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::memcpy_serialize(ptr_in, state.military_definitions.first_background_trait);
		//ptr_in = sys::serialize(ptr_in, state.military_definitions.unit_base_definitions);
		ptr_in = sys::memcpy_serialize(ptr_in, state.military_definitions.base_army_unit);
		ptr_in = sys::memcpy_serialize(ptr_in, state.military_definitions.base_naval_unit);
		ptr_in = sys::memcpy_serialize(ptr_in, state.military_definitions.standard_civil_war);
		ptr_in = sys::memcpy_serialize(ptr_in, state.military_definitions.standard_great_war);
		ptr_in = sys::memcpy_serialize(ptr_in, state.military_definitions.standard_status_quo);
		ptr_in = sys::memcpy_serialize(ptr_in, state.military_definitions.liberate);
		ptr_in = sys::memcpy_serialize(ptr_in, state.military_definitions.uninstall_communist_gov);
		ptr_in = sys::memcpy_serialize(ptr_in, state.military_definitions.crisis_colony);
		ptr_in = sys::memcpy_serialize(ptr_in, state.military_definitions.crisis_liberate);
		ptr_in = sys::memcpy_serialize(ptr_in, state.military_definitions.irregular);
		//ptr_in = sys::memcpy_serialize(ptr_in, state.military_definitions.infantry);
		ptr_in = sys::memcpy_serialize(ptr_in, state.military_definitions.artillery);
		return ptr_in;
	});
	write_single_component(state, NATIVE("national_definitions.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.national_definitions.flag_variable_names);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.global_flag_variable_names);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.variable_names);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.triggered_modifiers);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.rebel_id);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.very_easy_player);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.easy_player);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.hard_player);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.very_hard_player);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.very_easy_ai);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.easy_ai);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.hard_ai);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.very_hard_ai);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.overseas);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.coastal);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.non_coastal);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.coastal_sea);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.sea_zone);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.land_province);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.blockaded);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.no_adjacent_controlled);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.core);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.has_siege);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.occupied);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.nationalism);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.infrastructure);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.base_values);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.war);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.peace);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.disarming);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.war_exhaustion);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.badboy);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.debt_default_to);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.bad_debter);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.great_power);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.second_power);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.civ_nation);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.unciv_nation);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.average_literacy);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.plurality);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.generalised_debt_default);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.total_occupation);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.total_blockaded);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.in_bankrupcy);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.num_allocated_national_variables);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.num_allocated_national_flags);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.num_allocated_global_flags);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.flashpoint_focus);
		ptr_in = sys::memcpy_serialize(ptr_in, state.national_definitions.flashpoint_amount);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.on_yearly_pulse);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.on_quarterly_pulse);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.on_battle_won);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.on_battle_lost);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.on_surrender);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.on_new_great_nation);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.on_lost_great_nation);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.on_election_tick);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.on_colony_to_state);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.on_state_conquest);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.on_colony_to_state_free_slaves);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.on_debtor_default);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.on_debtor_default_small);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.on_debtor_default_second);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.on_civilize);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.on_my_factories_nationalized);
		ptr_in = sys::serialize(ptr_in, state.national_definitions.on_crisis_declare_interest);
		return ptr_in;
	});
	write_single_component(state, NATIVE("province_definitions.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.province_definitions.canals);
		ptr_in = sys::serialize(ptr_in, state.province_definitions.terrain_to_gfx_map);
		ptr_in = sys::memcpy_serialize(ptr_in, state.province_definitions.first_sea_province);
		ptr_in = sys::memcpy_serialize(ptr_in, state.province_definitions.europe);
		ptr_in = sys::memcpy_serialize(ptr_in, state.province_definitions.asia);
		ptr_in = sys::memcpy_serialize(ptr_in, state.province_definitions.africa);
		ptr_in = sys::memcpy_serialize(ptr_in, state.province_definitions.north_america);
		ptr_in = sys::memcpy_serialize(ptr_in, state.province_definitions.south_america);
		ptr_in = sys::memcpy_serialize(ptr_in, state.province_definitions.oceania);
		return ptr_in;
	});
	write_single_component(state, NATIVE("dates.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::memcpy_serialize(ptr_in, state.start_date);
		ptr_in = sys::memcpy_serialize(ptr_in, state.end_date);
		return ptr_in;
	});
	write_single_component(state, NATIVE("trigger_data.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.trigger_data);
		return ptr_in;
	});
	write_single_component(state, NATIVE("trigger_data_indices.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.trigger_data_indices);
		return ptr_in;
	});
	write_single_component(state, NATIVE("effect_data.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.effect_data);
		return ptr_in;
	});
	write_single_component(state, NATIVE("effect_data_indices.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.effect_data_indices);
		return ptr_in;
	});
	write_single_component(state, NATIVE("value_modifier_segments.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.value_modifier_segments);
		return ptr_in;
	});
	write_single_component(state, NATIVE("value_modifiers.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.value_modifiers);
		return ptr_in;
	});
	write_single_component(state, NATIVE("ui_defs_gfx.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.ui_defs.gfx);
		return ptr_in;
	});
	write_single_component(state, NATIVE("ui_defs_textures.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.ui_defs.textures);
		return ptr_in;
	});
	write_single_component(state, NATIVE("ui_defs_textures.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.ui_defs.textures);
		return ptr_in;
	});
	write_single_component(state, NATIVE("ui_defs_gui.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.ui_defs.gui);
		return ptr_in;
	});
	write_single_component(state, NATIVE("font_collection_font_names.bin"), [&](uint8_t* ptr_in, sys::state& state) -> uint8_t* {
		ptr_in = sys::serialize(ptr_in, state.font_collection.font_names);
		return ptr_in;
	});
	log_to_console(state, state.ui_state.console_window, u"Check \"My Documents\\Project Alice\\oos\" for the OOS dump");
	window::change_cursor(state, window::cursor_type::normal_cancel_busy);

	return p + 2;
}

int32_t* f_cheat_wargoals(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	bool toggle_state = s.main_data_back(0) != 0;
	s.pop_main();

	state->cheat_data.always_allow_wargoals = toggle_state;
	return p + 2;
}
int32_t* f_cheat_reforms(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	bool toggle_state = s.main_data_back(0) != 0;
	s.pop_main();

	state->cheat_data.always_allow_reforms = toggle_state;
	return p + 2;
}
int32_t* f_cheat_deals(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	bool toggle_state = s.main_data_back(0) != 0;
	s.pop_main();

	state->cheat_data.always_accept_deals = toggle_state;
	return p + 2;
}
int32_t* f_cheat_decisions(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	bool toggle_state = s.main_data_back(0) != 0;
	s.pop_main();

	state->cheat_data.always_allow_decisions = toggle_state;
	return p + 2;
}
int32_t* f_daily_oos(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	bool toggle_state = s.main_data_back(0) != 0;
	s.pop_main();

	state->cheat_data.daily_oos_check = toggle_state;
	return p + 2;
}
int32_t* f_cheat_decision_potential(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	bool toggle_state = s.main_data_back(0) != 0;
	s.pop_main();

	state->cheat_data.always_potential_decisions = toggle_state;
	return p + 2;
}
int32_t* f_set_auto_choice(fif::state_stack& s, int32_t* p, fif::environment* env) {
	if(fif::typechecking_mode(env->mode)) {
		if(fif::typechecking_failed(env->mode))
			return p + 2;
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*env, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	bool toggle_state = s.main_data_back(0) != 0;
	s.pop_main();

	if(toggle_state) {
		for(const auto e : state->world.in_national_event) {
			e.set_auto_choice(1);
		}
		for(const auto e : state->world.in_free_national_event) {
			e.set_auto_choice(1);
		}
		for(const auto e : state->world.in_provincial_event) {
			e.set_auto_choice(1);
		}
		for(const auto e : state->world.in_free_provincial_event) {
			e.set_auto_choice(1);
		}
	} else {
		for(const auto e : state->world.in_national_event) {
			e.set_auto_choice(0);
		}
		for(const auto e : state->world.in_free_national_event) {
			e.set_auto_choice(0);
		}
		for(const auto e : state->world.in_provincial_event) {
			e.set_auto_choice(0);
		}
		for(const auto e : state->world.in_free_provincial_event) {
			e.set_auto_choice(0);
		}
	}
	return p + 2;
}
int32_t* f_complete_construction(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	dcon::nation_id to_nation;
	to_nation.value = dcon::nation_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	for(uint32_t i = state->world.province_building_construction_size(); i-- > 0;) {
		dcon::province_building_construction_id c{ dcon::province_building_construction_id::value_base_t(i) };

		if(state->world.province_building_construction_get_nation(c) != to_nation)
			continue;

		auto t = economy::province_building_type(state->world.province_building_construction_get_type(c));
		auto const& base_cost = state->economy_definitions.building_definitions[int32_t(t)].cost;
		auto& current_purchased = state->world.province_building_construction_get_purchased_goods(c);
		for(uint32_t j = 0; j < economy::commodity_set::set_size; ++j)
			current_purchased.commodity_amounts[j] = base_cost.commodity_amounts[j] * 2.f;
	}

	return p + 2;
}
int32_t* f_instant_research(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	bool toggle_state = s.main_data_back(0) != 0;
	s.pop_main();

	dcon::nation_id to_nation;
	to_nation.value = dcon::nation_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	auto pos = std::find(
		state->cheat_data.instant_research_nations.begin(),
		state->cheat_data.instant_research_nations.end(),
		to_nation
	);
	if(toggle_state && pos == state->cheat_data.instant_research_nations.end()) {
		state->cheat_data.instant_research_nations.push_back(to_nation);
	} else if(!toggle_state && pos != state->cheat_data.instant_research_nations.end()) {
		state->cheat_data.instant_research_nations.erase(pos);
	}

	return p + 2;
}

int32_t* f_conquer(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	dcon::nation_id to_nation;
	to_nation.value = dcon::nation_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	dcon::nation_id to_nation_b;
	to_nation_b.value = dcon::nation_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	auto target_owns = state->world.nation_get_province_ownership(to_nation);

	while(target_owns.begin() != target_owns.end()) {
		auto prov = (*target_owns.begin()).get_province();
		province::conquer_province(*state, prov, to_nation_b);
	}

	return p + 2;
}

int32_t* f_make_core(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	dcon::nation_id to_nation;
	to_nation.value = dcon::nation_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	dcon::province_id prov;
	prov.value = dcon::province_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	province::add_core(*state, prov, state->world.nation_get_identity_from_identity_holder(to_nation));

	return p + 2;
}
int32_t* f_remove_core(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	dcon::nation_id to_nation;
	to_nation.value = dcon::nation_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	dcon::province_id prov;
	prov.value = dcon::province_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	province::remove_core(*state, prov, state->world.nation_get_identity_from_identity_holder(to_nation));

	return p + 2;
}
int32_t* f_set_owner(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	dcon::nation_id to_nation;
	to_nation.value = dcon::nation_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	dcon::province_id prov;
	prov.value = dcon::province_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	province::conquer_province(*state, prov, to_nation);

	return p + 2;
}
int32_t* f_set_controller(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	dcon::nation_id to_nation;
	to_nation.value = dcon::nation_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	dcon::province_id prov;
	prov.value = dcon::province_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	province::set_province_controller(*state, prov, to_nation);

	return p + 2;
}
int32_t* f_cheat_army(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	bool toggle_state = s.main_data_back(0) != 0;
	s.pop_main();

	state->cheat_data.instant_army = toggle_state;
	return p + 2;
}
int32_t* f_cheat_navy(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	bool toggle_state = s.main_data_back(0) != 0;
	s.pop_main();

	state->cheat_data.instant_navy = toggle_state;
	return p + 2;
}
int32_t* f_cheat_factories(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	bool toggle_state = s.main_data_back(0) != 0;
	s.pop_main();

	state->cheat_data.instant_industry = toggle_state;
	return p + 2;
}
int32_t* f_add_days(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	auto days = s.main_data_back(0);
	s.pop_main();

	state->current_date += int32_t(days);
	return p + 2;
}
int32_t* f_save_map(fif::state_stack& s, int32_t* ptr, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return ptr + 2;
		s.pop_main();
		return ptr + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	auto type = s.main_data_back(0);
	s.pop_main();

	bool opt_sea_lines = true;
	bool opt_province_lines = true;
	bool opt_blend = true;

	if(type == 0) {
		opt_sea_lines = false;
		opt_province_lines = false;
	} else if(type ==1) {
		opt_blend = false;
	} else if(type == 2) {
		opt_sea_lines = false;
	} else if(type == 3) {
		opt_sea_lines = false;
		opt_blend = false;
	} else if(type == 4) {
		opt_sea_lines = false;
		opt_province_lines = false;
		opt_blend = false;
	}

	auto total_px = state->map_state.map_data.size_x * state->map_state.map_data.size_y;
	auto buffer = std::unique_ptr<uint8_t[]>(new uint8_t[total_px * 3]);
	auto blend_fn = [&](uint32_t idx, bool sea_a, bool sea_b, dcon::province_id pa, dcon::province_id pb) {
		if(sea_a != sea_b) {
			buffer[idx * 3 + 0] = 0;
			buffer[idx * 3 + 1] = 0;
			buffer[idx * 3 + 2] = 0;
		}
		if(pa != pb) {
			if(((sea_a || sea_b) && opt_sea_lines)
			|| sea_a != sea_b
			|| (opt_province_lines && !sea_a && !sea_b)) {
				if(opt_blend) {
					buffer[idx * 3 + 0] &= 0x7f;
					buffer[idx * 3 + 1] &= 0x7f;
					buffer[idx * 3 + 2] &= 0x7f;
				} else {
					buffer[idx * 3 + 0] = 0;
					buffer[idx * 3 + 1] = 0;
					buffer[idx * 3 + 2] = 0;
				}
			}
		}
		};
	for(uint32_t y = 0; y < uint32_t(state->map_state.map_data.size_y); y++) {
		for(uint32_t x = 0; x < uint32_t(state->map_state.map_data.size_x); x++) {
			auto idx = y * uint32_t(state->map_state.map_data.size_x) + x;
			auto p = province::from_map_id(state->map_state.map_data.province_id_map[idx]);
			bool p_is_sea = state->map_state.map_data.province_id_map[idx] >= province::to_map_id(state->province_definitions.first_sea_province);
			if(p_is_sea) {
				buffer[idx * 3 + 0] = 128;
				buffer[idx * 3 + 1] = 128;
				buffer[idx * 3 + 2] = 255;
			} else {
				auto owner = state->world.province_get_nation_from_province_ownership(p);
				if(owner) {
					auto owner_color = state->world.nation_get_color(owner);
					buffer[idx * 3 + 0] = uint8_t(owner_color & 0xff);
					buffer[idx * 3 + 1] = uint8_t((owner_color >> 8) & 0xff) & 0xff;
					buffer[idx * 3 + 2] = uint8_t((owner_color >> 16) & 0xff) & 0xff;
				} else {
					buffer[idx * 3 + 0] = 170;
					buffer[idx * 3 + 1] = 170;
					buffer[idx * 3 + 2] = 170;
				}
			}
			if(x < uint32_t(state->map_state.map_data.size_x - 1)) {
				auto br_idx = idx + uint32_t(state->map_state.map_data.size_x);
				if(br_idx < total_px) {
					auto br_p = province::from_map_id(state->map_state.map_data.province_id_map[br_idx]);
					bool br_is_sea = state->map_state.map_data.province_id_map[br_idx] >= province::to_map_id(state->province_definitions.first_sea_province);
					blend_fn(idx, br_is_sea, p_is_sea, br_p, p);
				}
				auto rs_idx = idx + 1;
				if(rs_idx < total_px) {
					auto br_p = province::from_map_id(state->map_state.map_data.province_id_map[rs_idx]);
					bool br_is_sea = state->map_state.map_data.province_id_map[rs_idx] >= province::to_map_id(state->province_definitions.first_sea_province);
					blend_fn(idx, br_is_sea, p_is_sea, br_p, p);
				}
			}
		}
	}
	stbi_flip_vertically_on_write(true);
	auto func = [](void*, void* ptr_in, int size) -> void {
		auto sdir = simple_fs::get_or_create_oos_directory();
		simple_fs::write_file(sdir, NATIVE("map.png"), static_cast<const char*>(ptr_in), uint32_t(size));
		};
	stbi_write_png_to_func(func, nullptr, int(state->map_state.map_data.size_x), int(state->map_state.map_data.size_y), 3, buffer.get(), 0);

	return ptr + 2;
}
int32_t* f_dump_econ(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	if(state->cheat_data.ecodump) {
		state->cheat_data.ecodump = false;
	} else {
		state->cheat_data.ecodump = true;

		state->world.for_each_commodity([&](dcon::commodity_id c) {
			state->cheat_data.prices_dump_buffer += text::produce_simple_string(*state, state->world.commodity_get_name(c)) + ",";
			state->cheat_data.demand_dump_buffer += text::produce_simple_string(*state, state->world.commodity_get_name(c)) + ",";
			state->cheat_data.supply_dump_buffer += text::produce_simple_string(*state, state->world.commodity_get_name(c)) + ",";
		});

		state->cheat_data.prices_dump_buffer += "\n";
		state->cheat_data.demand_dump_buffer += "\n";
		state->cheat_data.supply_dump_buffer += "\n";

		state->world.for_each_pop_type([&](auto pop_type) {
			state->cheat_data.savings_buffer += text::produce_simple_string(
				*state,
				state->world.pop_type_get_name(pop_type)
			);
			state->cheat_data.savings_buffer += ";";
		});

		state->cheat_data.savings_buffer += "markets;nations;investments\n";
	}
	log_to_console(*state, state->ui_state.console_window, state->cheat_data.ecodump ? u"✔" : u"✘");

	return p + 2;
}

int32_t* f_market_shadow(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(!fif::typechecking_failed(e->mode)) s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	auto* state = (sys::state*)(state_global->data);
	dcon::commodity_id commodity;
	commodity.value = dcon::commodity_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	auto seed = state->map_state.selected_province;
	if(!seed && state->local_player_nation) seed = state->world.nation_get_capital(state->local_player_nation);
	auto report = economy::foundry_transport::run_live_shadow(*state, seed, commodity, 5);
	// This diagnostic can generate many wrapped lines. Replace the previous
	// snapshot and rebuild its layout once; clearing and then appending caused
	// two immediate HarfBuzz passes over the same console layout.
	auto* console = static_cast<ui::console_window*>(state->ui_state.console_window);
	console->replace_list(*state, "market-shadow\n" + report + "\n");
	return p + 2;
}

int32_t* f_market_shadow_wide(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(!fif::typechecking_failed(e->mode)) s.pop_main();
		return p + 2;
	}
	auto state_global = fif::get_global_var(*e, "state-ptr");
	auto* state = reinterpret_cast<sys::state*>(state_global->data);
	dcon::commodity_id commodity;
	commodity.value = dcon::commodity_id::value_base_t(s.main_data_back(0));
	s.pop_main();
	auto seed = state->map_state.selected_province;
	if(!seed && state->local_player_nation) seed = state->world.nation_get_capital(state->local_player_nation);
	auto report = economy::foundry_transport::run_live_shadow(*state, seed, commodity, 25);
	auto* console = static_cast<ui::console_window*>(state->ui_state.console_window);
	console->replace_list(*state, "market-shadow-wide\n" + report + "\n");
	return p + 2;
}

int32_t* f_market_shadow_batch(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) return p + 2;
	auto state_global = fif::get_global_var(*e, "state-ptr");
	auto* state = reinterpret_cast<sys::state*>(state_global->data);
	auto seed = state->map_state.selected_province;
	if(!seed && state->local_player_nation) seed = state->world.nation_get_capital(state->local_player_nation);
	auto report = economy::foundry_transport::run_live_shadow_batch(*state, seed, 25, 12);
	static_cast<ui::console_window*>(state->ui_state.console_window)->replace_list(*state, "market-shadow-batch\n" + report + "\n");
	return p + 2;
}

int32_t* f_market_live_audit(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(!fif::typechecking_failed(e->mode)) s.pop_main();
		return p + 2;
	}
	auto state_global = fif::get_global_var(*e, "state-ptr");
	auto* state = reinterpret_cast<sys::state*>(state_global->data);
	dcon::commodity_id commodity;
	commodity.value = dcon::commodity_id::value_base_t(s.main_data_back(0)); s.pop_main();
	if(!commodity || !state->world.commodity_is_valid(commodity)) {
		log_to_console(*state, state->ui_state.console_window, u"Invalid commodity; live audit remains disabled.");
		state->cheat_data.foundry_market_live_audit = false;
		return p + 2;
	}
	state->cheat_data.foundry_market_live_commodity = commodity;
	state->cheat_data.foundry_market_live_basket = false;
	state->cheat_data.foundry_market_live_all_goods = false;
	state->cheat_data.foundry_market_live_access_model = false;
	state->cheat_data.foundry_market_live_max_markets = 25;
	state->cheat_data.foundry_market_live_interval_days = 1;
	state->cheat_data.foundry_market_live_ran_today = false;
	state->cheat_data.foundry_market_live_runs = 0;
	state->cheat_data.foundry_market_live_failures = 0;
	state->cheat_data.foundry_market_live_comparisons = 0;
	state->cheat_data.foundry_market_live_markets.clear();
	state->cheat_data.foundry_market_sum_abs_consumption_delta = 0.0;
	state->cheat_data.foundry_market_sum_abs_unmet_delta = 0.0;
	state->cheat_data.foundry_market_sum_abs_unsold_delta = 0.0;
	state->cheat_data.foundry_market_live_audit = true;
	log_to_console(*state, state->ui_state.console_window,
		u"Foundry daily live market audit enabled (vanilla remains authoritative).");
	return p + 2;
}

int32_t* f_market_live_audit_basket(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) return p + 2;
	auto* state = reinterpret_cast<sys::state*>(fif::get_global_var(*e, "state-ptr")->data);
	state->cheat_data.foundry_market_live_basket = true;
	state->cheat_data.foundry_market_live_all_goods = false;
	state->cheat_data.foundry_market_live_access_model = false;
	state->cheat_data.foundry_market_live_max_markets = 25;
	state->cheat_data.foundry_market_live_interval_days = 1;
	state->cheat_data.foundry_market_live_ran_today = false;
	state->cheat_data.foundry_market_live_runs = 0;
	state->cheat_data.foundry_market_live_failures = 0;
	state->cheat_data.foundry_market_live_comparisons = 0;
	state->cheat_data.foundry_market_basket_runtime_sum_us = 0;
	state->cheat_data.foundry_market_live_markets.clear();
	state->cheat_data.foundry_market_live_audit = true;
	log_to_console(*state, state->ui_state.console_window,
		u"Foundry 12-good daily market audit enabled (vanilla remains authoritative).");
	return p + 2;
}

int32_t* f_market_live_audit_all(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) return p + 2;
	auto* state = reinterpret_cast<sys::state*>(fif::get_global_var(*e, "state-ptr")->data);
	state->cheat_data.foundry_market_live_basket = true;
	state->cheat_data.foundry_market_live_all_goods = true;
	state->cheat_data.foundry_market_live_access_model = false;
	state->cheat_data.foundry_market_live_max_markets = 25;
	state->cheat_data.foundry_market_live_interval_days = 1;
	state->cheat_data.foundry_market_live_ran_today = false;
	state->cheat_data.foundry_market_live_runs = 0;
	state->cheat_data.foundry_market_live_failures = 0;
	state->cheat_data.foundry_market_live_comparisons = 0;
	state->cheat_data.foundry_market_basket_runtime_sum_us = 0;
	state->cheat_data.foundry_market_live_markets.clear();
	state->cheat_data.foundry_market_live_audit = true;
	log_to_console(*state, state->ui_state.console_window,
		u"Foundry all-active-goods audit enabled (vanilla remains authoritative).");
	return p + 2;
}

void start_market_scale_audit(sys::state& state, size_t maximum_markets) {
	state.cheat_data.foundry_market_live_basket = true;
	state.cheat_data.foundry_market_live_all_goods = true;
	state.cheat_data.foundry_market_live_access_model = false;
	state.cheat_data.foundry_market_live_max_markets = maximum_markets;
	// Large state-market solves are deliberately batched. Local production and
	// vanilla clearing still update daily while the routed comparison refreshes
	// weekly, matching the intended hybrid economy cadence.
	state.cheat_data.foundry_market_live_interval_days = maximum_markets > 100 ? 7 : 1;
	state.cheat_data.foundry_market_live_ran_today = false;
	state.cheat_data.foundry_market_live_runs = 0;
	state.cheat_data.foundry_market_live_failures = 0;
	state.cheat_data.foundry_market_live_comparisons = 0;
	state.cheat_data.foundry_market_basket_runtime_sum_us = 0;
	state.cheat_data.foundry_market_live_markets.clear();
	state.cheat_data.foundry_market_live_audit = true;
}

int32_t* f_market_access_audit_world(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) return p + 2;
	auto* state = reinterpret_cast<sys::state*>(fif::get_global_var(*e, "state-ptr")->data);
	start_market_scale_audit(*state, state->world.market_size());
	state->cheat_data.foundry_market_live_access_model = true;
	state->cheat_data.foundry_market_live_interval_days = 1;
	log_to_console(*state, state->ui_state.console_window,
		u"Foundry tiered state-access world audit enabled daily (no pathfinding; vanilla remains authoritative).");
	return p + 2;
}

int32_t* f_market_live_audit_50(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) return p + 2;
	auto* state = reinterpret_cast<sys::state*>(fif::get_global_var(*e, "state-ptr")->data);
	start_market_scale_audit(*state, 50);
	log_to_console(*state, state->ui_state.console_window, u"Foundry all-goods 50-state-market audit enabled.");
	return p + 2;
}

int32_t* f_market_live_audit_100(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) return p + 2;
	auto* state = reinterpret_cast<sys::state*>(fif::get_global_var(*e, "state-ptr")->data);
	start_market_scale_audit(*state, 100);
	log_to_console(*state, state->ui_state.console_window, u"Foundry all-goods 100-state-market audit enabled.");
	return p + 2;
}

int32_t* f_market_live_audit_world(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) return p + 2;
	auto* state = reinterpret_cast<sys::state*>(fif::get_global_var(*e, "state-ptr")->data);
	start_market_scale_audit(*state, state->world.market_size());
	log_to_console(*state, state->ui_state.console_window, u"Foundry all-goods reachable-world state-market audit enabled (weekly routing cadence).");
	return p + 2;
}

int32_t* f_market_live_audit_off(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) return p + 2;
	auto state_global = fif::get_global_var(*e, "state-ptr");
	auto* state = reinterpret_cast<sys::state*>(state_global->data);
	state->cheat_data.foundry_market_live_audit = false;
	state->cheat_data.foundry_market_live_access_model = false;
	state->cheat_data.foundry_market_live_ran_today = false;
	log_to_console(*state, state->ui_state.console_window, u"Foundry daily live market audit disabled.");
	return p + 2;
}

int32_t* f_market_live_status(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) return p + 2;
	auto state_global = fif::get_global_var(*e, "state-ptr");
	auto* state = reinterpret_cast<sys::state*>(state_global->data);
	auto status = std::string(state->cheat_data.foundry_market_live_audit ? "ACTIVE" : "OFF")
		+ " runs=" + std::to_string(state->cheat_data.foundry_market_live_runs)
		+ " failures=" + std::to_string(state->cheat_data.foundry_market_live_failures);
	if(state->cheat_data.foundry_market_live_audit)
		status += " cadence=" + std::to_string(state->cheat_data.foundry_market_live_interval_days) + "d";
	if(state->cheat_data.foundry_market_live_access_model) status += " model=state-access";
	auto comparisons = state->cheat_data.foundry_market_live_comparisons;
	if(comparisons && state->cheat_data.foundry_market_live_basket) {
		auto format = [](double value) { std::ostringstream out; out << std::fixed << std::setprecision(2) << value; return out.str(); };
		status += " basket goods=" + std::to_string(state->cheat_data.foundry_market_basket_commodities.size());
		status += state->cheat_data.foundry_market_live_all_goods ? " (all active)" : " (sample)";
		status += " markets=" + std::to_string(state->cheat_data.foundry_market_live_markets.size());
		status += "\naggregate delta C/U/S="
			+ format(state->cheat_data.foundry_market_basket_latest_consumption_delta) + "/"
			+ format(state->cheat_data.foundry_market_basket_latest_unmet_delta) + "/"
			+ format(state->cheat_data.foundry_market_basket_latest_unsold_delta);
		status += "\nconsumption delta raw/industrial="
			+ format(state->cheat_data.foundry_market_basket_category_delta[0]) + "/"
			+ format(state->cheat_data.foundry_market_basket_category_delta[1]);
		status += "\nconsumer/military="
			+ format(state->cheat_data.foundry_market_basket_category_delta[2]) + "/"
			+ format(state->cheat_data.foundry_market_basket_category_delta[3]);
		if(state->cheat_data.foundry_market_basket_worst_commodity)
			status += "\nworst=" + text::produce_simple_string(*state,
				state->world.commodity_get_name(state->cheat_data.foundry_market_basket_worst_commodity))
				+ " delta=" + format(state->cheat_data.foundry_market_basket_worst_delta);
		status += "\nrouting runtime latest/mean us="
			+ std::to_string(state->cheat_data.foundry_market_basket_latest_runtime_us) + "/"
			+ std::to_string(state->cheat_data.foundry_market_basket_runtime_sum_us / comparisons);
		status += " searches=" + std::to_string(state->cheat_data.foundry_market_basket_latest_path_searches);
		if(state->cheat_data.foundry_market_live_access_model) {
			status += "\nconsumption local/domestic/global="
				+ format(state->cheat_data.foundry_market_access_local_consumption) + "/"
				+ format(state->cheat_data.foundry_market_access_domestic_consumption) + "/"
				+ format(state->cheat_data.foundry_market_access_global_consumption);
			status += "\nmarket access min/mean/max="
				+ format(state->cheat_data.foundry_market_access_min) + "/"
				+ format(state->cheat_data.foundry_market_access_mean) + "/"
				+ format(state->cheat_data.foundry_market_access_max);

			status += "\ntop shortages S/D/C/delta:";
			for(size_t rank = 0; rank < state->cheat_data.foundry_market_top_shortage_count; ++rank) {
				auto commodity = state->cheat_data.foundry_market_top_shortage_commodities[rank];
				status += "\n" + std::to_string(rank + 1) + ". "
					+ text::produce_simple_string(*state, state->world.commodity_get_name(commodity)) + " "
					+ format(state->cheat_data.foundry_market_top_shortage_supply[rank]) + "/"
					+ format(state->cheat_data.foundry_market_top_shortage_demand[rank]) + "/"
					+ format(state->cheat_data.foundry_market_top_shortage_consumption[rank]) + "/"
					+ format(state->cheat_data.foundry_market_top_shortage_delta[rank]);
			}
		}
	}
	if(comparisons && !state->cheat_data.foundry_market_live_basket) {
		auto format = [](double value) {
			std::ostringstream out;
			out << std::fixed << std::setprecision(2) << value;
			return out.str();
		};
		status += "\ndelta routed-vanilla latest C/U/S="
			+ format(state->cheat_data.foundry_market_latest_consumption_delta) + "/"
			+ format(state->cheat_data.foundry_market_latest_unmet_delta) + "/"
			+ format(state->cheat_data.foundry_market_latest_unsold_delta);
		status += "\nmean absolute delta C/U/S="
			+ format(state->cheat_data.foundry_market_sum_abs_consumption_delta / comparisons) + "/"
			+ format(state->cheat_data.foundry_market_sum_abs_unmet_delta / comparisons) + "/"
			+ format(state->cheat_data.foundry_market_sum_abs_unsold_delta / comparisons);
		status += "\nlatest consumption routed/vanilla="
			+ format(state->cheat_data.foundry_market_routed_consumption) + "/"
			+ format(state->cheat_data.foundry_market_vanilla_consumption);
	}
	static_cast<ui::console_window*>(state->ui_state.console_window)->replace_list(*state, "market-live-status\n" + status + "\n");
	return p + 2;
}

int32_t* f_rgo_ai_audit(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) return p + 2;
	auto* state = reinterpret_cast<sys::state*>(fif::get_global_var(*e, "state-ptr")->data);
	auto nation = state->local_player_nation;
	if(!nation) {
		log_to_console(*state, state->ui_state.console_window, u"Select a playable nation before running the RGO AI audit.");
		return p + 2;
	}

	struct candidate {
		dcon::province_id province;
		dcon::commodity_id commodity;
		float score = 0.f;
		float shortage = 0.f;
		float utilization = 0.f;
		float price_ratio = 0.f;
		float tier_room = 0.f;
		bool labor_available = false;
	};
	std::vector<candidate> candidates;
	state->world.nation_for_each_province_ownership(nation, [&](auto ownership) {
		auto province = state->world.province_ownership_get_province(ownership);
		auto local_state = state->world.province_get_state_membership(province);
		auto market = state->world.state_instance_get_market_from_local_market(local_state);
		if(!market) return;
		state->world.for_each_commodity([&](dcon::commodity_id commodity) {
			if(state->world.commodity_get_rgo_amount(commodity) <= 0.f) return;
			auto current_cap = state->world.province_get_rgo_max_size(province, commodity);
			auto potential = state->world.province_get_rgo_potential(province, commodity);
			if(current_cap <= 0.f || potential <= current_cap + 1.f) return;

			auto national_demand = economy::demand(*state, nation, commodity);
			auto national_supply = economy::supply(*state, nation, commodity);
			auto shortage = national_demand > 0.01f
				? std::clamp((national_demand - national_supply) / national_demand, 0.f, 1.f) : 0.f;
			if(shortage < 0.02f) return;

			auto size = state->world.province_get_rgo_size(province, commodity);
			auto utilization = std::clamp(size / current_cap, 0.f, 1.f);
			auto median_price = std::max(0.01f, state->world.commodity_get_median_price(commodity));
			auto price_ratio = state->world.market_get_price(market, commodity) / median_price;
			auto labor_available = ai::province_has_available_workers(*state, province);
			auto tier_room = std::max(0.f, (potential - current_cap)
				/ std::max(1.f, float(state->world.commodity_get_rgo_workforce(commodity))));

			auto score = shortage * 100.f
				+ utilization * 35.f
				+ std::clamp(price_ratio - 1.f, 0.f, 2.f) * 15.f
				+ (labor_available ? 5.f : 0.f)
				+ std::min(tier_room, 4.f);
			candidates.push_back({ province, commodity, score, shortage, utilization,
				price_ratio, tier_room, labor_available });
		});
	});
	std::sort(candidates.begin(), candidates.end(), [](auto const& a, auto const& b) {
		return a.score > b.score;
	});

	auto format = [](float value) {
		std::ostringstream out;
		out << std::fixed << std::setprecision(2) << value;
		return out.str();
	};
	std::string report = "RGO AI RECOMMENDATIONS (read-only; all RGO goods)\n";
	report += "Nation: " + text::produce_simple_string(*state, text::get_name(*state, nation));
	report += " candidates=" + std::to_string(candidates.size());
	if(candidates.empty()) report += "\nNo eligible persistent-shortage upgrades found.";
	for(size_t rank = 0; rank < std::min<size_t>(12, candidates.size()); ++rank) {
		auto const& c = candidates[rank];
		report += "\n" + std::to_string(rank + 1) + ". "
			+ text::produce_simple_string(*state, state->world.commodity_get_name(c.commodity))
			+ " - " + text::produce_simple_string(*state, state->world.province_get_name(c.province))
			+ " score=" + format(c.score)
			+ " shortage=" + format(c.shortage * 100.f) + "%"
			+ " used=" + format(c.utilization * 100.f) + "%"
			+ " price=" + format(c.price_ratio) + "x"
			+ " labor=" + (c.labor_available ? "yes" : "no")
			+ " room=" + format(c.tier_room);
	}
	static_cast<ui::console_window*>(state->ui_state.console_window)->replace_list(*state,
		"rgo-ai-audit\n" + report + "\n");
	return p + 2;
}

int32_t* f_rgo_ai_world_audit(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) return p + 2;
	auto* state = reinterpret_cast<sys::state*>(fif::get_global_var(*e, "state-ptr")->data);
	struct activity {
		dcon::nation_id nation;
		dcon::province_id province;
		dcon::commodity_id commodity;
		float shortage = 0.f;
		float utilization = 0.f;
		float trend = 0.f;
		float score = 0.f;
		float progress = 0.f;
	};
	std::vector<activity> active;
	for(uint32_t nation_index = 0;
			nation_index < state->cheat_data.foundry_rgo_selected_province.size(); ++nation_index) {
		auto province = state->cheat_data.foundry_rgo_selected_province[nation_index];
		auto commodity = state->cheat_data.foundry_rgo_selected_commodity[nation_index];
		if(!province || !commodity) continue;
		auto nation = dcon::nation_id{ dcon::nation_id::value_base_t(nation_index) };
		active.push_back({ nation, province, commodity,
			state->cheat_data.foundry_rgo_selected_shortage[nation_index],
			state->cheat_data.foundry_rgo_selected_utilization[nation_index],
			state->cheat_data.foundry_rgo_selected_trend[nation_index],
			state->cheat_data.foundry_rgo_selected_score[nation_index],
			state->world.province_get_rgo_level_progress(province, commodity) });
	}
	std::sort(active.begin(), active.end(), [](auto const& a, auto const& b) {
		return a.score > b.score;
	});
	auto format = [](float value) {
		std::ostringstream out;
		out << std::fixed << std::setprecision(2) << value;
		return out.str();
	};
	std::string report = "RGO AI WORLD ACTIVITY (actual selections) active=" + std::to_string(active.size());
	for(size_t rank = 0; rank < std::min<size_t>(24, active.size()); ++rank) {
		auto const& a = active[rank];
		report += "\n" + std::to_string(rank + 1) + ". "
			+ text::produce_simple_string(*state, text::get_name(*state, a.nation)) + ": "
			+ text::produce_simple_string(*state, state->world.commodity_get_name(a.commodity))
			+ " - " + text::produce_simple_string(*state, state->world.province_get_name(a.province))
			+ " score=" + format(a.score)
			+ " short=" + format(a.shortage * 100.f) + "%"
			+ " used=" + format(a.utilization * 100.f) + "%"
			+ " trend=" + format(a.trend * 100.f) + "pp"
			+ " progress=" + format(a.progress * 100.f) + "%";
	}
	static_cast<ui::console_window*>(state->ui_state.console_window)->replace_list(*state,
		"rgo-ai-world-audit\n" + report + "\n");
	return p + 2;
}

int32_t* f_rgo_government_audit(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) return p + 2;
	auto* state = reinterpret_cast<sys::state*>(fif::get_global_var(*e, "state-ptr")->data);
	struct entry {
		dcon::nation_id nation{};
		dcon::province_id province{};
		dcon::commodity_id commodity{};
		float goods_fraction = 0.f;
		float progress = 0.f;
		float capacity = 0.f;
	};
	std::vector<entry> projects;
	std::vector<int32_t> nation_counts(state->world.nation_size(), 0);
	int32_t private_projects = 0;
	state->world.for_each_province([&](dcon::province_id province) {
		auto nation = state->world.province_get_nation_from_province_ownership(province);
		if(!nation) return;
		for(auto commodity : state->world.in_commodity) {
			auto sponsor = state->world.province_get_rgo_upgrade_sponsor(province, commodity);
			if(sponsor == uint8_t(economy::rgo_upgrade_sponsor::private_investors)) {
				++private_projects;
				continue;
			}
			if(sponsor != uint8_t(economy::rgo_upgrade_sponsor::government)) continue;
			auto cost = economy::rgo_upgrade_goods_cost(*state, province, commodity);
			auto& purchased = state->world.province_get_rgo_upgrade_purchased_goods(province, commodity);
			float acquired = 0.f;
			float required = 0.f;
			for(uint32_t i = 0; i < economy::commodity_set::set_size; ++i) {
				if(!cost.commodity_type[i]) break;
				required += cost.commodity_amounts[i];
				acquired += std::min(cost.commodity_amounts[i], purchased.commodity_amounts[i]);
			}
			projects.push_back({nation, province, commodity,
				required > 0.f ? acquired / required : 1.f,
				state->world.province_get_rgo_level_progress(province, commodity),
				economy::civil_construction_capacity_share(*state, {province, commodity})});
			if(size_t(nation.index()) < nation_counts.size()) ++nation_counts[nation.index()];
		}
	});
	std::sort(projects.begin(), projects.end(), [](entry const& a, entry const& b) {
		if(a.nation != b.nation) return a.nation.index() < b.nation.index();
		return a.progress > b.progress;
	});
	auto pct = [](float v) { return text::format_float(std::clamp(v, 0.f, 1.f) * 100.f, 0) + "%"; };
	std::string report = "RGO GOVERNMENT FUNDING WORLD AUDIT government=" + std::to_string(projects.size())
		+ " private=" + std::to_string(private_projects);
	if(projects.empty()) report += "\nNo government-funded RGO upgrades are currently active.";
	for(size_t i = 0; i < std::min<size_t>(40, projects.size()); ++i) {
		auto const& project = projects[i];
		std::string status = project.goods_fraction < 0.999f ? "stockpiling"
			: (project.capacity <= 0.f ? "supplied-queued" : "building");
		report += "\n" + std::to_string(i + 1) + ". "
			+ text::produce_simple_string(*state, text::get_name(*state, project.nation)) + ": "
			+ text::produce_simple_string(*state, state->world.commodity_get_name(project.commodity)) + " - "
			+ text::produce_simple_string(*state, state->world.province_get_name(project.province))
			+ " status=" + status + " goods=" + pct(project.goods_fraction)
			+ " build=" + pct(project.progress)
			+ " capacity=" + text::format_float(project.capacity * 100.f, 0) + "%";
	}
	if(projects.size() > 40) report += "\n..." + std::to_string(projects.size() - 40) + " additional projects omitted.";
	static_cast<ui::console_window*>(state->ui_state.console_window)->replace_list(*state,
		"rgo-government-audit\n" + report + "\n");
	return p + 2;
}

int32_t* f_urban_ai_audit(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) return p + 2;
	auto* state = reinterpret_cast<sys::state*>(fif::get_global_var(*e, "state-ptr")->data);
	auto nation = state->local_player_nation;
	if(!nation) return p + 2;
	dcon::civic_building_type_id urban_type{};
	dcon::civic_building_type_id road_type{};
	for(auto type : state->world.in_civic_building_type) {
		if(type.get_is_urban_center()) urban_type = type.id;
		if(type.get_is_road_network()) road_type = type.id;
	}
	struct candidate {
		dcon::province_id province{};
		float score = 0.f;
		float population = 0.f;
		float literacy = 0.f;
		float control = 0.f;
		int32_t level = 0;
		int32_t used = 0;
		int32_t capacity = 0;
		int32_t factories = 0;
		int32_t factory_projects = 0;
		int32_t roads = 0;
		int32_t rail = 0;
		bool capital = false;
		bool state_capital = false;
		bool workers = false;
		bool colonial = false;
		bool already_building = false;
	};
	std::vector<candidate> candidates;
	state->world.nation_for_each_province_ownership(nation, [&](auto ownership) {
		auto province = state->world.province_ownership_get_province(ownership);
		if(state->world.province_get_nation_from_province_control(province) != nation) return;
		auto population = state->world.province_get_demographics(province, demographics::total);
		auto literacy = state->world.province_get_demographics(province, demographics::literacy)
			/ std::max(1.f, population);
		auto control = state->world.province_get_control_ratio(province);
		auto state_instance = state->world.province_get_state_membership(province);
		auto factories = int32_t(state->world.province_get_factory_location(province).end()
			- state->world.province_get_factory_location(province).begin());
		auto factory_projects = int32_t(state->world.province_get_factory_construction(province).end()
			- state->world.province_get_factory_construction(province).begin());
		auto level = urban_type ? int32_t(state->world.province_get_civic_building_level(province, urban_type.index())) : 0;
		auto capacity = civic_buildings::province_urban_building_capacity(*state, province);
		auto used = civic_buildings::province_used_urban_building_capacity(*state, province);
		auto roads = road_type ? int32_t(state->world.province_get_civic_building_level(province, road_type.index())) : 0;
		auto rail = int32_t(state->world.province_get_building_level(province,
			uint8_t(economy::province_building_type::railroad)));
		auto capital = state->world.nation_get_capital(nation) == province;
		auto state_capital = state->world.state_instance_get_capital(state_instance) == province;
		auto workers = ai::province_has_workers(*state, province);
		auto colonial = state->world.province_get_is_colonial(province);
		auto pressure = capacity <= 0 ? (workers ? 1.f : 0.f)
			: std::clamp(float(used + factory_projects) / float(std::max(1, capacity)), 0.f, 1.5f);
		float score = std::min(50.f, std::log1p(std::max(0.f, population)) * 4.f)
			+ literacy * 15.f + control * 10.f + pressure * 30.f
			+ float(factories + factory_projects) * 8.f + float(roads + rail) * 4.f
			+ (workers ? 10.f : 0.f) + (capital ? 30.f : 0.f) + (state_capital ? 12.f : 0.f)
			- (colonial ? 40.f : 0.f);
		candidates.push_back({province, score, population, literacy, control, level, used, capacity,
			factories, factory_projects, roads, rail, capital, state_capital, workers, colonial,
			urban_type && civic_buildings::upgrade_in_progress(*state, province, urban_type)});
	});
	std::sort(candidates.begin(), candidates.end(), [](candidate const& a, candidate const& b) {
		if(a.score != b.score) return a.score > b.score;
		return a.province.index() < b.province.index();
	});
	std::string report = "URBAN CENTER AI AUDIT (read-only; proposed Foundry ranking)\nNation: "
		+ text::produce_simple_string(*state, text::get_name(*state, nation))
		+ " candidates=" + std::to_string(candidates.size())
		+ "\nLIVE RULES: profitable factory intent requests an urban center when factory unlock/capacity blocks it;"
		  " one national urban project at a time, 25K population, 5K workers, 80% control, non-colonial, 2x cost reserve."
		  " Existing hubs prepare the next tier at 80% slot use.";
	for(size_t i = 0; i < std::min<size_t>(12, candidates.size()); ++i) {
		auto const& c = candidates[i];
		std::string reason = c.capacity <= 0 ? "needs-first-city"
			: (c.used + c.factory_projects >= c.capacity ? "capacity-full" : "growth-hub");
		if(c.capital) reason += ",national-capital";
		else if(c.state_capital) reason += ",state-capital";
		if(c.factories + c.factory_projects > 0) reason += ",industry-present";
		if(c.roads + c.rail > 0) reason += ",infrastructure";
		if(c.colonial) reason += ",colonial-penalty";
		report += "\n" + std::to_string(i + 1) + ". "
			+ text::produce_simple_string(*state, state->world.province_get_name(c.province))
			+ " score=" + text::format_float(c.score, 1)
			+ " reason=" + reason
			+ " urban=" + std::to_string(c.level)
			+ " slots=" + std::to_string(c.used) + "/" + std::to_string(c.capacity)
			+ " factories=" + std::to_string(c.factories) + "+" + std::to_string(c.factory_projects)
			+ " pop=" + text::prettify(int32_t(c.population))
			+ " literacy=" + text::format_percentage(c.literacy, 0)
			+ " roads/rail=" + std::to_string(c.roads) + "/" + std::to_string(c.rail)
			+ (c.already_building ? " BUILDING" : "");
	}
	static_cast<ui::console_window*>(state->ui_state.console_window)->replace_list(*state,
		"urban-ai-audit\n" + report + "\n");
	return p + 2;
}

int32_t* f_urban_ai_world_audit(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) return p + 2;
	auto* state = reinterpret_cast<sys::state*>(fif::get_global_var(*e, "state-ptr")->data);
	dcon::civic_building_type_id urban_type{};
	for(auto type : state->world.in_civic_building_type)
		if(type.get_is_urban_center()) {
			urban_type = type.id;
			break;
		}
	std::string report = "URBAN CENTER AI WORLD ACTIVITY (actual projects)";
	int32_t active = 0;
	int32_t ai_nations = 0;
	int32_t uncivilized = 0;
	int32_t already_has_city = 0;
	int32_t no_eligible_site = 0;
	int32_t affordable_bootstrap = 0;
	int32_t insufficient_treasury = 0;
	std::vector<std::string> blocked_examples;
	if(urban_type) {
		for(auto nation : state->world.in_nation) {
			if(!nation.get_owned_province_count() || nation.get_is_player_controlled()) continue;
			++ai_nations;
			if(!nation.get_is_civilized()) ++uncivilized;
			bool has_city = false;
			bool has_project = false;
			for(auto ownership : nation.get_province_ownership()) {
				auto province = ownership.get_province();
				if(state->world.province_get_civic_building_level(province, urban_type.index()) > 0)
					has_city = true;
				if(!civic_buildings::upgrade_in_progress(*state, province, urban_type)) continue;
				has_project = true;
				++active;
				auto level = state->world.province_get_civic_building_level(province, urban_type.index());
				auto progress = state->world.province_get_civic_building_progress(province, urban_type.index());
				auto& purchased = state->world.province_get_civic_building_purchased_goods(province, urban_type.index());
				auto const& definition = state->world.civic_building_type_get_levels(urban_type)[level];
				float bought = 0.f;
				float required = 0.f;
				for(uint32_t i = 0; i < economy::commodity_set::set_size && definition.cost.commodity_type[i]; ++i) {
					required += definition.cost.commodity_amounts[i];
					bought += std::min(purchased.commodity_amounts[i], definition.cost.commodity_amounts[i]);
				}
				auto goods = required > 0.f ? bought / required : 1.f;
				auto status = progress > 0.f ? "building" : (goods >= 0.999f ? "supplied-queued" : "stockpiling");
				report += "\n" + text::produce_simple_string(*state, text::get_name(*state, nation.id))
					+ " - " + text::produce_simple_string(*state, state->world.province_get_name(province))
					+ " level " + std::to_string(int32_t(level)) + "->" + std::to_string(int32_t(level) + 1)
					+ " " + status + " goods=" + text::format_percentage(goods, 0)
					+ " build=" + text::format_percentage(progress, 0);
			}
			if(has_project) continue;
			if(has_city) {
				++already_has_city;
				continue;
			}

			dcon::province_id best{};
			float best_population = 0.f;
			for(auto ownership : nation.get_province_ownership()) {
				auto province = ownership.get_province();
				auto population = state->world.province_get_demographics(province, demographics::total);
				if(state->world.province_get_is_colonial(province)
						|| state->world.province_get_nation_from_province_control(province) != nation
						|| population < 15'000.f
						|| state->world.province_get_control_ratio(province) < 0.5f
						|| !civic_buildings::can_begin_upgrade(*state, nation, province, urban_type))
					continue;
				if(population > best_population) {
					best = province;
					best_population = population;
				}
			}
			if(!best) {
				++no_eligible_site;
				if(blocked_examples.size() < 8)
					blocked_examples.push_back(text::produce_simple_string(*state, text::get_name(*state, nation.id))
						+ ": no eligible site");
				continue;
			}
			auto level = state->world.province_get_civic_building_level(best, urban_type.index());
			auto const& goods = state->world.civic_building_type_get_levels(urban_type)[level].cost;
			float cost = 0.f;
			for(uint32_t i = 0; i < economy::commodity_set::set_size && goods.commodity_type[i]; ++i)
				cost += goods.commodity_amounts[i] * state->world.commodity_get_cost(goods.commodity_type[i]);
			auto treasury = nation.get_stockpiles(economy::money);
			if(treasury >= cost * 1.25f) {
				++affordable_bootstrap;
				if(blocked_examples.size() < 8)
					blocked_examples.push_back(text::produce_simple_string(*state, text::get_name(*state, nation.id))
						+ ": ELIGIBLE at " + text::produce_simple_string(*state, state->world.province_get_name(best))
						+ " treasury=" + text::format_money(treasury) + " reserve=" + text::format_money(cost * 1.25f));
			} else {
				++insufficient_treasury;
				if(blocked_examples.size() < 8)
					blocked_examples.push_back(text::produce_simple_string(*state, text::get_name(*state, nation.id))
						+ ": insufficient treasury=" + text::format_money(treasury)
						+ " reserve=" + text::format_money(cost * 1.25f));
			}
		}
	}
	if(active == 0) report += "\nNo AI-funded urban projects are active.";
	report += "\nActive=" + std::to_string(active)
		+ " AI nations=" + std::to_string(ai_nations)
		+ " uncivilized=" + std::to_string(uncivilized)
		+ " already-city=" + std::to_string(already_has_city)
		+ " no-site=" + std::to_string(no_eligible_site)
		+ " affordable-waiting=" + std::to_string(affordable_bootstrap)
		+ " cash-blocked=" + std::to_string(insufficient_treasury);
	if(!urban_type) report += "\nBLOCKER: Urban Center definition is unavailable.";
	for(auto const& example : blocked_examples) report += "\n" + example;
	static_cast<ui::console_window*>(state->ui_state.console_window)->replace_list(*state,
		"urban-ai-world-audit\n" + report + "\n");
	return p + 2;
}

int32_t* f_add_road(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode))
		return p + 2;

	auto state_global = fif::get_global_var(*e, "state-ptr");
	auto* state = reinterpret_cast<sys::state*>(state_global->data);
	auto province = state->map_state.selected_province;
	if(!province || !state->world.province_is_valid(province)) {
		log_to_console(*state, state->ui_state.console_window, u"Select a land province first.");
		return p + 2;
	}

	dcon::civic_building_type_id road_type;
	for(auto type : state->world.in_civic_building_type) {
		if(type.get_is_road_network()) {
			road_type = type.id;
			break;
		}
	}
	if(!road_type) {
		log_to_console(*state, state->ui_state.console_window, u"Road Network definition is unavailable; rebuild the scenario.");
		return p + 2;
	}

	auto level = state->world.province_get_civic_building_level(province, road_type.index());
	auto maximum = state->world.civic_building_type_get_level_count(road_type);
	if(level >= maximum) {
		log_to_console(*state, state->ui_state.console_window, u"Selected province already has the maximum road level.");
		return p + 2;
	}

	// Development/testing shortcut: install exactly one completed level without
	// goods, money, queue time, or construction capacity.
	state->world.province_set_civic_building_level(province, road_type.index(), uint8_t(level + 1));
	state->world.province_set_civic_building_progress(province, road_type.index(), 0.f);
	state->world.province_set_civic_building_active(province, road_type.index(), uint8_t(0));
	state->world.province_set_civic_building_purchased_goods(province, road_type.index(), economy::commodity_set{});
	state->trade_route_cached_values_out_of_date = true;
	log_to_console(*state, state->ui_state.console_window, u"Added one Road Network level to the selected province.");
	return p + 2;
}

int32_t* f_foundry_help(fif::state_stack&, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) return p + 2;
	auto* state = reinterpret_cast<sys::state*>(fif::get_global_var(*e, "state-ptr")->data);
	std::string report =
		"FOUNDRY COMMANDS (also documented in docs/FOUNDRY_CONSOLE_COMMANDS.md)\n"
		"Syntax note: commodity comes first, e.g. coal market-shadow\n\n"
		"MARKET SNAPSHOTS (run once; read-only)\n"
		"<good> market-shadow - selected province/capital, 5 nearby markets\n"
		"<good> market-shadow-wide - selected province/capital, 25 markets\n"
		"market-shadow-batch - 12-good snapshot across 25 markets\n\n"
		"LIVE MARKET AUDITS (continue while time runs)\n"
		"<good> market-live-audit - daily single-good comparison\n"
		"market-live-audit-basket - daily 12-good/25-market comparison\n"
		"market-live-audit-all - daily all-good/25-market comparison\n"
		"market-live-audit-50 | market-live-audit-100 - scale test\n"
		"market-live-audit-world - all markets, weekly routed refresh\n"
		"market-access-audit-world - daily lightweight state-access model\n"
		"market-live-status - display current audit and measurements\n"
		"market-live-audit-off - STOP any live market audit\n\n"
		"RGO AI (read-only reports)\n"
		"rgo-ai-audit - current nation's ranked RGO recommendations\n"
		"rgo-ai-world-audit - actual AI RGO selections worldwide\n"
		"rgo-government-audit - government-funded RGO projects worldwide\n\n"
		"URBAN AI (read-only reports)\n"
		"urban-ai-audit - current nation's ranked urban candidates/rules\n"
		"urban-ai-world-audit - actual AI urban projects worldwide\n\n"
		"TEST SHORTCUTS (change game state)\n"
		"add-road - instantly add one road level to selected province\n\n"
		"foundry-help - show this list";
	static_cast<ui::console_window*>(state->ui_state.console_window)->replace_list(*state,
		"foundry-help\n" + report + "\n");
	return p + 2;
}
int32_t* f_provid(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	bool toggle_state = s.main_data_back(0) != 0;
	s.pop_main();

	state->cheat_data.show_province_id_tooltip = toggle_state;
	return p + 2;
}
int32_t* f_uidebug(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	bool toggle_state = s.main_data_back(0) != 0;
	s.pop_main();

	state->cheat_data.ui_debug_mode = toggle_state;
	return p + 2;
}
int32_t* f_fire_event(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		s.pop_main();
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	auto id = int32_t(s.main_data_back(0));
	s.pop_main();

	dcon::nation_id to_nation_b;
	to_nation_b.value = dcon::nation_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	dcon::free_national_event_id ev;
	for(auto v : state->world.in_free_national_event) {
		if(v.get_legacy_id() == uint32_t(id)) {
			ev = v;
			break;
		}
	}
	if(!ev) {
		e->report_error("no free national event found with that id");
		e->mode = fif::fif_mode::error;
	} else {
		event::trigger_national_event(*state, ev, to_nation_b, state->current_date.value, id ^ to_nation_b.index());
	}

	return p + 2;
}
int32_t* f_nation_name(fif::state_stack& s, int32_t* p, fif::environment* e) {
	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		s.push_back_main(state->type_text_key, 0, nullptr);
		return p + 2;
	}



	dcon::nation_id to_nation_b;
	to_nation_b.value = dcon::nation_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	auto name = text::get_name(*state, to_nation_b);

	s.push_back_main(state->type_text_key, int64_t(name.value), nullptr);

	return p + 2;
}

int32_t* f_nation_money_pools(fif::state_stack& s, int32_t* p, fif::environment* e) {
	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	if(fif::typechecking_mode(e->mode)) {
		if(fif::typechecking_failed(e->mode))
			return p + 2;
		s.pop_main();
		s.push_back_main(state->type_text_key, 0, nullptr);
		return p + 2;
	}



	dcon::nation_id to_nation_b;
	to_nation_b.value = dcon::nation_id::value_base_t(s.main_data_back(0));
	s.pop_main();

	//auto name = text::get_name(*state, to_nation_b);

	auto values = economy::breakdown_nation_monetary_structure(*state, to_nation_b);
	float container;

	int64_t data = 0;
	memcpy(&data, &values.total, 4);
	s.push_back_main(fif::fif_f32, data, nullptr);

	memcpy(&data, &values.nation, 4);
	s.push_back_main(fif::fif_f32, data, nullptr);

	memcpy(&data, &values.market, 4);
	s.push_back_main(fif::fif_f32, data, nullptr);

	memcpy(&data, &values.pops, 4);
	s.push_back_main(fif::fif_f32, data, nullptr);

	memcpy(&data, &values.rgo, 4);
	s.push_back_main(fif::fif_f32, data, nullptr);

	memcpy(&data, &values.factory, 4);
	s.push_back_main(fif::fif_f32, data, nullptr);

	container = values.educators + values.ports + values.landlords + values.artisans;
	memcpy(&data, &container, 4);
	s.push_back_main(fif::fif_f32, data, nullptr);

	container = values.bank + values.investment_pool;
	memcpy(&data, &container, 4);
	s.push_back_main(fif::fif_f32, data, nullptr);

	return p + 2;
}


inline int32_t* compile_modifier(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(!fif::typechecking_failed(e->mode)) {
			s.pop_main();
		}
		return p + 2;
	}
	if(e->mode != fif::fif_mode::interpreting) {
		e->report_error("attempted to compile a definition inside a definition");
		e->mode = fif::fif_mode::error;
		return p + 2;
	}

	auto state_global = fif::get_global_var(*e, "state-ptr");
	sys::state* state = (sys::state*)(state_global->data);

	auto index = s.main_data_back(0);
	s.pop_main();

	dcon::value_modifier_key mkey{ dcon::value_modifier_key::value_base_t(index) };
	std::string body = "" + fif_trigger::multiplicative_modifier(*state, mkey) + " drop drop r> ";
	std::string name_str = "ttest";

	e->dict.words.insert_or_assign(name_str, int32_t(e->dict.word_array.size()));
	e->dict.word_array.emplace_back();
	e->dict.word_array.back().source = body;

	return p + 2;
}

inline int32_t* load_file(fif::state_stack& s, int32_t* p, fif::environment* e) {
	if(fif::typechecking_mode(e->mode)) {
		if(!fif::typechecking_failed(e->mode)) {
			s.pop_main();
		}
		return p + 2;
	}
	if(e->mode != fif::fif_mode::interpreting) {
		e->report_error("attempted to load a file inside a definition");
		e->mode = fif::fif_mode::error;
		return p + 2;
	}

	auto name_token = fif::read_token(e->source_stack.back(), *e);

	auto dir = simple_fs::get_or_create_root_documents();
	auto file = simple_fs::open_file(dir, simple_fs::utf8_to_native(name_token.content));
	if(file) {
		auto content = simple_fs::view_contents(*file);
		fif::interpreter_stack values{ };
		fif::run_fif_interpreter(*e, std::string_view{content.data, content.file_size}, values);

	} else {
		e->mode = fif::fif_mode::error;
		e->report_error("could not open file");
		return p + 2;
	}
	return p + 2;
}


void ui::initialize_console_fif_environment(sys::state& state) {
	if(state.fif_environment)
		return;

	std::lock_guard lg{ state.lock_console_strings };

	state.fif_environment = std::make_unique<fif::environment>();

	int32_t error_count = 0;
	std::string error_list;
	state.fif_environment->report_error = [&](std::string_view s) {
		state.console_command_error += std::string("?R ERROR: ") + std::string(s) + "?W\\n";
	};

	fif::common_fif_environment(state, *state.fif_environment);

	fif::interpreter_stack values{ };

	fif::run_fif_interpreter(*state.fif_environment,
		" :struct localized i32 value ; "
		" :s localize text_key s: >index make localized .value! ; ",
		values);


	state.type_text_key = state.fif_environment->dict.types.find("text_key")->second;
	state.type_localized_key = state.fif_environment->dict.types.find("localized")->second;

	//
	// Add predefined names and tags
	//
	auto return_to_string = [&](dcon::text_key k) {
		std::string rvalue{state.to_string_view(k) };
		for(auto& c : rvalue) {
			if(uint8_t(c) == 127)
				c = '_';
		}
		return rvalue;
	};
	for(auto n : state.world.in_national_identity) {
		auto tag_str = nations::int_to_tag(n.get_identifying_int());
		fif::run_fif_interpreter(*state.fif_environment,
			std::string(" : ") + tag_str + " " + std::to_string(n.id.index()) + " >national_identity_id identity_holder-identity nation @ ; ",
		values);
		state.fif_environment->mode = fif::fif_mode::interpreting;
	}
	for(auto r : state.world.in_religion) {
		fif::run_fif_interpreter(*state.fif_environment,
			std::string(" : ") + return_to_string(r.get_name()) + " " + std::to_string(r.id.index()) + " >religion_id ; ",
		values);
		state.fif_environment->mode = fif::fif_mode::interpreting;
	}
	for(auto r : state.world.in_culture) {
		fif::run_fif_interpreter(*state.fif_environment,
			std::string(" : ") + return_to_string(r.get_name()) + " " + std::to_string(r.id.index()) + " >culture_id ; ",
		values);
		state.fif_environment->mode = fif::fif_mode::interpreting;
	}
	for(auto r : state.world.in_commodity) {
		fif::run_fif_interpreter(*state.fif_environment,
			std::string(" : ") + return_to_string(r.get_name()) + " " + std::to_string(r.id.index()) + " >commodity_id ; ",
		values);
		state.fif_environment->mode = fif::fif_mode::interpreting;
	}
	for(auto r : state.world.in_ideology) {
		fif::run_fif_interpreter(*state.fif_environment,
			std::string(" : ") + return_to_string(r.get_name()) + " " + std::to_string(r.id.index()) + " >ideology_id ; ",
		values);
		state.fif_environment->mode = fif::fif_mode::interpreting;
	}
	for(auto r : state.world.in_issue) {
		fif::run_fif_interpreter(*state.fif_environment,
			std::string(" : ") + return_to_string(r.get_name()) + " " + std::to_string(r.id.index()) + " >issue_id ; ",
		values);
		state.fif_environment->mode = fif::fif_mode::interpreting;
	}
	for(auto r : state.world.in_issue_option) {
		fif::run_fif_interpreter(*state.fif_environment,
			std::string(" : ") + return_to_string(r.get_name()) + " " + std::to_string(r.id.index()) + " >issue_option_id ; ",
		values);
		state.fif_environment->mode = fif::fif_mode::interpreting;
	}
	for(auto r : state.world.in_reform) {
		fif::run_fif_interpreter(*state.fif_environment,
			std::string(" : ") + return_to_string(r.get_name()) + " " + std::to_string(r.id.index()) + " >reform_id ; ",
		values);
		state.fif_environment->mode = fif::fif_mode::interpreting;
	}
	for(auto r : state.world.in_reform_option) {
		fif::run_fif_interpreter(*state.fif_environment,
			std::string(" : ") + return_to_string(r.get_name()) + " " + std::to_string(r.id.index()) + " >reform_option_id ; ",
		values);
		state.fif_environment->mode = fif::fif_mode::interpreting;
	}
	for(auto r : state.world.in_cb_type) {
		fif::run_fif_interpreter(*state.fif_environment,
			std::string(" : ") + return_to_string(r.get_name()) + " " + std::to_string(r.id.index()) + " >cb_type_id ; ",
		values);
		state.fif_environment->mode = fif::fif_mode::interpreting;
	}
	for(auto r : state.world.in_pop_type) {
		fif::run_fif_interpreter(*state.fif_environment,
			std::string(" : ") + return_to_string(r.get_name()) + " " + std::to_string(r.id.index()) + " >pop_type_id ; ",
		values);
		state.fif_environment->mode = fif::fif_mode::interpreting;
	}
	for(auto r : state.world.in_rebel_type) {
		fif::run_fif_interpreter(*state.fif_environment,
			std::string(" : ") + return_to_string(r.get_name()) + " " + std::to_string(r.id.index()) + " >rebel_type_id ; ",
		values);
		state.fif_environment->mode = fif::fif_mode::interpreting;
	}
	for(auto r : state.world.in_government_type) {
		fif::run_fif_interpreter(*state.fif_environment,
			std::string(" : ") + return_to_string(r.get_name()) + " " + std::to_string(r.id.index()) + " >government_type_id ; ",
		values);
		state.fif_environment->mode = fif::fif_mode::interpreting;
	}
	for(auto r : state.world.in_province) {
		fif::run_fif_interpreter(*state.fif_environment,
			std::string(" : ") + return_to_string(r.get_name()) + " " + std::to_string(r.id.index()) + " >province_id ; ",
		values);
		state.fif_environment->mode = fif::fif_mode::interpreting;
	}
	for(auto r : state.world.in_state_definition) {
		fif::run_fif_interpreter(*state.fif_environment,
			std::string(" : ") + return_to_string(r.get_name()) + " " + std::to_string(r.id.index()) + " >state_definition_id ; ",
		values);
		state.fif_environment->mode = fif::fif_mode::interpreting;
	}
	for(auto r : state.world.in_technology) {
		fif::run_fif_interpreter(*state.fif_environment,
			std::string(" : ") + return_to_string(r.get_name()) + " " + std::to_string(r.id.index()) + " >technology_id ; ",
		values);
		state.fif_environment->mode = fif::fif_mode::interpreting;
	}
	for(auto r : state.world.in_invention) {
		fif::run_fif_interpreter(*state.fif_environment,
			std::string(" : ") + return_to_string(r.get_name()) + " " + std::to_string(r.id.index()) + " >invention_id ; ",
		values);
		state.fif_environment->mode = fif::fif_mode::interpreting;
	}

	//
	// Add console commands here
	//

	auto nation_id_type = state.fif_environment->dict.types.find("nation_id")->second;
	auto prov_id_type = state.fif_environment->dict.types.find("province_id")->second;
	auto commodity_id_type = state.fif_environment->dict.types.find("commodity_id")->second;

	fif::add_import("clear", nullptr, f_clear, {}, {}, * state.fif_environment);
	fif::add_import("fps", nullptr, f_fps, { fif::fif_bool }, {}, * state.fif_environment);
	fif::add_import("change-tag", nullptr, f_change_tag, { nation_id_type }, {}, *state.fif_environment);
	fif::add_import("set-westernized", nullptr, f_set_westernized, { nation_id_type, fif::fif_bool }, {}, * state.fif_environment);
	fif::add_import("make-crisis", nullptr, f_make_crisis, { }, {}, * state.fif_environment);
	fif::add_import("end-game", nullptr, f_end_game, { }, {}, * state.fif_environment);
	fif::add_import("set-mil", nullptr, f_set_mil, { nation_id_type, fif::fif_f32 }, {}, * state.fif_environment);
	fif::add_import("set-con", nullptr, f_set_con, { nation_id_type, fif::fif_f32 }, {}, * state.fif_environment);
	fif::add_import("make-allied", nullptr, f_make_allied, { nation_id_type, nation_id_type }, {}, * state.fif_environment);
	fif::add_import("dump-oos", nullptr, f_dump_oos, { }, {}, * state.fif_environment);
	fif::add_import("cheat-wargoals", nullptr, f_cheat_wargoals, { fif::fif_bool }, {}, *state.fif_environment);
	fif::add_import("cheat-reforms", nullptr, f_cheat_reforms, { fif::fif_bool }, {}, * state.fif_environment);
	fif::add_import("cheat-diplomacy", nullptr, f_cheat_deals, { fif::fif_bool }, {}, * state.fif_environment);
	fif::add_import("cheat-decisions", nullptr, f_cheat_decisions, { fif::fif_bool }, {}, * state.fif_environment);
	fif::add_import("cheat-decision-potential", nullptr, f_cheat_decision_potential, { fif::fif_bool }, {}, * state.fif_environment);
	fif::add_import("cheat-army", nullptr, f_cheat_army, { fif::fif_bool }, {}, * state.fif_environment);
	fif::add_import("cheat-navy", nullptr, f_cheat_navy, { fif::fif_bool }, {}, * state.fif_environment);
	fif::add_import("cheat-factories", nullptr, f_cheat_factories, { fif::fif_bool }, {}, * state.fif_environment);
	fif::add_import("daily-oos-check", nullptr, f_daily_oos, { fif::fif_bool }, {}, * state.fif_environment);
	fif::add_import("set-auto-choice", nullptr, f_set_auto_choice, { fif::fif_bool }, {}, *state.fif_environment);
	fif::add_import("complete-construction", nullptr, f_complete_construction, { nation_id_type }, {}, * state.fif_environment);
	fif::add_import("instant-research", nullptr, f_instant_research, { nation_id_type, fif::fif_bool }, {}, * state.fif_environment);
	fif::add_import("spectate", nullptr, f_spectate, { }, {}, *state.fif_environment);
	fif::add_import("conquer", nullptr, f_conquer, { nation_id_type , nation_id_type }, {}, * state.fif_environment);
	fif::add_import("make-core", nullptr, f_make_core, { prov_id_type , nation_id_type }, {}, * state.fif_environment);
	fif::add_import("remove-core", nullptr, f_remove_core, { prov_id_type , nation_id_type }, {}, * state.fif_environment);
	fif::add_import("set-owner", nullptr, f_set_owner, { prov_id_type , nation_id_type }, {}, * state.fif_environment);
	fif::add_import("set-controller", nullptr, f_set_controller, { prov_id_type , nation_id_type }, {}, * state.fif_environment);
	fif::add_import("add-days", nullptr, f_add_days, { fif::fif_i32 }, {}, * state.fif_environment);
	fif::add_import("save-map", nullptr, f_save_map, { fif::fif_i32 }, {}, * state.fif_environment);
	fif::add_import("dump-econ", nullptr, f_dump_econ, {  }, {}, * state.fif_environment);
	fif::add_import("market-shadow", nullptr, f_market_shadow, { commodity_id_type }, {}, *state.fif_environment);
	fif::add_import("market-shadow-wide", nullptr, f_market_shadow_wide, { commodity_id_type }, {}, *state.fif_environment);
	fif::add_import("market-shadow-batch", nullptr, f_market_shadow_batch, {}, {}, *state.fif_environment);
	fif::add_import("market-live-audit", nullptr, f_market_live_audit, { commodity_id_type }, {}, *state.fif_environment);
	fif::add_import("market-live-audit-basket", nullptr, f_market_live_audit_basket, {}, {}, *state.fif_environment);
	fif::add_import("market-live-audit-all", nullptr, f_market_live_audit_all, {}, {}, *state.fif_environment);
	fif::add_import("market-live-audit-50", nullptr, f_market_live_audit_50, {}, {}, *state.fif_environment);
	fif::add_import("market-live-audit-100", nullptr, f_market_live_audit_100, {}, {}, *state.fif_environment);
	fif::add_import("market-live-audit-world", nullptr, f_market_live_audit_world, {}, {}, *state.fif_environment);
	fif::add_import("market-access-audit-world", nullptr, f_market_access_audit_world, {}, {}, *state.fif_environment);
	fif::add_import("market-live-audit-off", nullptr, f_market_live_audit_off, {}, {}, *state.fif_environment);
	fif::add_import("market-live-status", nullptr, f_market_live_status, {}, {}, *state.fif_environment);
	fif::add_import("rgo-ai-audit", nullptr, f_rgo_ai_audit, {}, {}, *state.fif_environment);
	fif::add_import("rgo-ai-world-audit", nullptr, f_rgo_ai_world_audit, {}, {}, *state.fif_environment);
	fif::add_import("rgo-government-audit", nullptr, f_rgo_government_audit, {}, {}, *state.fif_environment);
	fif::add_import("urban-ai-audit", nullptr, f_urban_ai_audit, {}, {}, *state.fif_environment);
	fif::add_import("urban-ai-world-audit", nullptr, f_urban_ai_world_audit, {}, {}, *state.fif_environment);
	fif::add_import("add-road", nullptr, f_add_road, {}, {}, *state.fif_environment);
	fif::add_import("foundry-help", nullptr, f_foundry_help, {}, {}, *state.fif_environment);
	fif::add_import("provid", nullptr, f_provid, { fif::fif_bool }, {}, * state.fif_environment);
	fif::add_import("ui-debug", nullptr, f_uidebug, { fif::fif_bool }, {}, *state.fif_environment);
	fif::add_import("fire-event", nullptr, f_fire_event, { nation_id_type, fif::fif_i32 }, {}, * state.fif_environment);
	fif::add_import("nation-name", nullptr, f_nation_name, { nation_id_type }, { state.type_text_key }, *state.fif_environment);
	fif::add_import("load-file", nullptr, load_file, {}, {}, * state.fif_environment);
	fif::add_import("nation-monetary-pools", nullptr, f_nation_money_pools, {nation_id_type}, { fif::fif_f32, fif::fif_f32, fif::fif_f32, fif::fif_f32, fif::fif_f32, fif::fif_f32, fif::fif_f32, fif::fif_f32 }, * state.fif_environment);

	fif::add_import("compile-mod", nullptr, compile_modifier, { fif::fif_i32 }, { }, * state.fif_environment);

	fif::run_fif_interpreter(*state.fif_environment,
		" : no-sea-line 0 ; : no-blend 1 ; : no-sea-line-2 2 ; : blend-no-sea 3 ; : vanilla 4 ; ",
		values);
	fif::run_fif_interpreter(*state.fif_environment,
		" :s name nation_id s: nation-name ; ",
		values);

	fif::run_fif_interpreter(*state.fif_environment,
		" : player " + std::to_string(offsetof(sys::state, local_player_nation)) + " state-ptr @ buf-add ptr-cast ptr(nation_id) ; ",
		values);

	fif::run_fif_interpreter(*state.fif_environment,
		" : selected-prov " + std::to_string(offsetof(sys::state, map_state) + offsetof(map::map_state, selected_province)) + " state-ptr @ buf-add ptr-cast ptr(province_id) ; ",
		values);

	assert(state.fif_environment->mode != fif::fif_mode::error);
}

std::string ui::format_fif_value(sys::state& state, int64_t data, int32_t type) {
	if(type == fif::fif_i8 || type == fif::fif_i16 || type == fif::fif_i32 || type == fif::fif_i64) {
		return std::to_string(data);
	} else if(type == fif::fif_u8 || type == fif::fif_u16 || type == fif::fif_u32 || type == fif::fif_u64) {
		return std::to_string(uint64_t(data));
	} else if(type == fif::fif_f32) {
		float v = 0;
		memcpy(&v, &data, 4);
		return std::to_string(v);
	} else if(type == fif::fif_f64) {
		double v = 0;
		memcpy(&v, &data, 8);
		return std::to_string(v);
	} else if(type == fif::fif_bool) {
		if(data != 0)
			return "true";
		return "false";
	} else if(type == fif::fif_opaque_ptr) {
		return "#ptr(nil)";
	} else if(type == state.type_text_key) {
		dcon::text_key k;
		k.value = dcon::text_key::value_base_t(data);
		return std::string("\"") + std::string(state.to_string_view(k)) + "\"";
	} else if(type == state.type_localized_key) {
		uint32_t localized_index = uint32_t(data);
		dcon::text_key k{ localized_index };
		if(!k)
			return "\"\"";

		std::string_view sv;
		if(auto it = state.locale_key_to_text_sequence.find(k); it != state.locale_key_to_text_sequence.end()) {
			return std::string("\"") + std::string{ state.locale_string_view(it->second) } + "\"";
		} else {
			return std::string("\"") + std::string{ state.to_string_view(k) } + "\"";
		}
	} else if(type == -1) {
		return "#nil";
	} else if(type == fif::fif_type) {
		return "#type";
	} else {
		if(state.fif_environment->dict.type_array[type].decomposed_types_count >= 2) {
			auto main_type = state.fif_environment->dict.all_stack_types[state.fif_environment->dict.type_array[type].decomposed_types_start];
			if(main_type == fif::fif_ptr) {
				return "#ptr";
			} else {
				return "#struct";
			}
		} else {
			return "#unknown";
		}
	}
}

void ui::console_window::show_toggle(sys::state& state) {
	assert(state.ui_state.console_window);
	if(state.ui_state.console_window->is_visible()) { //close
		sound::play_interface_sound(state, sound::get_console_close_sound(state), state.user_settings.master_volume * state.user_settings.interface_volume);
	} else { //open
		sound::play_interface_sound(state, sound::get_console_open_sound(state), state.user_settings.master_volume * state.user_settings.interface_volume);
	}

	state.ui_state.console_window->set_visible(state, !state.ui_state.console_window->is_visible());
	if(state.ui_state.console_window->is_visible())
		state.ui_state.root->move_child_to_front(state.ui_state.console_window);
}
