#pragma once

#include <cstddef>
#include <string>

#include "dcon_generated.hpp"

namespace sys { struct state; }

namespace economy::foundry_transport {

// Copies a small connected neighborhood of live state markets into the
// prototype and returns diagnostics. This function is strictly read-only with
// respect to the supplied game state.
std::string run_live_shadow(
	sys::state const& state,
	dcon::province_id seed_province,
	dcon::commodity_id commodity,
	size_t maximum_markets = 5
);

} // namespace economy::foundry_transport
