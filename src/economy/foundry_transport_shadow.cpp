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
#include <numeric>
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
	size_t maximum_markets,
	live_shadow_summary* summary
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
	struct route_infrastructure_debug {
		float first_road_level = 0.f;
		float second_road_level = 0.f;
		float base_cost = 0.f;
		float modified_cost = 0.f;
		float base_capacity = 0.f;
		float modified_capacity = 0.f;
	};
	std::vector<route_infrastructure_debug> route_debug;
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
			route_infrastructure_debug debug;
			debug.base_cost = distance * 0.01f;
			debug.base_capacity = capacity;
			if(mode == transport_mode::road) {
				debug.first_road_level = market_road_level(state, first);
				debug.second_road_level = market_road_level(state, second);
				auto roads = 0.5f * (debug.first_road_level + debug.second_road_level);
				distance /= 1.f + 0.15f * roads;
				capacity *= 1.f + 0.25f * roads;
			}
			if(mode == transport_mode::sea) {
				capacity = std::min(civilian_port_capacity(state, first), civilian_port_capacity(state, second));
			}
			debug.modified_cost = distance * 0.01f;
			debug.modified_capacity = capacity;
			auto first_owner = state.world.state_instance_get_nation_from_state_ownership(state.world.market_get_zone_from_local_market(first));
			auto second_owner = state.world.state_instance_get_nation_from_state_ownership(state.world.market_get_zone_from_local_market(second));
			float border_rate_first_to_second = 0.f;
			float border_rate_second_to_first = 0.f;
			if(first_owner && second_owner && first_owner != second_owner) {
				border_rate_first_to_second = effective_tariff_import_rate(state, second_owner, second);
				border_rate_second_to_first = effective_tariff_import_rate(state, first_owner, first);
			}

			shadow.add_edge({
				size_t(local_index[first.index()]), size_t(local_index[second.index()]), mode,
				distance * 0.01f, capacity, 0.f, 1.f,
				border_rate_first_to_second, border_rate_second_to_first,
				!state.world.trade_route_get_is_trade_forbidden(route) && capacity > 0.f
			});
			route_debug.push_back(debug);
		});
	}

	auto started = std::chrono::steady_clock::now();
	auto result = shadow.clear();
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - started);
	auto compact = maximum_markets > 5;
	auto market_lines = compact ? std::min<size_t>(selected.size(), 8) : selected.size();
	auto route_lines = compact ? std::min<size_t>(shadow.edges().size(), 10) : shadow.edges().size();
	auto shipment_lines = compact ? std::min<size_t>(result.shipments.size(), 10) : result.shipments.size();
	float total_supply = 0.f;
	float total_demand = 0.f;
	float total_local = 0.f;
	float total_unmet = 0.f;
	float total_unsold = 0.f;
	float total_shipped = 0.f;
	for(size_t index = 0; index < selected.size(); ++index) {
		total_supply += shadow.markets()[index].supply[0];
		total_demand += shadow.markets()[index].demand[0];
		total_local += result.local_consumption[index][0];
		total_unmet += result.unmet_demand[index][0];
		total_unsold += result.unsold_supply[index][0];
	}
	for(auto const& shipment : result.shipments) total_shipped += shipment.quantity;
	if(summary) {
		summary->markets = selected;
		summary->supply = total_supply;
		summary->demand = total_demand;
		summary->consumption = total_demand - total_unmet;
		summary->unmet = total_unmet;
		summary->unsold = total_unsold;
	}

	std::ostringstream report;
	report << std::fixed << std::setprecision(2);
	report << "MARKET SHADOW (read-only)\n";
	report << "Good: " << text::produce_simple_string(state, state.world.commodity_get_name(commodity))
		<< " | state markets: " << selected.size() << " | routes: " << shadow.edges().size()
		<< " | runtime: " << elapsed.count() << " us\n";
	report << "Infrastructure: roads reduce land cost and raise land capacity; civilian ports limit sea capacity.\n";
	report << "Totals: supply=" << total_supply << " demand=" << total_demand
		<< " local=" << total_local << " shipped=" << total_shipped
		<< " unmet=" << total_unmet << " unsold=" << total_unsold << "\n";

	for(size_t index = 0; index < market_lines; ++index) {
		auto const& node = shadow.markets()[index];
		report << "  [" << index << "] " << node.name
			<< " price=" << node.price[0]
			<< " supply=" << node.supply[0]
			<< " demand=" << node.demand[0]
			<< " local=" << result.local_consumption[index][0]
			<< " unmet=" << result.unmet_demand[index][0]
			<< " unsold=" << result.unsold_supply[index][0] << "\n";
	}
	if(market_lines < selected.size()) report << "  ... " << selected.size() - market_lines << " additional markets omitted\n";

	for(size_t index = 0; index < route_lines; ++index) {
		auto const& edge = shadow.edges()[index];
		auto const& debug = route_debug[index];
		report << "  route " << index << " " << mode_name(edge.mode)
			<< " " << shadow.markets()[edge.first].name << " <-> " << shadow.markets()[edge.second].name
			<< " cost=" << std::setprecision(4) << edge.base_cost
			<< " tariff-rate=" << edge.border_tariff_rate_first_to_second << "/" << edge.border_tariff_rate_second_to_first
			<< " used=" << std::setprecision(2) << edge.used_capacity << "/" << edge.capacity;
		if(edge.mode == transport_mode::road) {
			auto cost_reduction = debug.base_cost > 0.f
				? 100.f * (1.f - debug.modified_cost / debug.base_cost) : 0.f;
			auto capacity_increase = debug.base_capacity > 0.f
				? 100.f * (debug.modified_capacity / debug.base_capacity - 1.f) : 0.f;
			report << " roads=" << debug.first_road_level << "/" << debug.second_road_level
				<< " base-cost=" << std::setprecision(4) << debug.base_cost
				<< " cost-down=" << std::setprecision(1) << cost_reduction << "%"
				<< " base-cap=" << std::setprecision(2) << debug.base_capacity
				<< " cap-up=" << std::setprecision(1) << capacity_increase << "%";
		}
		report << std::setprecision(2)
			<< (edge.open ? "" : " CLOSED") << "\n";
	}
	if(route_lines < shadow.edges().size()) report << "  ... " << shadow.edges().size() - route_lines << " additional routes omitted\n";

	for(size_t shipment_index = 0; shipment_index < shipment_lines; ++shipment_index) {
		auto const& shipment = result.shipments[shipment_index];
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
	if(shipment_lines < result.shipments.size()) report << "  ... " << result.shipments.size() - shipment_lines << " additional shipments omitted\n";

	return report.str();
}

