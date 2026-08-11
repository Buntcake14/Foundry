#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <queue>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace economy::foundry_transport {

enum class transport_mode { road, river, rail, sea };

struct market_node {
	std::string name;
	int32_t country = 0;
	std::vector<float> supply;
	std::vector<float> demand;
	std::vector<float> price;
};

struct transport_edge {
	size_t first = 0;
	size_t second = 0;
	transport_mode mode = transport_mode::road;
	float base_cost = 0.f;
	float capacity = 0.f;
	float used_capacity = 0.f;
	float congestion_strength = 1.f;
	float border_cost = 0.f;
	bool open = true;
};

struct policy {
	// Additive prototype tariff paid per unit when a route crosses a border.
	// The live implementation will use the engine's percentage tariffs.
	float border_tariff = 0.f;
};

struct shipment {
	size_t commodity = 0;
	size_t origin = 0;
	size_t destination = 0;
	float quantity = 0.f;
	float origin_price = 0.f;
	float transport_cost = 0.f;
	float tariff_cost = 0.f;
	float delivered_price = 0.f;
	std::vector<size_t> edges;
};

struct result {
	std::vector<shipment> shipments;
	std::vector<std::vector<float>> local_consumption;
	std::vector<std::vector<float>> unmet_demand;
	std::vector<std::vector<float>> unsold_supply;
};

class simulation {
  public:
	size_t add_market(market_node node) {
		if(node.supply.size() != commodity_count_ || node.demand.size() != commodity_count_ || node.price.size() != commodity_count_) {
			throw std::invalid_argument("market commodity vectors must match the simulation commodity count");
		}
		markets_.push_back(std::move(node));
		return markets_.size() - 1;
	}

	size_t add_edge(transport_edge edge) {
		if(edge.first >= markets_.size() || edge.second >= markets_.size() || edge.first == edge.second) {
			throw std::invalid_argument("transport edge has invalid endpoints");
		}
		if(edge.base_cost < 0.f || edge.capacity < 0.f) {
			throw std::invalid_argument("transport edge cost and capacity must be non-negative");
		}
		edges_.push_back(edge);
		return edges_.size() - 1;
	}

	explicit simulation(size_t commodity_count) : commodity_count_(commodity_count) { }

	std::vector<market_node> const& markets() const { return markets_; }
	std::vector<transport_edge> const& edges() const { return edges_; }
	std::vector<transport_edge>& edges() { return edges_; }

	result clear(policy trade_policy = {}) {
		result output;
		output.local_consumption.assign(markets_.size(), std::vector<float>(commodity_count_, 0.f));
		output.unmet_demand.assign(markets_.size(), std::vector<float>(commodity_count_, 0.f));
		output.unsold_supply.assign(markets_.size(), std::vector<float>(commodity_count_, 0.f));

		for(auto& edge : edges_) edge.used_capacity = 0.f;

		for(size_t commodity = 0; commodity < commodity_count_; ++commodity) {
			std::vector<float> surplus(markets_.size(), 0.f);
			std::vector<float> unmet(markets_.size(), 0.f);

			// Local transactions always happen first and consume no transport.
			for(size_t market = 0; market < markets_.size(); ++market) {
				auto local = std::min(markets_[market].supply[commodity], markets_[market].demand[commodity]);
				output.local_consumption[market][commodity] = local;
				surplus[market] = markets_[market].supply[commodity] - local;
				unmet[market] = markets_[market].demand[commodity] - local;
			}

			while(true) {
				candidate best;
				for(size_t origin = 0; origin < markets_.size(); ++origin) {
					if(surplus[origin] <= epsilon) continue;
					auto paths = cheapest_paths(origin, trade_policy);
					for(size_t destination = 0; destination < markets_.size(); ++destination) {
						if(origin == destination || unmet[destination] <= epsilon || !std::isfinite(paths[destination].cost)) continue;
						auto delivered = markets_[origin].price[commodity] + paths[destination].cost;
						if(!best.valid || delivered < best.delivered_price - epsilon ||
							(std::abs(delivered - best.delivered_price) <= epsilon && std::pair{origin, destination} < std::pair{best.origin, best.destination})) {
							best = { true, origin, destination, delivered, paths[destination] };
						}
					}
				}

				if(!best.valid) break;
				auto available_capacity = path_capacity(best.path.edges);
				auto quantity = std::min({ surplus[best.origin], unmet[best.destination], available_capacity });
				if(quantity <= epsilon) break;

				float tariff = 0.f;
				for(auto edge_id : best.path.edges) {
					auto& edge = edges_[edge_id];
					edge.used_capacity += quantity;
					if(markets_[edge.first].country != markets_[edge.second].country) tariff += trade_policy.border_tariff + edge.border_cost;
				}

				surplus[best.origin] -= quantity;
				unmet[best.destination] -= quantity;
				output.shipments.push_back({
					commodity, best.origin, best.destination, quantity,
					markets_[best.origin].price[commodity], best.path.cost - tariff,
					tariff, best.delivered_price, best.path.edges
				});
			}

			for(size_t market = 0; market < markets_.size(); ++market) {
				output.unmet_demand[market][commodity] = unmet[market];
				output.unsold_supply[market][commodity] = surplus[market];
			}
		}

		return output;
	}

