#pragma once

#include <array>
#include <cstring>
#include <string>
#include <unordered_map>
#include "container_types.hpp"
#include "commands.hpp"
#include <iostream>
#include <chrono>
#include <ctime>
#include "system_state.hpp"
#include "network.hpp"
#include "parsers.hpp"
#include "simple_fs.hpp"
#include "network.hpp"
#include "demographics.hpp"
#include "province.hpp"

#include <text.hpp>
#include "json.hpp"

#define CPPHTTPLIB_NO_EXCEPTIONS
#include "httplib.h"

#include "jsonlayer.hpp"
#include "jsonlayer.cpp"
#include "websocket.hpp"
#include "color.hpp"
#include "color_templates.hpp"
#include "modes/political.hpp"
#include "modes/admin.hpp"
#include "modes/diplomatic.hpp"
#include "modes/region.hpp"
#include "modes/population.hpp"
#include "modes/nationality.hpp"
#include "modes/sphere.hpp"
#include "modes/supply.hpp"
#include "modes/party_loyalty.hpp"
#include "modes/rank.hpp"
#include "modes/migration.hpp"
#include "modes/civilization_level.hpp"
#include "modes/infrastructure.hpp"
#include "modes/revolt.hpp"
#include "modes/colonial.hpp"
#include "modes/recruitment.hpp"
#include "modes/national_focus.hpp"
#include "modes/rgo_output.hpp"
#include "stb_image_write.h"

using json = nlohmann::json;