std::string run_live_shadow_batch(sys::state const& state, dcon::province_id seed_province,
		size_t maximum_markets, size_t maximum_commodities, live_basket_summary* summary) {
	if(!seed_province || !state.world.province_is_valid(seed_province)) return "MARKET BATCH: select a land province first.";
	auto seed_state = state.world.province_get_state_membership(seed_province);
	auto seed_market = seed_state ? state.world.state_instance_get_market_from_local_market(seed_state) : dcon::market_id{};
	if(!seed_market) return "MARKET BATCH: selected province has no local market.";

	std::vector<int32_t> local_index(state.world.market_size(), -1);
	std::vector<dcon::market_id> selected;
	std::queue<dcon::market_id> pending;
	pending.push(seed_market);
	local_index[size_t(seed_market.index())] = 0;
	while(!pending.empty() && selected.size() < maximum_markets) {
		auto market = pending.front(); pending.pop();
		if(!valid_market(state, market) || std::find(selected.begin(), selected.end(), market) != selected.end()) continue;
		selected.push_back(market);
		state.world.market_for_each_trade_route(market, [&](auto route) {
			for(int endpoint = 0; endpoint < 2; ++endpoint) {
				auto other = state.world.trade_route_get_connected_markets(route, endpoint);
				if(valid_market(state, other) && local_index[size_t(other.index())] < 0) {
					local_index[size_t(other.index())] = int32_t(selected.size()); pending.push(other);
				}
			}
		});
	}
	std::vector<dcon::commodity_id> commodities;
	auto active_in_sample = [&](dcon::commodity_id commodity) {
		for(auto market : selected)
			if(state.world.market_get_supply(market, commodity) > 0.00001f
					|| state.world.market_get_demand(market, commodity) > 0.00001f) return true;
		return false;
	};
	// A bounded audit deliberately samples across the four trade categories.
	// An effectively unbounded audit includes every active tradable good.
	if(maximum_commodities >= state.world.commodity_size()) {
		state.world.for_each_commodity([&](dcon::commodity_id commodity) {
			if(commodity && !state.world.commodity_get_money_rgo(commodity)
					&& !state.world.commodity_get_is_local(commodity) && active_in_sample(commodity))
				commodities.push_back(commodity);
		});
	} else {
		for(auto wanted : { sys::commodity_group::raw_material_goods, sys::commodity_group::industrial_goods,
				sys::commodity_group::consumer_goods, sys::commodity_group::military_goods }) {
			size_t category_count = 0;
			state.world.for_each_commodity([&](dcon::commodity_id commodity) {
				if(!commodity || commodities.size() >= maximum_commodities || category_count >= 3
						|| state.world.commodity_get_money_rgo(commodity) || state.world.commodity_get_is_local(commodity)
						|| !active_in_sample(commodity)) return;
				auto group = sys::commodity_group(state.world.commodity_get_commodity_group(commodity));
				if(group == wanted || (wanted == sys::commodity_group::industrial_goods
						&& group == sys::commodity_group::industrial_and_consumer_goods)) {
					commodities.push_back(commodity); ++category_count;
				}
			});
		}
	}
	if(commodities.empty()) return "MARKET BATCH: no active commodities available.";
	std::fill(local_index.begin(), local_index.end(), -1);
	simulation shadow(commodities.size());
	for(size_t index = 0; index < selected.size(); ++index) {
		auto market = selected[index]; local_index[size_t(market.index())] = int32_t(index);
		auto zone = state.world.market_get_zone_from_local_market(market);
		auto owner = state.world.state_instance_get_nation_from_state_ownership(zone);
		market_node node;
		node.name = market_name(state, market); node.country = owner ? owner.index() : -1;
		for(auto commodity : commodities) {
			node.supply.push_back(std::max(0.f, state.world.market_get_supply(market, commodity)));
			node.demand.push_back(std::max(0.f, state.world.market_get_demand(market, commodity)));
			node.price.push_back(std::max(0.f, state.world.market_get_price(market, commodity)));
		}
		shadow.add_market(std::move(node));
	}
	std::vector<bool> route_added(state.world.trade_route_size(), false);
	for(auto market : selected) state.world.market_for_each_trade_route(market, [&](auto route) {
		if(!route || size_t(route.index()) >= route_added.size() || route_added[size_t(route.index())]) return;
		route_added[size_t(route.index())] = true;
		auto first = state.world.trade_route_get_connected_markets(route, 0);
		auto second = state.world.trade_route_get_connected_markets(route, 1);
		if(!valid_market(state, first) || !valid_market(state, second)
				|| local_index[size_t(first.index())] < 0 || local_index[size_t(second.index())] < 0) return;
		auto sea = state.world.trade_route_get_is_sea_route(route);
		auto land = state.world.trade_route_get_is_land_route(route);
		auto mode = sea && (!land || state.world.trade_route_get_sea_distance(route) <= state.world.trade_route_get_land_distance(route))
			? transport_mode::sea : transport_mode::road;
		auto distance = std::max(0.f, state.world.trade_route_get_distance(route));
		auto capacity = std::max(1.f, std::min(state.world.market_get_max_throughput(first), state.world.market_get_max_throughput(second)));
		if(mode == transport_mode::road) {
			auto roads = .5f * (market_road_level(state, first) + market_road_level(state, second));
			distance /= 1.f + .15f * roads; capacity *= 1.f + .25f * roads;
		} else capacity = std::min(civilian_port_capacity(state, first), civilian_port_capacity(state, second));
		auto first_owner = state.world.state_instance_get_nation_from_state_ownership(state.world.market_get_zone_from_local_market(first));
		auto second_owner = state.world.state_instance_get_nation_from_state_ownership(state.world.market_get_zone_from_local_market(second));
		float forward = 0.f, reverse = 0.f;
		if(first_owner && second_owner && first_owner != second_owner) {
			forward = effective_tariff_import_rate(state, second_owner, second);
			reverse = effective_tariff_import_rate(state, first_owner, first);
		}
		shadow.add_edge({ size_t(local_index[size_t(first.index())]), size_t(local_index[size_t(second.index())]),
			mode, distance * .01f, capacity, 0.f, 1.f, forward, reverse,
			!state.world.trade_route_get_is_trade_forbidden(route) && capacity > 0.f });
	});

	auto started = std::chrono::steady_clock::now();
	auto result = shadow.clear();
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - started);
	float local = 0.f, shipped = 0.f, unmet = 0.f, unsold = 0.f;
	for(auto const& row : result.local_consumption) for(auto value : row) local += value;
	for(auto const& row : result.unmet_demand) for(auto value : row) unmet += value;
	for(auto const& row : result.unsold_supply) for(auto value : row) unsold += value;
	for(auto const& shipment : result.shipments) shipped += shipment.quantity;
	if(summary) {
		summary->runtime_us = elapsed.count();
		summary->path_searches = result.path_searches;
		summary->markets = selected;
		summary->commodities = commodities;
		summary->supply.assign(commodities.size(), 0.f);
		summary->demand.assign(commodities.size(), 0.f);
		summary->consumption.assign(commodities.size(), 0.f);
		summary->unmet.assign(commodities.size(), 0.f);
		summary->unsold.assign(commodities.size(), 0.f);
		for(size_t market = 0; market < selected.size(); ++market) {
			for(size_t good = 0; good < commodities.size(); ++good) {
				summary->supply[good] += shadow.markets()[market].supply[good];
				summary->demand[good] += shadow.markets()[market].demand[good];
				summary->unmet[good] += result.unmet_demand[market][good];
				summary->unsold[good] += result.unsold_supply[market][good];
			}
		}
		for(size_t good = 0; good < commodities.size(); ++good)
			summary->consumption[good] = summary->demand[good] - summary->unmet[good];
	}
	std::ostringstream report;
	report << std::fixed << std::setprecision(2)
		<< "MARKET SHADOW BATCH (read-only)\nstate-markets=" << selected.size()
		<< " routes=" << shadow.edges().size() << " goods=" << commodities.size()
		<< " runtime=" << elapsed.count() << " us\nlocal=" << local << " shipped=" << shipped
		<< " unmet=" << unmet << " unsold=" << unsold << " shipments=" << result.shipments.size()
		<< "\npath-searches=" << result.path_searches << "\n";
	return report.str();
}