  private:
	static constexpr float epsilon = 0.00001f;

	struct path_result {
		float cost = std::numeric_limits<float>::infinity();
		std::vector<size_t> edges;
	};

	struct candidate {
		bool valid = false;
		size_t origin = 0;
		size_t destination = 0;
		float delivered_price = 0.f;
		path_result path;
	};

	float marginal_edge_cost(transport_edge const& edge, policy trade_policy) const {
		auto utilization = edge.capacity > epsilon ? edge.used_capacity / edge.capacity : 1.f;
		auto congestion = edge.base_cost * edge.congestion_strength * utilization * utilization;
		auto tariff = markets_[edge.first].country != markets_[edge.second].country ? trade_policy.border_tariff + edge.border_cost : 0.f;
		return edge.base_cost + congestion + tariff;
	}

	std::vector<path_result> cheapest_paths(size_t origin, policy trade_policy) const {
		std::vector<path_result> paths(markets_.size());
		using queue_item = std::pair<float, size_t>;
		std::priority_queue<queue_item, std::vector<queue_item>, std::greater<>> pending;
		paths[origin].cost = 0.f;
		pending.emplace(0.f, origin);

		while(!pending.empty()) {
			auto [cost, node] = pending.top();
			pending.pop();
			if(cost > paths[node].cost + epsilon) continue;

			for(size_t edge_id = 0; edge_id < edges_.size(); ++edge_id) {
				auto const& edge = edges_[edge_id];
				if(!edge.open || edge.capacity - edge.used_capacity <= epsilon) continue;
				size_t next;
				if(edge.first == node) next = edge.second;
				else if(edge.second == node) next = edge.first;
				else continue;

				auto next_cost = cost + marginal_edge_cost(edge, trade_policy);
				if(next_cost + epsilon < paths[next].cost) {
					paths[next].cost = next_cost;
					paths[next].edges = paths[node].edges;
					paths[next].edges.push_back(edge_id);
					pending.emplace(next_cost, next);
				}
			}
		}
		return paths;
	}

	float path_capacity(std::vector<size_t> const& path) const {
		float available = std::numeric_limits<float>::infinity();
		for(auto edge_id : path) {
			auto const& edge = edges_[edge_id];
			available = std::min(available, edge.capacity - edge.used_capacity);
		}
		return path.empty() ? 0.f : std::max(0.f, available);
	}

	size_t commodity_count_ = 0;
	std::vector<market_node> markets_;
	std::vector<transport_edge> edges_;
};

} // namespace economy::foundry_transport