namespace webui {

// HTTP
static httplib::Server svr;

// Wraps a route handler with a shared_lock on state.webui_state_lock, so every route is
// safe-by-construction against the host thread's per-tick/per-command mutation (see
// webui_state_lock's declaration in system_state.hpp and the exclusive lock taken around
// process_server_outgoing_queue in network.cpp). New routes should go through this rather
// than reading state directly.
template <typename Fn>
inline auto guarded(sys::state& state, Fn&& fn) {
	return [&state, fn = std::forward<Fn>(fn)](const httplib::Request& req, httplib::Response& res) {
		std::shared_lock<std::shared_mutex> read_lock(state.webui_state_lock);
		fn(req, res);
	};
}

struct province_id_map_offsets_t {
	uint32_t raw_height;
	uint32_t top_free_space;
};

// The legacy .bmp map-loading path (display_data::load_province_data,
// map_data_loading.cpp) pads province_id_map with extra "water" rows above the
// real content and scales size_y to raw_image_height * 1.3 -- confirmed live:
// size_y=2808 vs the served image's real 2160px height. /map/provinces.png and
// /map/province_ids.png both stream the raw (unpadded) image dimensions, so
// pixel coordinates from either must be shifted down by this same offset before
// indexing into province_id_map, or the lookup silently reads the wrong rows
// entirely (not just an off-by-a-little error). Shared by every route that
// walks province_id_map so they can't drift out of sync with each other.
template <typename MapData>
inline province_id_map_offsets_t province_id_map_offsets(MapData const& map_data) {
	uint32_t raw_height = uint32_t(float(map_data.size_y) / 1.3f);
	uint32_t free_space = map_data.size_y > raw_height ? (map_data.size_y - raw_height) : 0;
	uint32_t top_free_space = (free_space * 3) / 5;
	return province_id_map_offsets_t{ raw_height, top_free_space };
}

// Shared by every /map/province_colors mode: every X_map_from(state) function returns
// the same texture_size*2 layout (primary color + stripe color, indexed by
// province::to_map_id), so extracting just the primary half into a flat [r,g,b, ...]
// JSON array is identical regardless of which mode produced the array.
inline std::string extract_primary_colors_json(sys::state& state, std::vector<uint32_t> const& prov_color) {
	auto province_count = state.world.province_size();
	std::string out;
	out.reserve(size_t(province_count) * 12);
	out.push_back('[');
	for(int32_t i = 0; i < int32_t(province_count); ++i) {
		auto map_id = province::to_map_id(dcon::province_id(dcon::province_id::value_base_t(i)));
		uint32_t packed = prov_color[map_id];
		if(i != 0)
			out.push_back(',');
		out += std::to_string(sys::int_red_from_int(packed));
		out.push_back(',');
		out += std::to_string(sys::int_green_from_int(packed));
		out.push_back(',');
		out += std::to_string(sys::int_blue_from_int(packed));
	}
	out.push_back(']');
	return out;
}

inline void init(sys::state& state) noexcept {

	if(state.host_settings.alice_expose_webui != 1 || state.network_mode == sys::network_mode_type::client) {
		return;
	}
	/* Conventions:
	/{collection}/(\d+) for one entry retrieval,
	/{collection}/all to retrieve all entries of type
	*/

	svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
		res.set_redirect("/app/");
	});

	svr.set_mount_point("/app", "./web");

	svr.Get("/date", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		json j = format_date(state, state.current_date);
		res.set_content(j.dump(), "application/json");
	}));


	svr.Get("/nation/all", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		json jlist = json::array();

		for(auto nation : state.world.in_nation) {
				json j = format_nation(state, nation);

				jlist.push_back(j);
			}

		res.set_content(jlist.dump(), "application/json");

	}));

	svr.Get(R"(/nation/(\d+))", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		auto match = req.matches[1];
		auto nationnum = std::atoi(match.str().c_str());

		dcon::nation_id n{ dcon::nation_id::value_base_t(nationnum) };

		auto j = format_nation(state, n);

		res.set_content(j.dump(), "application/json");
	}));

	svr.Get(R"(/factory/(\d+))", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		auto match = req.matches[1];
		auto facnum = std::atoi(match.str().c_str());
		dcon::factory_id f{ dcon::factory_id::value_base_t(facnum) };

		auto j = format_factory(state, f);

		res.set_content(j.dump(), "application/json");
	}));

	svr.Get("/commodity/all", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		json jlist = json::array();

		for(auto commodity : state.world.in_commodity) {
			jlist.push_back(format_commodity(state, commodity));
		}

		res.set_content(jlist.dump(), "application/json");
	}));

	svr.Get("/routes", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		json jlist = json::array();

		for(auto cid : state.world.in_commodity) {
			state.world.for_each_trade_route([&](dcon::trade_route_id trade_route) {
				auto current_volume = state.world.trade_route_get_volume(trade_route, cid);
				auto origin =
					current_volume > 0.f
					? state.world.trade_route_get_connected_markets(trade_route, 0)
					: state.world.trade_route_get_connected_markets(trade_route, 1);
				auto target =
					current_volume <= 0.f
					? state.world.trade_route_get_connected_markets(trade_route, 0)
					: state.world.trade_route_get_connected_markets(trade_route, 1);

				auto s_origin = state.world.market_get_zone_from_local_market(origin);
				auto s_target = state.world.market_get_zone_from_local_market(target);

				auto p_origin = state.world.state_instance_get_capital(s_origin);
				auto p_target = state.world.state_instance_get_capital(s_target);

				auto sat = state.world.market_get_actual_probability_to_buy(origin, cid);

				auto absolute_volume = std::abs(current_volume);
				auto factual_volume = sat * absolute_volume;

				if(absolute_volume <= 0) {
					return;
				}

				bool is_sea = state.world.trade_route_get_distance(trade_route) == state.world.trade_route_get_sea_distance(trade_route);

				auto commodity_name = text::produce_simple_string(state, state.world.commodity_get_name(cid));

				json j = json::object();

				j["commodity_id"] = cid.id.value;
				j["commodity"] = commodity_name;

				j["origin_market_id"] = origin.value;
				j["target_market_id"] = target.value;

				j["origin_state_id"] = s_origin.value;
				j["target_state_id"] = s_target.value;

				j["origin_province_id"] = p_origin.id.value;
				j["target_province_id"] = p_target.id.value;

				j["origin_province_name"] = text::produce_simple_string(state, state.world.province_get_name(p_origin));
				j["target_province_name"] = text::produce_simple_string(state, state.world.province_get_name(p_target));

				auto origin_country = state.world.province_get_nation_from_province_ownership(p_origin);
				auto target_country = state.world.province_get_nation_from_province_ownership(p_target);

				j["origin_country_id"] = origin_country.value;
				j["target_country_id"] = target_country.value;

				j["origin_country_name"] = text::produce_simple_string(state, text::get_name(state, origin_country));
				j["target_country_name"] = text::produce_simple_string(state, text::get_name(state, target_country));

				j["volume"] = text::format_float(factual_volume);
				j["actual_volume"] = text::format_float(absolute_volume);

				j["is_sea"] = is_sea;
				jlist.push_back(j);
			});
		}

		res.set_content(jlist.dump(), "application/json");
	}));

	svr.Get(R"(/province/(\d+))", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		auto match = req.matches[1];
		auto provnum = std::atoi(match.str().c_str());

		dcon::province_id p{ dcon::province_id::value_base_t(provnum) };

		auto j = format_province(state, p);

		res.set_content(j.dump(), "application/json");
	}));

	svr.Get("/province/all", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		json jlist = json::array();

		for(auto prov : state.world.in_province) {
			auto j = format_province(state, prov);
			jlist.push_back(j);
		}

		res.set_content(jlist.dump(), "application/json");
	}));

	svr.Get(R"(/state/(\d+))", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		auto match = req.matches[1];
		auto statenum = std::atoi(match.str().c_str());

		dcon::state_instance_id s{ dcon::state_instance_id::value_base_t(statenum) };

		auto j = format_state(state, s);

		res.set_content(j.dump(), "application/json");
	}));

	svr.Get("/wars", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		json jlist = json::array();

		for(auto war : state.world.in_war) {
			auto id = war.id.index();

			json j = json::object();

			j["id"] = id;
			j["name"] = text::produce_simple_string(state, war.get_name());
			j["is_great"] = war.get_is_great();
			j["is_crisis"] = war.get_is_crisis_war();
			j["attacker_battle_score"] = war.get_attacker_battle_score();
			j["defender_battle_score"] = war.get_defender_battle_score();
			j["primary_attacker"] = format_nation(state, war.get_primary_attacker());
			j["primary_defender"] = format_nation(state, war.get_primary_defender());

			j["over_state"] = text::produce_simple_string(state, war.get_over_state().get_name());

			json jalist = json::array();
			json jdlist = json::array();
			std::vector<dcon::nation_id> attackers;

			for(auto wp : state.world.war_get_war_participant(war)) {
				if(wp.get_is_attacker()) { 
					jalist.push_back(format_nation(state, wp.get_nation()));
					attackers.push_back(wp.get_nation());
				} else {
					jdlist.push_back(format_nation(state, wp.get_nation()));
				}
			}

			j["attackers"] = jalist;
			j["defenders"] = jdlist;

			json jawgslist = json::array();
			json jdwgslist = json::array();

			for(auto el : war.get_wargoals_attached()) {
				auto wg = el.get_wargoal();
				if(std::find(attackers.begin(), attackers.end(), wg.get_added_by()) != attackers.end()) {
					jawgslist.push_back(format_wargoal(state, wg));
				}
				else {
					jdwgslist.push_back(format_wargoal(state, wg));
				}
			}

			j["attacker_wargoals"] = jawgslist;
			j["defender_wargoals"] = jdwgslist;

			jlist.push_back(j);
		}

		res.set_content(jlist.dump(), "application/json");
	}));

	svr.Get("/crisis", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		json j = json::object();

		j["attacker"] = format_nation(state, state.crisis_attacker);
		j["defender"] = format_nation(state, state.crisis_defender);

		j["primary_attacker"] = format_nation(state, state.primary_crisis_attacker);
		j["primary_defender"] = format_nation(state, state.primary_crisis_defender);

		if(state.crisis_state_instance) {
			auto fid = dcon::fatten(state.world, state.crisis_state_instance);
			auto defid = fid.get_definition();
			j["over_state"] = text::produce_simple_string(state, defid.get_name());
		}

		j["temperature"] = state.crisis_temperature;

		json jalist = json::array();
		json jdlist = json::array();

		for(auto cp : state.crisis_participants) {
			if(cp.supports_attacker) {
				jalist.push_back(format_nation(state, cp.id));
			} else if (!cp.merely_interested) {
				jdlist.push_back(format_nation(state, cp.id));
			}
		}

		j["attackers"] = jalist;
		j["defenders"] = jdlist;

		json jawgslist = json::array();
		json jdwgslist = json::array();

		for(auto awg : state.crisis_attacker_wargoals) {
			jawgslist.push_back(format_wargoal(state, awg));
		}
		for(auto dwg : state.crisis_attacker_wargoals) {
			jdwgslist.push_back(format_wargoal(state, dwg));
		}

		j["attacker_wargoals"] = jawgslist;
		j["defender_wargoals"] = jdwgslist;

		res.set_content(j.dump(), "application/json");
	}));

	svr.Get("/unittype/all", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		json jlist = json::array();

		for(uint32_t i = 2; i < state.military_definitions.unit_base_definitions.size(); ++i) {
			dcon::unit_type_id uid = dcon::unit_type_id{ dcon::unit_type_id::value_base_t(i) };
			auto j = json::object();

			jlist.push_back(format_unit_type(state, uid));
		}

		res.set_content(jlist.dump(), "application/json");
	}));

	svr.Get(R"(/army/all)", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {

		auto j = json::array();

		for(auto a : state.world.in_army) {
			j.push_back(format_army(state, a));
		}

		res.set_content(j.dump(), "application/json");
	}));

	svr.Get(R"(/navy/all)", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {

		auto j = json::array();

		for(auto n : state.world.in_navy) {
			j.push_back(format_navy(state, n));
		}

		res.set_content(j.dump(), "application/json");
	}));

	svr.Get(R"(/regiment/(\d+))", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		auto match = req.matches[1];
		auto num = std::atoi(match.str().c_str());

		dcon::regiment_id p{ dcon::regiment_id::value_base_t(num) };

		auto j = format_regiment(state, p);

		res.set_content(j.dump(), "application/json");
	}));

	svr.Get(R"(/ship/(\d+))", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		auto match = req.matches[1];
		auto num = std::atoi(match.str().c_str());

		dcon::ship_id p{ dcon::ship_id::value_base_t(num) };

		auto j = format_ship(state, p);

		res.set_content(j.dump(), "application/json");
	}));

	svr.Get("/map/provinces.png", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		auto root = simple_fs::get_root(state.common_fs);
		auto map_dir = simple_fs::open_directory(root, NATIVE("map"));

		// Mirrors display_data::load_map_data's own fallback chain (map_data_loading.cpp) so the
		// browser always sees exactly the file the engine itself loaded, mod overlays included --
		// deliberately not a blind mount of the map/ directory, which would bypass simple_fs's
		// virtual mod-overlay filesystem.
		if(auto f = simple_fs::open_file(map_dir, NATIVE("alice_provinces.png")); f) {
			auto content = simple_fs::view_contents(*f);
			res.set_content(reinterpret_cast<const char*>(content.data), size_t(content.file_size), "image/png");
			return;
		}
		if(auto f = simple_fs::open_file(map_dir, NATIVE("provinces.png")); f) {
			auto content = simple_fs::view_contents(*f);
			res.set_content(reinterpret_cast<const char*>(content.data), size_t(content.file_size), "image/png");
			return;
		}
		if(auto f = simple_fs::open_file(map_dir, NATIVE("provinces.bmp")); f) {
			auto content = simple_fs::view_contents(*f);
			res.set_content(reinterpret_cast<const char*>(content.data), size_t(content.file_size), "image/bmp");
			return;
		}
		res.status = 404;
	}));

	svr.Get("/map/province_at", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		json j = json::object();
		if(!req.has_param("x") || !req.has_param("y")) {
			res.status = 400;
			return;
		}
		auto x = std::atoi(req.get_param_value("x").c_str());
		auto y = std::atoi(req.get_param_value("y").c_str());
		auto& map_data = state.map_state.map_data;
		auto offsets = province_id_map_offsets(map_data);

		if(x < 0 || y < 0 || uint32_t(x) >= map_data.size_x || uint32_t(y) >= offsets.raw_height) {
			res.status = 400;
			return;
		}
		uint32_t index = (offsets.top_free_space + uint32_t(y)) * map_data.size_x + uint32_t(x);
		auto map_id = map_data.province_id_map[index];
		auto prov = province::from_map_id(map_id);
		j["province_id"] = prov.index();
		res.set_content(j.dump(), "application/json");
	}));

	svr.Get("/map/meta", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		auto& map_data = state.map_state.map_data;
		auto offsets = province_id_map_offsets(map_data);
		json j = json::object();
		j["size_x"] = map_data.size_x;
		j["raw_height"] = offsets.raw_height;
		// The *full padded* height and the crop offset within it -- needed only for
		// correctly sampling colormap.dds/colormap_water.dds, which are separate
		// equirectangular-world-space assets at a different scale/offset than the
		// province bitmap (map_f.glsl's get_corrected_coords works in this padded
		// space, not the cropped space every other /map/* route uses).
		j["size_y"] = map_data.size_y;
		j["top_free_space"] = offsets.top_free_space;
		j["province_count"] = state.world.province_size();
		// 1-based, same space as the raw values /map/province_ids.png encodes -- the
		// frontend never needs to know about province::from_map_id's 0=invalid convention.
		j["first_sea_province_map_id"] = province::to_map_id(state.province_definitions.first_sea_province);
		res.set_content(j.dump(), "application/json");
	}));

	svr.Get("/map/province_ids.png", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		// Encodes the engine's raw province_id_map (map_id space: 0 = no province,
		// same convention province::from_map_id already uses) directly into R/G, so
		// the frontend never needs a separate encode/decode step -- R = map_id >> 8,
		// G = map_id & 0xFF. Computed once and cached: the map layout is fixed for
		// the life of the process, there's nothing to invalidate.
		static std::vector<unsigned char> cached_png;
		if(cached_png.empty()) {
			auto& map_data = state.map_state.map_data;
			auto offsets = province_id_map_offsets(map_data);

			std::vector<unsigned char> pixels(size_t(map_data.size_x) * offsets.raw_height * 3);
			for(uint32_t y = 0; y < offsets.raw_height; ++y) {
				for(uint32_t x = 0; x < map_data.size_x; ++x) {
					uint32_t index = (offsets.top_free_space + y) * map_data.size_x + x;
					uint16_t map_id = map_data.province_id_map[index];
					size_t out = (size_t(y) * map_data.size_x + x) * 3;
					pixels[out + 0] = (unsigned char)((map_id >> 8) & 0xFF);
					pixels[out + 1] = (unsigned char)(map_id & 0xFF);
					pixels[out + 2] = 0;
				}
			}

			stbi_write_png_to_func(
				[](void* context, void* data, int size) {
					auto* out = reinterpret_cast<std::vector<unsigned char>*>(context);
					auto* bytes = reinterpret_cast<unsigned char*>(data);
					out->insert(out->end(), bytes, bytes + size);
				},
				&cached_png, int(map_data.size_x), int(offsets.raw_height), 3, pixels.data(), 0);
		}
		res.set_content(reinterpret_cast<const char*>(cached_png.data()), cached_png.size(), "image/png");
	}));

	svr.Get("/map/terrain_ids.png", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		// Same shape as /map/province_ids.png, but for terrain_id_map (map_f.glsl's
		// terrain-sheet index, 0-63, 255 = no data) instead of province_id_map --
		// same size_x*size_y padded array on the same display_data, same crop math,
		// so it reuses province_id_map_offsets unchanged. Only the R channel is used
		// (one byte is enough for a 0-255 index); cached the same way.
		static std::vector<unsigned char> cached_png;
		if(cached_png.empty()) {
			auto& map_data = state.map_state.map_data;
			auto offsets = province_id_map_offsets(map_data);

			std::vector<unsigned char> pixels(size_t(map_data.size_x) * offsets.raw_height * 3);
			for(uint32_t y = 0; y < offsets.raw_height; ++y) {
				for(uint32_t x = 0; x < map_data.size_x; ++x) {
					uint32_t index = (offsets.top_free_space + y) * map_data.size_x + x;
					uint8_t terrain_id = map_data.terrain_id_map[index];
					size_t out = (size_t(y) * map_data.size_x + x) * 3;
					pixels[out + 0] = terrain_id;
					pixels[out + 1] = 0;
					pixels[out + 2] = 0;
				}
			}

			stbi_write_png_to_func(
				[](void* context, void* data, int size) {
					auto* out = reinterpret_cast<std::vector<unsigned char>*>(context);
					auto* bytes = reinterpret_cast<unsigned char*>(data);
					out->insert(out->end(), bytes, bytes + size);
				},
				&cached_png, int(map_data.size_x), int(offsets.raw_height), 3, pixels.data(), 0);
		}
		res.set_content(reinterpret_cast<const char*>(cached_png.data()), cached_png.size(), "image/png");
	}));

	svr.Get("/map/coastline_mask.png", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		// A plain binary land(255)/water(0) mask, same shape/crop as province_ids.png --
		// but unlike that texture, this one is meant to be sampled with LINEAR filtering
		// client-side. province_ids.png has to stay NEAREST (averaging two different
		// province ids is meaningless), which is exactly why coastlines built from it look
		// like a hard pixel staircase when zoomed in. This mask carries no per-province
		// identity, just a smooth 0..1 quantity, so ordinary GPU bilinear sampling gives a
		// genuinely anti-aliased coast for free -- no blur pass needed server-side, and no
		// new rendering technique, just the right kind of texture for the job. Cached the
		// same way as the other two per-pixel routes.
		static std::vector<unsigned char> cached_png;
		if(cached_png.empty()) {
			auto& map_data = state.map_state.map_data;
			auto offsets = province_id_map_offsets(map_data);
			uint16_t first_sea_map_id = province::to_map_id(state.province_definitions.first_sea_province);

			std::vector<unsigned char> pixels(size_t(map_data.size_x) * offsets.raw_height * 3);
			for(uint32_t y = 0; y < offsets.raw_height; ++y) {
				for(uint32_t x = 0; x < map_data.size_x; ++x) {
					uint32_t index = (offsets.top_free_space + y) * map_data.size_x + x;
					uint16_t map_id = map_data.province_id_map[index];
					bool is_land = map_id != 0 && map_id < first_sea_map_id;
					size_t out = (size_t(y) * map_data.size_x + x) * 3;
					pixels[out + 0] = is_land ? 255 : 0;
					pixels[out + 1] = pixels[out + 0];
					pixels[out + 2] = pixels[out + 0];
				}
			}

			stbi_write_png_to_func(
				[](void* context, void* data, int size) {
					auto* out = reinterpret_cast<std::vector<unsigned char>*>(context);
					auto* bytes = reinterpret_cast<unsigned char*>(data);
					out->insert(out->end(), bytes, bytes + size);
				},
				&cached_png, int(map_data.size_x), int(offsets.raw_height), 3, pixels.data(), 0);
		}
		res.set_content(reinterpret_cast<const char*>(cached_png.data()), cached_png.size(), "image/png");
	}));

	svr.Get("/map/border_segments", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		// Original run-length merge over province_id_map, not a port of map_borders.cpp's
		// real curved-vertex GPU generator (see the plan doc for why) -- walks column
		// boundaries (vertical edges) and row boundaries (horizontal edges) separately,
		// merging consecutive same-pair pixels into one segment. Pixels are treated as unit
		// squares with an integer top-left corner, so a boundary between column x and x+1
		// sits at the plain integer coordinate x+1 -- no half-integer/fixed-point encoding
		// needed. Coastlines (either side water) are deliberately skipped: vanilla doesn't
		// draw a border line there either, the water/land color difference already reads as
		// a coast. Cached after first computation, same as the two PNG routes above --
		// geometry never changes during a process lifetime.
		static std::vector<unsigned char> cached_buffer;
		if(cached_buffer.empty()) {
			auto& map_data = state.map_state.map_data;
			auto offsets = province_id_map_offsets(map_data);
			uint16_t first_sea_map_id = province::to_map_id(state.province_definitions.first_sea_province);

			auto is_water = [&](uint16_t map_id) {
				return map_id == 0 || map_id >= first_sea_map_id;
			};
			auto pixel_at = [&](uint32_t x, uint32_t y) -> uint16_t {
				return map_data.province_id_map[(offsets.top_free_space + y) * map_data.size_x + x];
			};
			// (a, b) valid (border-worthy) pair between two map ids, or (0, 0) -- a real
			// pair can never be (0, 0) since map_id 0 is always treated as water above.
			auto pair_of = [&](uint16_t a, uint16_t b) -> std::pair<uint16_t, uint16_t> {
				if(a == b || is_water(a) || is_water(b))
					return { 0, 0 };
				return { a, b };
			};

			std::vector<uint16_t> records; // 6 uint16 per segment: x1, y1, x2, y2, a, b

			// Vertical edges: boundary between column x and x+1, at coordinate x+1.
			for(uint32_t x = 0; x + 1 < map_data.size_x; ++x) {
				std::pair<uint16_t, uint16_t> run_pair{ 0, 0 };
				uint32_t run_start_y = 0;
				for(uint32_t y = 0; y < offsets.raw_height; ++y) {
					auto pair = pair_of(pixel_at(x, y), pixel_at(x + 1, y));
					if(pair != run_pair) {
						if(run_pair.first != 0 || run_pair.second != 0) {
							records.insert(records.end(), { uint16_t(x + 1), uint16_t(run_start_y), uint16_t(x + 1), uint16_t(y), run_pair.first, run_pair.second });
						}
						run_pair = pair;
						run_start_y = y;
					}
				}
				if(run_pair.first != 0 || run_pair.second != 0) {
					records.insert(records.end(), { uint16_t(x + 1), uint16_t(run_start_y), uint16_t(x + 1), uint16_t(offsets.raw_height), run_pair.first, run_pair.second });
				}
			}

			// Horizontal edges: boundary between row y and y+1, at coordinate y+1.
			for(uint32_t y = 0; y + 1 < offsets.raw_height; ++y) {
				std::pair<uint16_t, uint16_t> run_pair{ 0, 0 };
				uint32_t run_start_x = 0;
				for(uint32_t x = 0; x < map_data.size_x; ++x) {
					auto pair = pair_of(pixel_at(x, y), pixel_at(x, y + 1));
					if(pair != run_pair) {
						if(run_pair.first != 0 || run_pair.second != 0) {
							records.insert(records.end(), { uint16_t(run_start_x), uint16_t(y + 1), uint16_t(x), uint16_t(y + 1), run_pair.first, run_pair.second });
						}
						run_pair = pair;
						run_start_x = x;
					}
				}
				if(run_pair.first != 0 || run_pair.second != 0) {
					records.insert(records.end(), { uint16_t(run_start_x), uint16_t(y + 1), uint16_t(map_data.size_x), uint16_t(y + 1), run_pair.first, run_pair.second });
				}
			}

			uint32_t segment_count = uint32_t(records.size() / 6);
			cached_buffer.resize(4 + records.size() * 2);
			std::memcpy(cached_buffer.data(), &segment_count, 4);
			std::memcpy(cached_buffer.data() + 4, records.data(), records.size() * 2);
		}
		res.set_content(reinterpret_cast<const char*>(cached_buffer.data()), cached_buffer.size(), "application/octet-stream");
	}));

	svr.Get("/map/province_colors", guarded(state, [&](const httplib::Request& req, httplib::Response& res) {
		// Every Vic2/Alice map mode already has a ready-made X_map_from(state) function
		// (src/map/modes/*.hpp) returning this exact shape -- the same one map_modes.cpp's
		// own native-client dispatch (map_modes.cpp:877-998) feeds into its GL texture
		// upload. Dispatching by name here means every mode's colors are byte-identical to
		// what the native Alice client shows, not a reimplementation. Not cached: game state
		// (ownership, selection, etc.) can change at any time, and this is a cheap single
		// pass over provinces.
		static const std::unordered_map<std::string, std::vector<uint32_t> (*)(sys::state&)> mode_dispatch = {
			{ "political", political_map_from },
			{ "admin", admin_map_from },
			{ "diplomatic", diplomatic_map_from },
			{ "region", region_map_from },
			{ "population", population_map_from },
			{ "nationality", nationality_map_from },
			{ "sphere", sphere_map_from },
			{ "supply", supply_map_from },
			{ "party_loyalty", party_loyalty_map_from },
			{ "rank", rank_map_from },
			{ "migration", migration_map_from },
			{ "civilization_level", civilization_level_map_from },
			{ "infrastructure", infrastructure_map_from },
			{ "revolt", revolt_map_from },
			{ "colonial", colonial_map_from },
			{ "recruitment", recruitment_map_from },
			{ "national_focus", national_focus_map_from },
			{ "rgo_output", rgo_output_map_from },
		};

		std::string mode = req.has_param("mode") ? req.get_param_value("mode") : "political";
		auto it = mode_dispatch.find(mode);
		if(it == mode_dispatch.end()) {
			res.status = 400;
			return;
		}

		auto prov_color = it->second(state);
		res.set_content(extract_primary_colors_json(state, prov_color), "application/json");
	}));

	svr.listen("0.0.0.0", 1234);
}

}

