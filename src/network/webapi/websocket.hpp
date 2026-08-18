#pragma once
// Minimal RFC6455 WebSocket server for Foundry's web UI push channel. Hand-rolled
// rather than pulling in a library, matching this codebase's existing pattern of
// hand-rolling raw sockets for multiplayer in network.cpp/network.hpp. Deliberately
// small: one first-party browser client, server mostly pushes short text
// notifications, no compression/fragmentation/binary-frame support.
//
// Runs on its own port (default 1235) separate from the httplib JSON API (1234),
// since cpp-httplib's blocking accept loop can't share a socket with a WS upgrade
// handshake -- see docs/... Phase 1 plan for the reasoning.

#include "sha1.hpp"
#include "system_state.hpp"

#include <string>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cstdint>

#ifdef _WIN64
#ifndef WINSOCK2_IMPORTED
#define WINSOCK2_IMPORTED
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

namespace webui {
namespace ws {

#ifdef _WIN64
using ws_socket_t = SOCKET;
inline constexpr ws_socket_t ws_invalid_socket = INVALID_SOCKET;
#else
using ws_socket_t = int;
inline constexpr ws_socket_t ws_invalid_socket = -1;
#endif

inline void ws_close(ws_socket_t s) {
#ifdef _WIN64
	closesocket(s);
#else
	close(s);
#endif
}

inline int ws_send_all(ws_socket_t s, const uint8_t* data, size_t len) {
	size_t sent = 0;
	while(sent < len) {
#ifdef _WIN64
		int n = send(s, reinterpret_cast<const char*>(data + sent), int(len - sent), 0);
#else
		int n = int(send(s, data + sent, len - sent, MSG_NOSIGNAL));
#endif
		if(n <= 0)
			return -1;
		sent += size_t(n);
	}
	return int(sent);
}

inline int ws_recv_some(ws_socket_t s, uint8_t* data, size_t len) {
#ifdef _WIN64
	return recv(s, reinterpret_cast<char*>(data), int(len), 0);
#else
	return int(recv(s, data, len, 0));
#endif
}

// Small local base64 encoder for the handshake's Sec-WebSocket-Accept value.
// Kept self-contained rather than reaching into vendored httplib::detail internals.
inline std::string base64_encode(const uint8_t* data, size_t len) {
	static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string out;
	out.reserve(((len + 2) / 3) * 4);
	size_t i = 0;
	for(; i + 3 <= len; i += 3) {
		uint32_t n = (uint32_t(data[i]) << 16) | (uint32_t(data[i + 1]) << 8) | uint32_t(data[i + 2]);
		out += table[(n >> 18) & 0x3F];
		out += table[(n >> 12) & 0x3F];
		out += table[(n >> 6) & 0x3F];
		out += table[n & 0x3F];
	}
	size_t rem = len - i;
	if(rem == 1) {
		uint32_t n = uint32_t(data[i]) << 16;
		out += table[(n >> 18) & 0x3F];
		out += table[(n >> 12) & 0x3F];
		out += "==";
	} else if(rem == 2) {
		uint32_t n = (uint32_t(data[i]) << 16) | (uint32_t(data[i + 1]) << 8);
		out += table[(n >> 18) & 0x3F];
		out += table[(n >> 12) & 0x3F];
		out += table[(n >> 6) & 0x3F];
		out += "=";
	}
	return out;
}

struct connection {
	ws_socket_t sock = ws_invalid_socket;
};

inline std::vector<connection> connections;
inline std::mutex connections_mutex;

// Extremely small line-oriented reader, just enough to find the Sec-WebSocket-Key
// header of a browser's upgrade request. Not a general HTTP parser.
inline bool read_handshake_key(ws_socket_t s, std::string& key_out) {
	std::string buffer;
	uint8_t chunk[512];
	for(int guard = 0; guard < 64; ++guard) {
		int n = ws_recv_some(s, chunk, sizeof(chunk));
		if(n <= 0)
			return false;
		buffer.append(reinterpret_cast<char*>(chunk), size_t(n));
		if(buffer.find("\r\n\r\n") != std::string::npos)
			break;
	}
	const std::string marker = "Sec-WebSocket-Key:";
	auto pos = buffer.find(marker);
	if(pos == std::string::npos)
		return false;
	pos += marker.size();
	auto line_end = buffer.find("\r\n", pos);
	if(line_end == std::string::npos)
		return false;
	std::string value = buffer.substr(pos, line_end - pos);
	auto start = value.find_first_not_of(" \t");
	auto end = value.find_last_not_of(" \t");
	if(start == std::string::npos)
		return false;
	key_out = value.substr(start, end - start + 1);
	return true;
}

inline void handle_connection(ws_socket_t s) {
	std::string key;
	if(!read_handshake_key(s, key)) {
		ws_close(s);
		return;
	}
	static const std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	auto digest = sha1(key + magic);
	std::string accept = base64_encode(digest.data(), digest.size());

	std::string response = "HTTP/1.1 101 Switching Protocols\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: " + accept + "\r\n\r\n";
	if(ws_send_all(s, reinterpret_cast<const uint8_t*>(response.data()), response.size()) < 0) {
		ws_close(s);
		return;
	}

	{
		std::lock_guard<std::mutex> lock(connections_mutex);
		connections.push_back(connection{ s });
	}

	// Reader loop: Phase 1 has no application-level messages from the client, but we
	// keep reading so we notice Close frames / disconnects and can answer a Ping.
	uint8_t frame_buf[512];
	for(;;) {
		int n = ws_recv_some(s, frame_buf, sizeof(frame_buf));
		if(n <= 0)
			break;
		if(size_t(n) < 2)
			continue;
		uint8_t opcode = frame_buf[0] & 0x0F;
		if(opcode == 0x8) // close
			break;
		if(opcode == 0x9) { // ping -> pong (empty payload; good enough for liveness)
			uint8_t pong[2] = { uint8_t(0x80 | 0xA), 0x00 };
			ws_send_all(s, pong, 2);
		}
	}

	{
		std::lock_guard<std::mutex> lock(connections_mutex);
		connections.erase(std::remove_if(connections.begin(), connections.end(),
			[&](const connection& c) { return c.sock == s; }), connections.end());
	}
	ws_close(s);
}

inline void broadcast(const std::string& message) {
	// Single unmasked text frame (server->client frames are never masked per RFC6455).
	std::vector<uint8_t> frame;
	size_t len = message.size();
	frame.push_back(0x80 | 0x1); // FIN + text opcode
	if(len <= 125) {
		frame.push_back(uint8_t(len));
	} else if(len <= 0xFFFF) {
		frame.push_back(126);
		frame.push_back(uint8_t((len >> 8) & 0xFF));
		frame.push_back(uint8_t(len & 0xFF));
	} else {
		frame.push_back(127);
		for(int i = 7; i >= 0; --i)
			frame.push_back(uint8_t((uint64_t(len) >> (i * 8)) & 0xFF));
	}
	frame.insert(frame.end(), message.begin(), message.end());

	std::lock_guard<std::mutex> lock(connections_mutex);
	for(auto& c : connections) {
		ws_send_all(c.sock, frame.data(), frame.size());
	}
}

// Blocking accept loop -- call from its own thread (mirrors webui::init's own
// blocking svr.listen() in controllers.hpp).
inline void init(sys::state& state, uint16_t port = 1235) {
	if(state.host_settings.alice_expose_webui == 0 || state.network_mode == sys::network_mode_type::client) {
		return;
	}
#ifdef _WIN64
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
	ws_socket_t listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_sock == ws_invalid_socket)
		return;

	int reuse = 1;
	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if(bind(listen_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
		ws_close(listen_sock);
		return;
	}
	if(listen(listen_sock, 16) != 0) {
		ws_close(listen_sock);
		return;
	}

	for(;;) {
		ws_socket_t client = accept(listen_sock, nullptr, nullptr);
		if(client == ws_invalid_socket)
			continue;
		std::thread(handle_connection, client).detach();
	}
}

// Watches sys::state's tick counter and broadcasts a small notification whenever it
// changes. Deliberately coarse for Phase 1 -- see the plan doc: clients just refetch
// whatever they currently have open rather than receiving a real delta.
inline void run_notify_loop(sys::state& state) {
	int64_t last_seen = -1;
	for(;;) {
		std::this_thread::sleep_for(std::chrono::milliseconds(75));

		int64_t current;
		{
			std::shared_lock<std::shared_mutex> lock(state.webui_state_lock);
			current = state.tick_end_counter.load(std::memory_order::acquire);
		}
		if(current == last_seen)
			continue;
		last_seen = current;

		broadcast(std::string("{\"type\":\"tick\",\"seq\":") + std::to_string(current) + "}");
	}
}

} // namespace ws
} // namespace webui
