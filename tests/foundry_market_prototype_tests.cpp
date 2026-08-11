#include "economy/foundry_transport_prototype.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

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
	model.add_edge({ 0, 1, transport_mode::road, 1.f, 25.f, 0.f, 0.f, 0.f, true });
	model.add_edge({ 0, 3, transport_mode::river, 1.5f, 30.f, 0.f, 1.f, 0.f, true });
	model.add_edge({ 3, 1, transport_mode::river, 1.5f, 30.f, 0.f, 1.f, 0.f, true });
	model.add_edge({ 1, 2, transport_mode::rail, 0.5f, 100.f, 0.f, 1.f, 0.f, true });
	model.add_edge({ 2, 4, transport_mode::sea, 1.f, 100.f, 0.f, 1.f, 0.f, true });
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
	model.add_edge({ 0, 1, transport_mode::road, 1.f, 25.f, 0.f, 0.f, 0.f, true });
	model.add_edge({ 0, 2, transport_mode::river, 2.f, 100.f, 0.f, 0.f, 0.f, true });
	model.add_edge({ 2, 1, transport_mode::river, 2.f, 100.f, 0.f, 0.f, 0.f, true });
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
	model.add_edge({ 0, 1, transport_mode::rail, 0.5f, 100.f, 0.f, 0.f, 0.f, true });
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

} // namespace

int main() {
	test_local_first();
	test_capacity_and_congestion();
	test_rail_reduces_delivered_cost();
	test_nearby_foreign_supplier_can_win();
	test_closed_border_stops_foreign_trade();
	test_tariff_is_part_of_delivered_price();
	std::cout << "Foundry five-market transport prototype tests passed\n";
	return EXIT_SUCCESS;
}