bool run_live_audit(sys::state& state, dcon::commodity_id commodity) {
	if(!commodity || !state.world.commodity_is_valid(commodity) || !state.local_player_nation) return false;
	auto seed = state.world.nation_get_capital(state.local_player_nation);
	if(!seed || !state.world.province_is_valid(seed)) return false;
	// The read-only regional run exercises the same live supply, demand, price,
	// infrastructure, tariff, capacity, and routing inputs used by the console
	// benchmark. Any explicit diagnostic error disables the audit immediately.
	live_shadow_summary summary;
	auto report = run_live_shadow(state, seed, commodity, 25, &summary);
	state.cheat_data.foundry_market_live_markets = std::move(summary.markets);
	state.cheat_data.foundry_market_baseline_supply = summary.supply;
	state.cheat_data.foundry_market_baseline_demand = summary.demand;
	state.cheat_data.foundry_market_routed_consumption = summary.consumption;
	state.cheat_data.foundry_market_routed_unmet = summary.unmet;
	state.cheat_data.foundry_market_routed_unsold = summary.unsold;
	return report.rfind("MARKET SHADOW (read-only)", 0) == 0;
}

bool run_live_audit_basket(sys::state& state, size_t maximum_commodities, size_t maximum_markets) {
	if(!state.local_player_nation) return false;
	auto seed = state.world.nation_get_capital(state.local_player_nation);
	if(!seed || !state.world.province_is_valid(seed)) return false;
	live_basket_summary summary;
	auto report = run_live_shadow_batch(state, seed, maximum_markets, maximum_commodities, &summary);
	if(report.rfind("MARKET SHADOW BATCH (read-only)", 0) != 0 || summary.commodities.empty()) return false;
	state.cheat_data.foundry_market_live_markets = std::move(summary.markets);
	state.cheat_data.foundry_market_basket_commodities = std::move(summary.commodities);
	state.cheat_data.foundry_market_basket_supply = std::move(summary.supply);
	state.cheat_data.foundry_market_basket_demand = std::move(summary.demand);
	state.cheat_data.foundry_market_basket_consumption = std::move(summary.consumption);
	state.cheat_data.foundry_market_basket_unmet = std::move(summary.unmet);
	state.cheat_data.foundry_market_basket_unsold = std::move(summary.unsold);
	state.cheat_data.foundry_market_basket_latest_runtime_us = summary.runtime_us;
	state.cheat_data.foundry_market_basket_latest_path_searches = summary.path_searches;
	state.cheat_data.foundry_market_basket_runtime_sum_us += uint64_t(std::max<int64_t>(0, summary.runtime_us));
	return true;
}

