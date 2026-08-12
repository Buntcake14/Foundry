#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "dcon_generated.hpp"

namespace sys { struct state; }

namespace economy::foundry_transport {

struct live_shadow_summary {
	std::vector<dcon::market_id> markets;
	float supply = 0.f;
	float demand = 0.f;
	float consumption = 0.f;
	float unmet = 0.f;
	float unsold = 0.f;
};

struct live_basket_summary {
	std::vector<dcon::market_id> markets;
	std::vector<dcon::commodity_id> commodities;
	std::vector<float> supply;
	std::vector<float> demand;
	std::vector<float> consumption;
	std::vector<float> unmet;
	std::vector<float> unsold;
	int64_t runtime_us = 0;
	size_t path_searches = 0;
};

// Copies a connected neighborhood of live state markets into the
// prototype and returns diagnostics. This function is strictly read-only with
// respect to the supplied game state.
std::string run_live_shadow(
	sys::state const& state,
	dcon::province_id seed_province,
	dcon::commodity_id commodity,
	size_t maximum_markets = 5,
	live_shadow_summary* summary = nullptr
);

std::string run_live_shadow_batch(
	sys::state const& state,
	dcon::province_id seed_province,
	size_t maximum_markets = 25,
	size_t maximum_commodities = 12,
	live_basket_summary* summary = nullptr
);

// Runs the selected commodity through the routed shadow during a normal daily
// economy update. Returns false only when the live inputs cannot be validated.
bool run_live_audit(sys::state& state, dcon::commodity_id commodity);
bool run_live_audit_basket(sys::state& state, size_t maximum_commodities = 12,
	size_t maximum_markets = 25);

} // namespace economy::foundry_transport
