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
	// Additive border cost in each direction. The destination/importing
	// market's policy determines the applicable cost.
	float border_tariff_rate_first_to_second = 0.f;
	float border_tariff_rate_second_to_first = 0.f;
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
	size_t path_searches = 0;
};

class simulation {
  public:
	size_t add_market(market_node node) {
		if(node.supply.size() != commodity_count_ || node.demand.size() != commodity_count_ || node.price.size() != commodity_count_) {
			throw std::invalid_argument("market commodity vectors must match the simulation commodity count");
		}
		markets_.push_back(std::move(node));
		adjacency_.emplace_back();
		return markets_.size() - 1;
	}

	size_t add_edge(transport_edge edge) {
		if(edge.first >= markets_.size() || edge.second >= markets_.size() || edge.first == edge.second) {
			throw std::invalid_argument("transport edge has invalid endpoints");
		}
		if(edge.base_cost < 0.f || edge.capacity < 0.f) {
			throw std::invalid_argument("transport edge cost and capacity must be non-negative");
		}
		auto edge_id = edges_.size();
		edges_.push_back(edge);
		adjacency_[edge.first].push_back(edge_id);
		adjacency_[edge.second].push_back(edge_id);
		return edge_id;
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

		std::vector<std::vector<float>> surplus(commodity_count_, std::vector<float>(markets_.size(), 0.f));
		std::vector<std::vector<float>> unmet(commodity_count_, std::vector<float>(markets_.size(), 0.f));
		std::vector<bool> active_commodity(commodity_count_, true);
		std::vector<search_node> search_nodes(markets_.size());
		std::vector<queue_item> search_heap;
		search_heap.reserve(markets_.size() * 2);
		for(size_t commodity = 0; commodity < commodity_count_; ++commodity) {
			// Local transactions always happen first and consume no transport.
			for(size_t market = 0; market < markets_.size(); ++market) {
				auto local = std::min(markets_[market].supply[commodity], markets_[market].demand[commodity]);
				output.local_consumption[market][commodity] = local;
				surplus[commodity][market] = markets_[market].supply[commodity] - local;
				unmet[commodity][market] = markets_[market].demand[commodity] - local;
			}
		}

		// Allocate shared capacity in rounds. Every commodity chooses one best
		// shipment before any commodity can take a second turn. Dividing each
		// path's currently available capacity by the remaining contenders keeps
		// an early commodity from exhausting a contested edge by list order.
		while(true) {
			std::vector<candidate> round(commodity_count_);
			size_t contenders = 0;
			for(size_t commodity = 0; commodity < commodity_count_; ++commodity) {
				if(!active_commodity[commodity]) continue;
				auto& best = round[commodity];
				best = cheapest_shipment(surplus[commodity], unmet[commodity], commodity, trade_policy,
					search_nodes, search_heap);
				++output.path_searches;
				if(best.valid) ++contenders;
				else active_commodity[commodity] = false;
			}
			if(contenders == 0) break;

			std::vector<size_t> edge_contenders(edges_.size(), 0);
			std::vector<float> edge_available(edges_.size(), 0.f);
			for(size_t edge_id = 0; edge_id < edges_.size(); ++edge_id)
				edge_available[edge_id] = std::max(0.f, edges_[edge_id].capacity - edges_[edge_id].used_capacity);
			for(auto const& candidate : round) if(candidate.valid)
				for(auto edge_id : candidate.path.edges) ++edge_contenders[edge_id];

			bool moved_anything = false;
			for(size_t commodity = 0; commodity < commodity_count_; ++commodity) {
				auto const& best = round[commodity];
				if(!best.valid) continue;
				float fair_capacity = std::numeric_limits<float>::infinity();
				for(auto edge_id : best.path.edges) {
					auto divisor = std::max<size_t>(1, edge_contenders[edge_id]);
					fair_capacity = std::min(fair_capacity, edge_available[edge_id] / float(divisor));
				}
				auto quantity = std::min({ surplus[commodity][best.origin], unmet[commodity][best.destination], fair_capacity });
				if(quantity <= epsilon) continue;

				float tariff = 0.f;
				auto path_market = best.origin;
				for(auto edge_id : best.path.edges) {
					auto& edge = edges_[edge_id];
					edge.used_capacity += quantity;
					auto next_market = edge.first == path_market ? edge.second : edge.first;
					if(markets_[edge.first].country != markets_[edge.second].country)
						tariff += trade_policy.border_tariff
							+ directional_border_tariff_rate(edge, path_market, next_market)
								* markets_[path_market].price[commodity];
					path_market = next_market;
				}

				surplus[commodity][best.origin] -= quantity;
				unmet[commodity][best.destination] -= quantity;
				output.shipments.push_back({
					commodity, best.origin, best.destination, quantity,
					markets_[best.origin].price[commodity], best.path.cost - tariff,
					tariff, best.delivered_price, best.path.edges
				});
				moved_anything = true;
			}
			if(!moved_anything) break;
		}

		for(size_t commodity = 0; commodity < commodity_count_; ++commodity) {
			for(size_t market = 0; market < markets_.size(); ++market) {
				output.unmet_demand[market][commodity] = unmet[commodity][market];
				output.unsold_supply[market][commodity] = surplus[commodity][market];
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
	struct search_node {
		float cost = std::numeric_limits<float>::infinity();
		size_t origin = std::numeric_limits<size_t>::max();
		size_t previous = std::numeric_limits<size_t>::max();
		size_t previous_edge = std::numeric_limits<size_t>::max();
	};
	using queue_item = std::pair<float, size_t>;

	candidate cheapest_shipment(std::vector<float> const& surplus, std::vector<float> const& unmet,
			size_t commodity, policy trade_policy, std::vector<search_node>& paths,
			std::vector<queue_item>& pending) const {
		std::fill(paths.begin(), paths.end(), search_node{});
		pending.clear();
		auto push = [&](queue_item item) {
			pending.push_back(item);
			std::push_heap(pending.begin(), pending.end(), std::greater<queue_item>{});
		};
		for(size_t origin = 0; origin < markets_.size(); ++origin) if(surplus[origin] > epsilon) {
			paths[origin].cost = markets_[origin].price[commodity];
			paths[origin].origin = origin;
			push({ paths[origin].cost, origin });
		}
		while(!pending.empty()) {
			std::pop_heap(pending.begin(), pending.end(), std::greater<queue_item>{});
			auto [cost, node] = pending.back(); pending.pop_back();
			if(cost > paths[node].cost + epsilon) continue;
			for(auto edge_id : adjacency_[node]) {
				auto const& edge = edges_[edge_id];
				if(!edge.open || edge.capacity - edge.used_capacity <= epsilon) continue;
				auto next = edge.first == node ? edge.second : edge.first;
				auto next_cost = cost + marginal_edge_cost(edge, node, next, commodity, trade_policy);
				if(next_cost + epsilon < paths[next].cost) {
					paths[next].cost = next_cost;
					paths[next].origin = paths[node].origin;
					paths[next].previous = node;
					paths[next].previous_edge = edge_id;
					push({ next_cost, next });
				}
			}
		}
		candidate best;
		for(size_t destination = 0; destination < markets_.size(); ++destination) {
			auto const& path = paths[destination];
			if(unmet[destination] <= epsilon || path.origin == std::numeric_limits<size_t>::max()
					|| path.origin == destination || !std::isfinite(path.cost)) continue;
			if(!best.valid || path.cost < best.delivered_price - epsilon
					|| (std::abs(path.cost - best.delivered_price) <= epsilon
						&& std::pair{path.origin, destination} < std::pair{best.origin, best.destination})) {
				std::vector<size_t> edges;
				auto cursor = destination;
				while(cursor != path.origin) {
					auto edge_id = paths[cursor].previous_edge;
					auto previous = paths[cursor].previous;
					if(edge_id == std::numeric_limits<size_t>::max()
							|| previous == std::numeric_limits<size_t>::max()) {
						edges.clear();
						break;
					}
					edges.push_back(edge_id);
					cursor = previous;
				}
				std::reverse(edges.begin(), edges.end());
				if(!edges.empty())
					best = { true, path.origin, destination, path.cost,
						{ path.cost - markets_[path.origin].price[commodity], std::move(edges) } };
			}
		}
		return best;
	}

	float directional_border_tariff_rate(transport_edge const& edge, size_t from, size_t to) const {
		if(from == edge.first && to == edge.second) return edge.border_tariff_rate_first_to_second;
		if(from == edge.second && to == edge.first) return edge.border_tariff_rate_second_to_first;
		return 0.f;
	}

	float marginal_edge_cost(transport_edge const& edge, size_t from, size_t to, size_t commodity, policy trade_policy) const {
		auto utilization = edge.capacity > epsilon ? edge.used_capacity / edge.capacity : 1.f;
		auto congestion = edge.base_cost * edge.congestion_strength * utilization * utilization;
		auto tariff = markets_[edge.first].country != markets_[edge.second].country
			? trade_policy.border_tariff + directional_border_tariff_rate(edge, from, to) * markets_[from].price[commodity] : 0.f;
		return edge.base_cost + congestion + tariff;
	}

	std::vector<path_result> cheapest_paths(size_t origin, size_t commodity, policy trade_policy) const {
		std::vector<path_result> paths(markets_.size());
		using queue_item = std::pair<float, size_t>;
		std::priority_queue<queue_item, std::vector<queue_item>, std::greater<>> pending;
		paths[origin].cost = 0.f;
		pending.emplace(0.f, origin);

		while(!pending.empty()) {
			auto [cost, node] = pending.top();
			pending.pop();
			if(cost > paths[node].cost + epsilon) continue;

			for(auto edge_id : adjacency_[node]) {
				auto const& edge = edges_[edge_id];
				if(!edge.open || edge.capacity - edge.used_capacity <= epsilon) continue;
				size_t next;
				if(edge.first == node) next = edge.second;
				else if(edge.second == node) next = edge.first;
				else continue;

				auto next_cost = cost + marginal_edge_cost(edge, node, next, commodity, trade_policy);
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
	std::vector<std::vector<size_t>> adjacency_;
};

} // namespace economy::foundry_transport
