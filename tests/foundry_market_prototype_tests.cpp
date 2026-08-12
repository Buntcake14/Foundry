#include "economy/foundry_transport_prototype.hpp"

#include <cmath>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using namespace economy::foundry_transport;

void require(bool condition, std::string_view message) {
	if(!condition) {
		std::cerr << "FAILED: " << message << '\n';
		std::exit(EXIT_FAILURE);
	}
}

bool near(float first, float second) {
	return std::abs(first - second) < 0.001f;
}

simulation five_market_model(float industrial_local_supply = 10.f, float foreign_price = 20.f) {
	simulation model(1);
	model.add_market({ "Coal State", 0, { 100.f }, { 0.f }, { 4.f } });
	model.add_market({ "Industrial State", 0, { industrial_local_supply }, { 80.f }, { 9.f } });
	model.add_market({ "Port State", 0, { 0.f }, { 10.f }, { 10.f } });
	model.add_market({ "Farm State", 0, { 20.f }, { 20.f }, { 8.f } });
	model.add_market({ "Foreign Port State", 1, { 100.f }, { 20.f }, { foreign_price } });
	model.add_edge({ 0, 1, transport_mode::road, 1.f, 25.f, 0.f, 0.f, 0.f, 0.f, true });
	model.add_edge({ 0, 3, transport_mode::river, 1.5f, 30.f, 0.f, 1.f, 0.f, 0.f, true });
	model.add_edge({ 3, 1, transport_mode::river, 1.5f, 30.f, 0.f, 1.f, 0.f, 0.f, true });
	model.add_edge({ 1, 2, transport_mode::rail, 0.5f, 100.f, 0.f, 1.f, 0.f, 0.f, true });
	model.add_edge({ 2, 4, transport_mode::sea, 1.f, 100.f, 0.f, 1.f, 0.f, 0.f, true });
	return model;
}

void test_local_first() {
	auto model = five_market_model(10.f);
	auto result = model.clear();
	require(near(result.local_consumption[1][0], 10.f), "local supply must satisfy local demand before imports");
}

void test_capacity_and_congestion() {
	simulation model(1);
	model.add_market({ "Origin", 0, { 100.f }, { 0.f }, { 4.f } });
	model.add_market({ "Destination", 0, { 0.f }, { 80.f }, { 10.f } });
	model.add_market({ "River Junction", 0, { 0.f }, { 0.f }, { 10.f } });
	model.add_edge({ 0, 1, transport_mode::road, 1.f, 25.f, 0.f, 0.f, 0.f, 0.f, true });
	model.add_edge({ 0, 2, transport_mode::river, 2.f, 100.f, 0.f, 0.f, 0.f, 0.f, true });
	model.add_edge({ 2, 1, transport_mode::river, 2.f, 100.f, 0.f, 0.f, 0.f, 0.f, true });
	auto result = model.clear();
	require(near(model.edges()[0].used_capacity, 25.f), "road should stop at its shared capacity");
	bool used_river_path = false;
	for(auto const& shipment : result.shipments) {
		if(shipment.origin == 0 && shipment.destination == 1 && shipment.edges.size() == 2) used_river_path = true;
	}
	require(used_river_path, "traffic should use the alternate river path after the direct road fills");
}

void test_rail_reduces_delivered_cost() {
	simulation model(1);
	model.add_market({ "Coal State", 0, { 20.f }, { 0.f }, { 4.f } });
	model.add_market({ "Rail Port", 0, { 0.f }, { 10.f }, { 10.f } });
	model.add_edge({ 0, 1, transport_mode::rail, 0.5f, 100.f, 0.f, 0.f, 0.f, 0.f, true });
	auto result = model.clear();
	float coal_to_port = 1000.f;
	for(auto const& shipment : result.shipments) {
		if(shipment.origin == 0 && shipment.destination == 1) coal_to_port = std::min(coal_to_port, shipment.delivered_price);
	}
	require(near(coal_to_port, 4.5f), "rail should add its low transport cost to the origin price");
}

void test_nearby_foreign_supplier_can_win() {
	auto model = five_market_model(0.f, 3.f);
	model.edges()[0].base_cost = 12.f;
	auto result = model.clear({ 1.f });
	bool foreign_supplied_industry = false;
	for(auto const& shipment : result.shipments) {
		if(shipment.origin == 4 && shipment.destination == 1) foreign_supplied_industry = true;
	}
	require(foreign_supplied_industry, "cheap nearby foreign supply should beat costly domestic delivery");
}

void test_closed_border_stops_foreign_trade() {
	auto model = five_market_model(0.f);
	model.edges()[4].open = false;
	auto result = model.clear();
	for(auto const& shipment : result.shipments) {
		require(shipment.origin != 4 && shipment.destination != 4, "closed sea border must stop foreign shipments");
	}
}

