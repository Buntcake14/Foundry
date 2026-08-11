#include "foundry_transport_shadow.hpp"

#include "economy_stats.hpp"
#include "foundry_transport_prototype.hpp"
#include "system_state.hpp"
#include "text.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <queue>
#include <sstream>
#include <vector>

namespace economy::foundry_transport {

namespace {

char const* mode_name(transport_mode mode) {
	switch(mode) {
	case transport_mode::road: return "land";
	case transport_mode::river: return "river";
	case transport_mode::rail: return "rail";
	case transport_mode::sea: return "sea";
	}
	return "unknown";
}

std::string market_name(sys::state const& state, dcon::market_id market) {
	auto zone = state.world.market_get_zone_from_local_market(market);
	auto capital = state.world.state_instance_get_capital(zone);
	if(!capital) return "Market " + std::to_string(market.index());
	return text::produce_simple_string(state, state.world.province_get_name(capital));
}

} // namespace

std::string run_live_shadow(
	sys::state const& state,
	dcon::province_id seed_province,
	dcon::commodity_id commodity,
	size_t maximum_markets
) {
	if(!seed_province || !state.world.province_is_valid(seed_province)) return "MARKET SHADOW: select a land province first.";
	if(!commodity || !state.world.commodity_is_valid(commodity)) return "MARKET SHADOW: invalid commodity.";
	if(maximum_markets < 2) return "MARKET SHADOW: at least two markets are required.";

	auto seed_state = state.world.province_get_state_membership(seed_province);
	if(!seed_state) return "MARKET SHADOW: the selected province has no state market.";
	auto seed_market = state.world.state_instance_get_market_from_local_market(seed_state);
	if(!seed_market) return "MARKET SHADOW: the selected province has no local market.";

	std::vector<int32_t> local_index(state.world.market_size(), -1);
	std::vector<dcon::market_id> selected;
	std::queue<dcon::market_id> pending;
	pending.push(seed_market);
	local_index[seed_market.index()] = 0;

	while(!pending.empty() && selected.size() < maximum_markets) {
		auto market = pending.front();
		pending.pop();
		if(std::find(selected.begin(), selected.end(), market) != selected.end()) continue;
		selected.push_back(market);
		state.world.market_for_each_trade_route(market, [&](auto route) {
			for(int endpoint = 0; endpoint < 2; ++endpoint) {
				auto other = state.world.trade_route_get_connected_markets(route, endpoint);
				if(other && local_index[other.index()] < 0) {
					local_index[other.index()] = int32_t(selected.size());
					pending.push(other);
				}
			}
		});
	}

	std::fill(local_index.begin(), local_index.end(), -1);
	simulation shadow(1);
	for(size_t index = 0; index < selected.size(); ++index) {
		auto market = selected[index];
		local_index[market.index()] = int32_t(index);
		auto zone = state.world.market_get_zone_from_local_market(market);
		auto owner = state.world.state_instance_get_nation_from_state_ownership(zone);
		shadow.add_market({
			market_name(state, market), owner ? owner.index() : -1,
			{ std::max(0.f, state.world.market_get_supply(market, commodity)) },
			{ std::max(0.f, state.world.market_get_demand(market, commodity)) },
			{ std::max(0.f, state.world.market_get_price(market, commodity)) }
		});
	}

	std::vector<bool> route_added(state.world.trade_route_size(), false);
	for(auto market : selected) {
		state.world.market_for_each_trade_route(market, [&](auto route) {
			if(route_added[route.index()]) return;
			route_added[route.index()] = true;
			auto first = state.world.trade_route_get_connected_markets(route, 0);
			auto second = state.world.trade_route_get_connected_markets(route, 1);
			if(!first || !second || local_index[first.index()] < 0 || local_index[second.index()] < 0) return;

			auto sea = state.world.trade_route_get_is_sea_route(route);
			auto land = state.world.trade_route_get_is_land_route(route);
			auto mode = sea && (!land || state.world.trade_route_get_sea_distance(route) <= state.world.trade_route_get_land_distance(route))
				? transport_mode::sea : transport_mode::road;
			auto distance = std::max(0.f, state.world.trade_route_get_distance(route));
			auto capacity = std::max(1.f, std::min(
				state.world.market_get_max_throughput(first),
				state.world.market_get_max_throughput(second)
			));
			auto first_owner = state.world.state_instance_get_nation_from_state_ownership(state.world.market_get_zone_from_local_market(first));
			auto second_owner = state.world.state_instance_get_nation_from_state_ownership(state.world.market_get_zone_from_local_market(second));
			float border_cost = 0.f;
			if(first_owner && second_owner && first_owner != second_owner) {
				auto tariff_rate = 0.5f * (
					effective_tariff_import_rate(state, first_owner, first)
					+ effective_tariff_import_rate(state, second_owner, second)
				);
				border_cost = tariff_rate * 0.5f * (
					state.world.market_get_price(first, commodity)
					+ state.world.market_get_price(second, commodity)
				);
			}

			shadow.add_edge({
				size_t(local_index[first.index()]), size_t(local_index[second.index()]), mode,
				distance * 0.01f, capacity, 0.f, 1.f, border_cost,
				!state.world.trade_route_get_is_trade_forbidden(route)
			});
		});
	}

	auto started = std::chrono::steady_clock::now();
	auto result = shadow.clear();
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - started);

	std::ostringstream report;
	report << std::fixed << std::setprecision(2);
	report << "MARKET SHADOW (read-only)\n";
	report << "Good: " << text::produce_simple_string(state, state.world.commodity_get_name(commodity))
		<< " | markets: " << selected.size() << " | routes: " << shadow.edges().size()
		<< " | runtime: " << elapsed.count() << " us\n";
	report << "Provisional conversion: transport cost = live effective distance x 0.01.\n";

	for(size_t index = 0; index < selected.size(); ++index) {
		auto const& node = shadow.markets()[index];
		report << "  [" << index << "] " << node.name
			<< " price=" << node.price[0]
			<< " supply=" << node.supply[0]
			<< " demand=" << node.demand[0]
			<< " local=" << result.local_consumption[index][0]
			<< " unmet=" << result.unmet_demand[index][0]
			<< " unsold=" << result.unsold_supply[index][0] << "\n";
	}

	for(size_t index = 0; index < shadow.edges().size(); ++index) {
		auto const& edge = shadow.edges()[index];
		report << "  route " << index << " " << mode_name(edge.mode)
			<< " " << shadow.markets()[edge.first].name << " <-> " << shadow.markets()[edge.second].name
			<< " cost=" << edge.base_cost << " tariff=" << edge.border_cost
			<< " used=" << edge.used_capacity << "/" << edge.capacity
			<< (edge.open ? "" : " CLOSED") << "\n";
	}

	for(auto const& shipment : result.shipments) {
		report << "  ship " << shipment.quantity << " "
			<< shadow.markets()[shipment.origin].name << " -> " << shadow.markets()[shipment.destination].name
			<< " origin=" << shipment.origin_price << " transport=" << shipment.transport_cost
			<< " tariff=" << shipment.tariff_cost << " delivered=" << shipment.delivered_price << " path=";
		for(size_t index = 0; index < shipment.edges.size(); ++index) {
			if(index) report << ",";
			report << shipment.edges[index];
		}
		report << "\n";
	}

	return report.str();
}

} // namespace economy::foundry_transport
