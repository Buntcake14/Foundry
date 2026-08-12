#include "foundry_transport_shadow.hpp"

#include "economy_stats.hpp"
#include "advanced_province_buildings.hpp"
#include "foundry_transport_prototype.hpp"
#include "province.hpp"
#include "province_templates.hpp"
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

bool valid_market(sys::state const& state, dcon::market_id market) {
	return market && size_t(market.index()) < state.world.market_size() && state.world.market_is_valid(market);
}

dcon::civic_building_type_id road_type(sys::state const& state) {
	for(auto type : state.world.in_civic_building_type)
		if(type.get_is_road_network())
			return type.id;
	return {};
}

float market_road_level(sys::state const& state, dcon::market_id market) {
	auto type = road_type(state);
	auto zone = state.world.market_get_zone_from_local_market(market);
	if(!type || !zone || size_t(zone.index()) >= state.world.state_instance_size()) return 0.f;
	float total = 0.f;
	float provinces = 0.f;
	province::for_each_province_in_state_instance(state, zone, [&](dcon::province_id province_id) {
		if(!province_id || !state.world.province_is_valid(province_id)) return;
		total += float(state.world.province_get_civic_building_level(province_id, type.index()));
		provinces += 1.f;
	});
	return provinces > 0.f ? total / provinces : 0.f;
}

float civilian_port_capacity(sys::state const& state, dcon::market_id market) {
	auto zone = state.world.market_get_zone_from_local_market(market);
	if(!zone || size_t(zone.index()) >= state.world.state_instance_size()) return 0.f;
	float capacity = 0.f;
	province::for_each_province_in_state_instance(state, zone, [&](dcon::province_id province_id) {
		if(!province_id || !state.world.province_is_valid(province_id) || !state.world.province_get_port_to(province_id)) return;
		auto port_size = std::max(0.f,
			state.world.province_get_advanced_province_building_max_private_size(
				province_id, advanced_province_buildings::list::civilian_ports));
		auto service = std::clamp(
			state.world.province_get_service_satisfaction(province_id, services::list::port_capacity), 0.f, 1.f);
		// Existing coastal settlements retain modest harbor traffic. Developed
		// civilian ports add substantially more service-backed throughput.
		capacity += 100.f + port_size * std::max(0.1f, service);
	});
	return capacity;
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
		if(!valid_market(state, market)) continue;
		if(std::find(selected.begin(), selected.end(), market) != selected.end()) continue;
		selected.push_back(market);
		state.world.market_for_each_trade_route(market, [&](auto route) {
			for(int endpoint = 0; endpoint < 2; ++endpoint) {
				auto other = state.world.trade_route_get_connected_markets(route, endpoint);
				if(valid_market(state, other) && local_index[size_t(other.index())] < 0) {
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
			if(!route || size_t(route.index()) >= route_added.size() || route_added[size_t(route.index())]) return;
			route_added[route.index()] = true;
			auto first = state.world.trade_route_get_connected_markets(route, 0);
			auto second = state.world.trade_route_get_connected_markets(route, 1);
			if(!valid_market(state, first) || !valid_market(state, second)
				|| local_index[size_t(first.index())] < 0 || local_index[size_t(second.index())] < 0) return;

			auto sea = state.world.trade_route_get_is_sea_route(route);
			auto land = state.world.trade_route_get_is_land_route(route);
			auto mode = sea && (!land || state.world.trade_route_get_sea_distance(route) <= state.world.trade_route_get_land_distance(route))
				? transport_mode::sea : transport_mode::road;
			auto distance = std::max(0.f, state.world.trade_route_get_distance(route));
			auto capacity = std::max(1.f, std::min(
				state.world.market_get_max_throughput(first),
				state.world.market_get_max_throughput(second)
			));
			if(mode == transport_mode::road) {
				auto roads = 0.5f * (market_road_level(state, first) + market_road_level(state, second));
				distance /= 1.f + 0.15f * roads;
				capacity *= 1.f + 0.25f * roads;
			}
			if(mode == transport_mode::sea) {
				capacity = std::min(civilian_port_capacity(state, first), civilian_port_capacity(state, second));
			}
			auto first_owner = state.world.state_instance_get_nation_from_state_ownership(state.world.market_get_zone_from_local_market(first));
			auto second_owner = state.world.state_instance_get_nation_from_state_ownership(state.world.market_get_zone_from_local_market(second));
			float border_cost_first_to_second = 0.f;
			float border_cost_second_to_first = 0.f;
			if(first_owner && second_owner && first_owner != second_owner) {
				border_cost_first_to_second = effective_tariff_import_rate(state, second_owner, second)
					* state.world.market_get_price(first, commodity);
				border_cost_second_to_first = effective_tariff_import_rate(state, first_owner, first)
					* state.world.market_get_price(second, commodity);
			}

			shadow.add_edge({
				size_t(local_index[first.index()]), size_t(local_index[second.index()]), mode,
				distance * 0.01f, capacity, 0.f, 1.f,
				border_cost_first_to_second, border_cost_second_to_first,
				!state.world.trade_route_get_is_trade_forbidden(route) && capacity > 0.f
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
	report << "Infrastructure: roads reduce land cost and raise land capacity; civilian ports limit sea capacity.\n";

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
			<< " cost=" << edge.base_cost
			<< " tariff=" << edge.border_cost_first_to_second << "/" << edge.border_cost_second_to_first
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