void test_tariff_is_part_of_delivered_price() {
	auto without_tariff = five_market_model();
	auto free_result = without_tariff.clear({ 0.f });
	auto with_tariff = five_market_model();
	auto tariff_result = with_tariff.clear({ 2.f });
	float free_price = 1000.f;
	float tariff_price = 1000.f;
	for(auto const& shipment : free_result.shipments) if(shipment.origin == 4) free_price = std::min(free_price, shipment.delivered_price);
	for(auto const& shipment : tariff_result.shipments) if(shipment.origin == 4) tariff_price = std::min(tariff_price, shipment.delivered_price);
	require(tariff_price >= free_price + 1.999f, "border tariff must increase foreign delivered price");
}

void test_import_tariff_is_directional() {
	simulation outward(1);
	outward.add_market({ "Exporter", 0, { 10.f }, { 0.f }, { 4.f } });
	outward.add_market({ "High Tariff Importer", 1, { 0.f }, { 10.f }, { 4.f } });
	outward.add_edge({ 0, 1, transport_mode::road, 1.f, 100.f, 0.f, 0.f, 1.25f, 0.25f, true });
	auto outward_result = outward.clear();
	require(near(outward_result.shipments.front().tariff_cost, 5.f),
		"first-to-second shipment must use the second market's import tariff");

	simulation inward(1);
	inward.add_market({ "High Tariff Importer", 1, { 0.f }, { 10.f }, { 4.f } });
	inward.add_market({ "Exporter", 0, { 10.f }, { 0.f }, { 4.f } });
	inward.add_edge({ 0, 1, transport_mode::road, 1.f, 100.f, 0.f, 0.f, 1.25f, 0.25f, true });
	auto inward_result = inward.clear();
	require(near(inward_result.shipments.front().tariff_cost, 1.f),
		"second-to-first shipment must use the first market's import tariff");
}

void test_cheapest_multi_hop_supplier_wins() {
	simulation model(1);
	model.add_market({ "Expensive Neighbor", 0, { 20.f }, { 0.f }, { 10.f } });
	model.add_market({ "Consumer", 0, { 0.f }, { 10.f }, { 10.f } });
	model.add_market({ "Cheap Distant Producer", 0, { 20.f }, { 0.f }, { 2.f } });
	model.add_market({ "Road Junction", 0, { 0.f }, { 0.f }, { 10.f } });
	model.add_edge({ 0, 1, transport_mode::road, 1.f, 100.f, 0.f, 0.f, 0.f, 0.f, true });
	model.add_edge({ 2, 3, transport_mode::road, 2.f, 100.f, 0.f, 0.f, 0.f, 0.f, true });
	model.add_edge({ 3, 1, transport_mode::road, 2.f, 100.f, 0.f, 0.f, 0.f, 0.f, true });
	auto result = model.clear();
	require(result.shipments.size() == 1, "one supplier should satisfy the consumer");
	require(result.shipments.front().origin == 2 && result.shipments.front().edges.size() == 2,
		"a cheaper delivered price should beat proximity alone and use the multi-hop path");
	require(near(result.shipments.front().delivered_price, 6.f),
		"multi-hop delivered price must include every transport edge");
}

void test_route_capacity_is_shared_across_goods() {
	simulation model(2);
	model.add_market({ "Producer", 0, { 10.f, 10.f }, { 0.f, 0.f }, { 2.f, 2.f } });
	model.add_market({ "Consumer", 0, { 0.f, 0.f }, { 10.f, 10.f }, { 5.f, 5.f } });
	model.add_edge({ 0, 1, transport_mode::road, 1.f, 12.f, 0.f, 0.f, 0.f, 0.f, true });
	auto result = model.clear();
	float shipped = 0.f;
	for(auto const& shipment : result.shipments) shipped += shipment.quantity;
	require(near(shipped, 12.f), "all goods together must respect shared route capacity");
	require(near(model.edges().front().used_capacity, 12.f), "route utilization must equal shared shipments");
	require(near(result.unmet_demand[1][0] + result.unmet_demand[1][1], 8.f),
		"demand beyond shared transport capacity must remain unmet");
}

void test_contested_capacity_is_fair_across_goods() {
	simulation model(2);
	model.add_market({ "Producer", 0, { 100.f, 100.f }, { 0.f, 0.f }, { 2.f, 2.f } });
	model.add_market({ "Consumer", 0, { 0.f, 0.f }, { 100.f, 100.f }, { 5.f, 5.f } });
	model.add_edge({ 0, 1, transport_mode::road, 1.f, 10.f, 0.f, 0.f, 0.f, 0.f, true });
	auto result = model.clear();
	float first_good = 0.f;
	float second_good = 0.f;
	for(auto const& shipment : result.shipments) {
		if(shipment.commodity == 0) first_good += shipment.quantity;
		if(shipment.commodity == 1) second_good += shipment.quantity;
	}
	require(near(first_good, 5.f) && near(second_good, 5.f),
		"equally constrained goods must split a shared route fairly regardless of list order");
}

