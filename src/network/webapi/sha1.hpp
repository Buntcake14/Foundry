#pragma once
// Minimal public-domain SHA-1 (in the lineage of Steve Reid's widely-vendored
// implementation). Used ONLY for the RFC6455 WebSocket handshake's Sec-WebSocket-Accept
// computation -- not appropriate for anything security-sensitive.

#include <cstdint>
#include <cstring>
#include <string>
#include <array>
#include <algorithm>

namespace webui {

struct sha1_context {
	uint32_t state[5];
	uint64_t count; // total bytes processed so far
	uint8_t buffer[64];
};

inline uint32_t sha1_rol(uint32_t value, int bits) {
	return (value << bits) | (value >> (32 - bits));
}

inline void sha1_transform(uint32_t state[5], const uint8_t buffer[64]) {
	uint32_t block[80];
	for(int i = 0; i < 16; ++i) {
		block[i] = (uint32_t(buffer[i * 4]) << 24) | (uint32_t(buffer[i * 4 + 1]) << 16) |
			(uint32_t(buffer[i * 4 + 2]) << 8) | uint32_t(buffer[i * 4 + 3]);
	}
	for(int i = 16; i < 80; ++i) {
		block[i] = sha1_rol(block[i - 3] ^ block[i - 8] ^ block[i - 14] ^ block[i - 16], 1);
	}

	uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];

	for(int i = 0; i < 80; ++i) {
		uint32_t f, k;
		if(i < 20) {
			f = (b & c) | ((~b) & d);
			k = 0x5A827999u;
		} else if(i < 40) {
			f = b ^ c ^ d;
			k = 0x6ED9EBA1u;
		} else if(i < 60) {
			f = (b & c) | (b & d) | (c & d);
			k = 0x8F1BBCDCu;
		} else {
			f = b ^ c ^ d;
			k = 0xCA62C1D6u;
		}
		uint32_t temp = sha1_rol(a, 5) + f + e + k + block[i];
		e = d;
		d = c;
		c = sha1_rol(b, 30);
		b = a;
		a = temp;
	}

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;
}

inline void sha1_init(sha1_context& ctx) {
	ctx.state[0] = 0x67452301u;
	ctx.state[1] = 0xEFCDAB89u;
	ctx.state[2] = 0x98BADCFEu;
	ctx.state[3] = 0x10325476u;
	ctx.state[4] = 0xC3D2E1F0u;
	ctx.count = 0;
}

inline void sha1_update(sha1_context& ctx, const uint8_t* data, size_t len) {
	size_t buffer_offset = size_t(ctx.count % 64);
	ctx.count += uint64_t(len);
	size_t i = 0;

	if(buffer_offset > 0) {
		size_t fill = 64 - buffer_offset;
		size_t take = std::min(fill, len);
		std::memcpy(ctx.buffer + buffer_offset, data, take);
		i += take;
		if(buffer_offset + take == 64) {
			sha1_transform(ctx.state, ctx.buffer);
		}
	}
	for(; i + 64 <= len; i += 64) {
		sha1_transform(ctx.state, data + i);
	}
	if(i < len) {
		std::memcpy(ctx.buffer, data + i, len - i);
	}
}

inline std::array<uint8_t, 20> sha1_final(sha1_context& ctx) {
	uint64_t bit_count = ctx.count * 8; // captured BEFORE padding is appended below

	uint8_t pad = 0x80;
	sha1_update(ctx, &pad, 1);

	uint8_t zero = 0x00;
	while(ctx.count % 64 != 56) {
		sha1_update(ctx, &zero, 1);
	}

	uint8_t length_bytes[8];
	for(int i = 0; i < 8; ++i) {
		length_bytes[i] = uint8_t(bit_count >> (56 - i * 8));
	}
	sha1_update(ctx, length_bytes, 8);

	std::array<uint8_t, 20> digest{};
	for(int i = 0; i < 5; ++i) {
		digest[i * 4 + 0] = uint8_t(ctx.state[i] >> 24);
		digest[i * 4 + 1] = uint8_t(ctx.state[i] >> 16);
		digest[i * 4 + 2] = uint8_t(ctx.state[i] >> 8);
		digest[i * 4 + 3] = uint8_t(ctx.state[i]);
	}
	return digest;
}

inline std::array<uint8_t, 20> sha1(const std::string& input) {
	sha1_context ctx;
	sha1_init(ctx);
	sha1_update(ctx, reinterpret_cast<const uint8_t*>(input.data()), input.size());
	return sha1_final(ctx);
}

} // namespace webui
