#include "gui_population_window.hpp"
#include "alice_ui.hpp"
#include "demographics.hpp"
#include "gui_modifier_tooltips.hpp"
#include "gui_trigger_tooltips.hpp"
#include "triggers.hpp"

namespace ui {

void describe_conversion(sys::state& state, text::columnar_layout& contents, dcon::pop_id ids) { alice_ui::describe_conversion(state, contents, ids); }
void describe_migration(sys::state& state, text::columnar_layout& contents, dcon::pop_id ids) { alice_ui::describe_migration(state, contents, ids); }
void describe_colonial_migration(sys::state& state, text::columnar_layout& contents, dcon::pop_id ids) { alice_ui::describe_colonial_migration(state, contents, ids); }
void describe_emigration(sys::state& state, text::columnar_layout& contents, dcon::pop_id ids) { alice_ui::describe_emigration(state, contents, ids); }
void describe_promotion_demotion(sys::state& state, text::columnar_layout& contents, dcon::pop_id ids) {
	alice_ui::describe_promotion(state, contents, ids);
	alice_ui::describe_demotion(state, contents, ids);
}
void describe_con(sys::state& state, text::columnar_layout& contents, dcon::pop_id ids) { alice_ui::describe_con(state, contents, ids); }
void describe_mil(sys::state& state, text::columnar_layout& contents, dcon::pop_id ids) { alice_ui::describe_mil(state, contents, ids); }
void describe_lit(sys::state& state, text::columnar_layout& contents, dcon::pop_id ids) { alice_ui::describe_lit(state, contents, ids); }
void describe_growth(sys::state& state, text::columnar_layout& contents, dcon::pop_id ids) { alice_ui::describe_growth(state, contents, ids); }
void describe_assimilation(sys::state& state, text::columnar_layout& contents, dcon::pop_id ids) { alice_ui::describe_assimilation(state, contents, ids); }

std::vector<dcon::pop_id> const& get_pop_window_list(sys::state& state) {
	static const std::vector<dcon::pop_id> empty{};
	if(state.ui_state.population_subwindow)
		return static_cast<population_window*>(state.ui_state.population_subwindow)->country_pop_listbox->row_contents;
	return empty;
}

dcon::pop_id get_pop_details_pop(sys::state& state) {
	dcon::pop_id id{};
	if(state.ui_state.population_subwindow) {
		auto win = static_cast<population_window*>(state.ui_state.population_subwindow)->details_win;
		if(win) {
			Cyto::Any payload = dcon::pop_id{};
			win->impl_get(state, payload);
			id = any_cast<dcon::pop_id>(payload);
		}
	}
	return id;
}

void pop_national_focus_button::button_action(sys::state& state) noexcept {
	if(parent) {
		Cyto::Any payload = dcon::state_instance_id{};
		parent->impl_get(state, payload);

		auto pop_window = static_cast<population_window*>(state.ui_state.population_subwindow);
		pop_window->focus_state = any_cast<dcon::state_instance_id>(payload);
		pop_window->nf_win->set_visible(state, !pop_window->nf_win->is_visible());
		pop_window->nf_win->base_data.position = base_data.position;
		pop_window->move_child_to_front(pop_window->nf_win);
		pop_window->impl_on_update(state);
	}
}

} // namespace ui