void test_regional_network_performance() {
	constexpr size_t market_count = 25;
	simulation model(1);
	for(size_t index = 0; index < market_count; ++index) {
		auto supply = index % 4 == 0 ? 20.f : 0.f;
		auto demand = index % 4 != 0 ? 4.f : 0.f;
		model.add_market({ "Regional Market", int32_t(index / 5), { supply }, { demand }, { 2.f + float(index % 7) } });
	}
	for(size_t first = 0; first < market_count; ++first) {
		for(size_t second = first + 1; second < market_count; ++second) {
			if((first + second) % 4 == 0 || second == first + 1)
				model.add_edge({ first, second, transport_mode::road, 0.5f + float((first + second) % 5),
					100.f, 0.f, 0.f, 0.f, 0.f, true });
		}
	}
	auto started = std::chrono::steady_clock::now();
	auto result = model.clear();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - started);
	require(!result.shipments.empty(), "regional benchmark must produce shipments");
	require(elapsed.count() < 1000, "25-market regional routing should remain comfortably sub-second");
}

simulation accounting_model() {
	simulation model(3);
	model.add_market({ "Mine", 0, { 30.f, 5.f, 0.f }, { 2.f, 8.f, 4.f }, { 2.f, 6.f, 7.f } });
	model.add_market({ "City", 0, { 1.f, 18.f, 3.f }, { 20.f, 2.f, 14.f }, { 5.f, 3.f, 6.f } });
	model.add_market({ "Foreign Port", 1, { 10.f, 0.f, 20.f }, { 8.f, 9.f, 1.f }, { 4.f, 7.f, 2.f } });
	model.add_edge({ 0, 1, transport_mode::road, 1.f, 14.f, 0.f, 1.f, 0.f, 0.f, true });
	model.add_edge({ 1, 2, transport_mode::sea, 0.5f, 18.f, 0.f, 1.f, .1f, .2f, true });
	model.add_edge({ 0, 2, transport_mode::road, 3.f, 7.f, 0.f, 1.f, .1f, .2f, true });
	return model;
}

void test_goods_accounting_and_route_capacity() {
	auto model = accounting_model();
	auto original = model.markets();
	auto result = model.clear();
	for(size_t commodity = 0; commodity < 3; ++commodity) {
		for(size_t market = 0; market < original.size(); ++market) {
			float exported = 0.f;
			float imported = 0.f;
			for(auto const& shipment : result.shipments) if(shipment.commodity == commodity) {
				if(shipment.origin == market) exported += shipment.quantity;
				if(shipment.destination == market) imported += shipment.quantity;
			}
			require(near(original[market].supply[commodity],
				result.local_consumption[market][commodity] + exported + result.unsold_supply[market][commodity]),
				"every unit supplied must be consumed locally, exported, or remain unsold");
			require(near(original[market].demand[commodity],
				result.local_consumption[market][commodity] + imported + result.unmet_demand[market][commodity]),
				"every unit demanded must be consumed locally, imported, or remain unmet");
		}
	}
	for(size_t edge_id = 0; edge_id < model.edges().size(); ++edge_id) {
		float shipment_use = 0.f;
		for(auto const& shipment : result.shipments)
			for(auto used_edge : shipment.edges) if(used_edge == edge_id) shipment_use += shipment.quantity;
		require(near(shipment_use, model.edges()[edge_id].used_capacity),
			"recorded route utilization must equal shipments crossing that route");
		require(model.edges()[edge_id].used_capacity <= model.edges()[edge_id].capacity + 0.001f,
			"route utilization must never exceed capacity");
	}
}

void test_identical_inputs_are_deterministic() {
	auto first_model = accounting_model();
	auto second_model = accounting_model();
	auto first = first_model.clear();
	auto second = second_model.clear();
	require(first.shipments.size() == second.shipments.size(), "identical runs must produce the same shipment count");
	for(size_t index = 0; index < first.shipments.size(); ++index) {
		auto const& a = first.shipments[index];
		auto const& b = second.shipments[index];
		require(a.commodity == b.commodity && a.origin == b.origin && a.destination == b.destination
			&& near(a.quantity, b.quantity) && near(a.delivered_price, b.delivered_price) && a.edges == b.edges,
			"identical runs must produce identical ordered shipments");
	}
	require(first.local_consumption == second.local_consumption
		&& first.unmet_demand == second.unmet_demand && first.unsold_supply == second.unsold_supply,
		"identical runs must produce identical accounting totals");
}

} // namespace

int main() {
	test_local_first();
	test_capacity_and_congestion();
	test_rail_reduces_delivered_cost();
	test_nearby_foreign_supplier_can_win();
	test_closed_border_stops_foreign_trade();
	test_tariff_is_part_of_delivered_price();
	test_import_tariff_is_directional();
	test_cheapest_multi_hop_supplier_wins();
	test_route_capacity_is_shared_across_goods();
	test_contested_capacity_is_fair_across_goods();
	test_regional_network_performance();
	test_goods_accounting_and_route_capacity();
	test_identical_inputs_are_deterministic();
	std::cout << "Foundry five-market transport prototype tests passed\n";
	return EXIT_SUCCESS;
}