bool run_live_access_audit(sys::state& state, size_t maximum_commodities, size_t maximum_markets) {
	if(!state.local_player_nation) return false;
	auto seed_province = state.world.nation_get_capital(state.local_player_nation);
	if(!seed_province || !state.world.province_is_valid(seed_province)) return false;
	auto seed_state = state.world.province_get_state_membership(seed_province);
	auto seed_market = seed_state ? state.world.state_instance_get_market_from_local_market(seed_state) : dcon::market_id{};
	if(!seed_market) return false;

	// Select reachable state markets once. Unlike the exact validation solver,
	// the access model never searches this graph while allocating commodities.
	std::vector<int32_t> visited(state.world.market_size(), -1);
	std::vector<dcon::market_id> markets;
	std::queue<dcon::market_id> pending;
	pending.push(seed_market);
	visited[size_t(seed_market.index())] = 0;
	while(!pending.empty() && markets.size() < maximum_markets) {
		auto market = pending.front(); pending.pop();
		if(!valid_market(state, market) || std::find(markets.begin(), markets.end(), market) != markets.end()) continue;
		markets.push_back(market);
		state.world.market_for_each_trade_route(market, [&](auto route) {
			for(int endpoint = 0; endpoint < 2; ++endpoint) {
				auto other = state.world.trade_route_get_connected_markets(route, endpoint);
				if(valid_market(state, other) && visited[size_t(other.index())] < 0) {
					visited[size_t(other.index())] = int32_t(markets.size());
					pending.push(other);
				}
			}
		});
	}

	std::vector<dcon::commodity_id> commodities;
	state.world.for_each_commodity([&](dcon::commodity_id commodity) {
		if(!commodity || commodities.size() >= maximum_commodities
				|| state.world.commodity_get_money_rgo(commodity) || state.world.commodity_get_is_local(commodity)) return;
		for(auto market : markets) {
			if(state.world.market_get_supply(market, commodity) > 0.00001f
					|| state.world.market_get_demand(market, commodity) > 0.00001f) {
				commodities.push_back(commodity);
				break;
			}
		}
	});
	if(markets.empty() || commodities.empty()) return false;

	auto const market_count = markets.size();
	auto const good_count = commodities.size();
	std::vector<dcon::nation_id> owners(market_count);
	std::vector<float> access(market_count, 0.35f);
	for(size_t m = 0; m < market_count; ++m) {
		auto zone = state.world.market_get_zone_from_local_market(markets[m]);
		owners[m] = state.world.state_instance_get_nation_from_state_ownership(zone);
		auto roads = market_road_level(state, markets[m]);
		auto ports = civilian_port_capacity(state, markets[m]);
		// A baseline keeps remote states connected; infrastructure controls how
		// effectively they compete for the global remainder.
		access[m] = std::clamp(0.35f + 0.12f * roads + (ports > 0.f ? 0.30f : 0.f), 0.10f, 1.f);
		if(owners[m]) access[m] /= 1.f + std::max(0.f, effective_tariff_import_rate(state, owners[m], markets[m]));
	}

	std::vector<std::vector<float>> surplus(good_count, std::vector<float>(market_count));
	std::vector<std::vector<float>> unmet(good_count, std::vector<float>(market_count));
	std::vector<float> supply(good_count), demand(good_count), consumption(good_count);
	float local_total = 0.f, domestic_total = 0.f, global_total = 0.f;
	auto started = std::chrono::steady_clock::now();
	for(size_t g = 0; g < good_count; ++g) {
		for(size_t m = 0; m < market_count; ++m) {
			auto s = std::max(0.f, state.world.market_get_supply(markets[m], commodities[g]));
			auto d = std::max(0.f, state.world.market_get_demand(markets[m], commodities[g]));
			auto local = std::min(s, d);
			supply[g] += s; demand[g] += d; consumption[g] += local;
			local_total += local;
			surplus[g][m] = s - local; unmet[g][m] = d - local;
		}

		// Tier two: all states belonging to a country share their remaining
		// domestic supply proportionally. No national-rank ordering is involved.
		std::vector<float> nation_supply(state.world.nation_size(), 0.f);
		std::vector<float> nation_demand(state.world.nation_size(), 0.f);
		for(size_t m = 0; m < market_count; ++m) if(owners[m]) {
			nation_supply[size_t(owners[m].index())] += surplus[g][m];
			nation_demand[size_t(owners[m].index())] += unmet[g][m];
		}
		for(size_t m = 0; m < market_count; ++m) if(owners[m] && unmet[g][m] > 0.f) {
			auto n = size_t(owners[m].index());
			auto available = std::min(nation_supply[n], nation_demand[n]);
			auto received = nation_demand[n] > 0.f ? available * unmet[g][m] / nation_demand[n] : 0.f;
			unmet[g][m] -= received; consumption[g] += received; domestic_total += received;
		}
		for(size_t n = 0; n < nation_supply.size(); ++n)
			nation_supply[n] = std::max(0.f, nation_supply[n] - std::min(nation_supply[n], nation_demand[n]));

		// Tier three: exportable national surplus enters one fair pool. States
		// receive it proportionally to remaining demand times market access.
		float global_supply = 0.f, weighted_demand = 0.f;
		for(auto value : nation_supply) global_supply += value;
		for(size_t m = 0; m < market_count; ++m) weighted_demand += unmet[g][m] * access[m];
		if(global_supply > 0.f && weighted_demand > 0.f) {
			for(size_t m = 0; m < market_count; ++m) {
				auto received = std::min(unmet[g][m], global_supply * unmet[g][m] * access[m] / weighted_demand);
				unmet[g][m] -= received; consumption[g] += received; global_total += received;
			}
		}
	}
	auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - started);

	state.cheat_data.foundry_market_live_markets = std::move(markets);
	state.cheat_data.foundry_market_basket_commodities = std::move(commodities);
	state.cheat_data.foundry_market_basket_supply = std::move(supply);
	state.cheat_data.foundry_market_basket_demand = std::move(demand);
	state.cheat_data.foundry_market_basket_consumption = std::move(consumption);
	state.cheat_data.foundry_market_basket_unmet.assign(good_count, 0.f);
	state.cheat_data.foundry_market_basket_unsold.assign(good_count, 0.f);
	for(size_t g = 0; g < good_count; ++g) {
		for(auto value : unmet[g]) state.cheat_data.foundry_market_basket_unmet[g] += value;
		state.cheat_data.foundry_market_basket_unsold[g] = std::max(0.f,
			state.cheat_data.foundry_market_basket_supply[g] - state.cheat_data.foundry_market_basket_consumption[g]);
	}
	state.cheat_data.foundry_market_basket_latest_runtime_us = elapsed.count();
	state.cheat_data.foundry_market_basket_latest_path_searches = 0;
	state.cheat_data.foundry_market_access_local_consumption = local_total;
	state.cheat_data.foundry_market_access_domestic_consumption = domestic_total;
	state.cheat_data.foundry_market_access_global_consumption = global_total;
	state.cheat_data.foundry_market_access_min = *std::min_element(access.begin(), access.end());
	state.cheat_data.foundry_market_access_max = *std::max_element(access.begin(), access.end());
	state.cheat_data.foundry_market_access_mean = std::accumulate(access.begin(), access.end(), 0.f) / float(access.size());
	state.cheat_data.foundry_market_basket_runtime_sum_us += uint64_t(std::max<int64_t>(0, elapsed.count()));
	return true;
}

} // namespace economy::foundry_transport
