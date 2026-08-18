#define STB_IMAGE_IMPLEMENTATION
#include "../../dependencies/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../dependencies/stb/stb_image_write.h"

#include <algorithm>
#include <cmath>
#include <vector>

static void put_pixel(std::vector<unsigned char>& image, int width, int height, int x, int y,
		uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
	if(x < 0 || y < 0 || x >= width || y >= height) return;
	auto const i = (y * width + x) * 4;
	image[i + 0] = r;
	image[i + 1] = g;
	image[i + 2] = b;
	image[i + 3] = a;
}

static void fill_rect(std::vector<unsigned char>& image, int width, int height, int x0, int y0,
		int x1, int y1, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
	for(int y = y0; y < y1; ++y)
		for(int x = x0; x < x1; ++x)
			put_pixel(image, width, height, x, y, r, g, b, a);
}

static void framed_button(std::vector<unsigned char>& image, int width, int height, int x0, int y0,
		int w, int h, bool bright) {
	fill_rect(image, width, height, x0, y0, x0 + w, y0 + h, 19, 7, 10, 255);
	fill_rect(image, width, height, x0 + 1, y0 + 1, x0 + w - 1, y0 + h - 1,
		bright ? 105 : 77, bright ? 23 : 14, bright ? 30 : 22, 255);
	for(int x = x0 + 2; x < x0 + w - 2; ++x) {
		put_pixel(image, width, height, x, y0 + 1, 190, 137, 43);
		put_pixel(image, width, height, x, y0 + h - 2, 83, 48, 14);
	}
	for(int y = y0 + 2; y < y0 + h - 2; ++y) {
		put_pixel(image, width, height, x0 + 1, y, 190, 137, 43);
		put_pixel(image, width, height, x0 + w - 2, y, 83, 48, 14);
	}
}

static void draw_triangle(std::vector<unsigned char>& image, int width, int height, int cx, int cy, bool right) {
	for(int dx = 0; dx < 7; ++dx) {
		int const half = dx / 2;
		int const x = right ? cx + 3 - dx : cx - 3 + dx;
		for(int y = cy - half; y <= cy + half; ++y)
			put_pixel(image, width, height, x, y, 237, 198, 89);
	}
}

static void crop_resize(unsigned char const* src, int sw, int sh, int sx, int sy, int cw, int ch,
		int dw, int dh, int stride, int xoff, float brightness, std::vector<unsigned char>& dst) {
	for(int y = 0; y < dh; ++y) {
		for(int x = 0; x < dw; ++x) {
			float fx = sx + (x + 0.5f) * cw / dw - 0.5f;
			float fy = sy + (y + 0.5f) * ch / dh - 0.5f;
			int x0 = std::clamp(int32_t(std::floor(fx)), 0, sw - 1);
			int x1 = std::min(x0 + 1, sw - 1);
			int y0 = std::clamp(int32_t(std::floor(fy)), 0, sh - 1);
			int y1 = std::min(y0 + 1, sh - 1);
			float tx = fx - std::floor(fx);
			float ty = fy - std::floor(fy);
			for(int c = 0; c < 4; ++c) {
				float a = src[(y0 * sw + x0) * 4 + c] * (1 - tx) + src[(y0 * sw + x1) * 4 + c] * tx;
				float b = src[(y1 * sw + x0) * 4 + c] * (1 - tx) + src[(y1 * sw + x1) * 4 + c] * tx;
				float v = (a * (1 - ty) + b * ty) * (c == 3 ? 1.0f : brightness);
				dst[(y * stride + x + xoff) * 4 + c] = uint8_t(std::clamp(v, 0.0f, 255.0f));
			}
		}
	}
}

int main(int argc, char** argv) {
	if(argc != 10) return 2;
	int sw = 0, sh = 0, channels = 0;
	auto* src = stbi_load(argv[1], &sw, &sh, &channels, 4);
	if(!src) return 3;

	// The source atlas contains a blank identity card on the left and a blank
	// module card on the right.  Keep all labels/icons in the live GUI, never in
	// these raster layers, so the artwork can be reused by every module.
	std::vector<unsigned char> module(280 * 92 * 4);
	crop_resize(src, sw, sh, 1184, 301, 476, 302, 140, 92, 280, 0, 1.0f, module);
	crop_resize(src, sw, sh, 1184, 301, 476, 302, 140, 92, 280, 140, 1.10f, module);
	stbi_write_png(argv[2], 280, 92, 4, module.data(), 280 * 4);

	// The shell owns the wider nation identity card.  The remainder is a quiet
	// burgundy backing strip underneath the individual module sprites.
	std::vector<unsigned char> shell(1600 * 118 * 4);
	for(int y = 0; y < 118; ++y) {
		for(int x = 0; x < 1600; ++x) {
			auto const i = (y * 1600 + x) * 4;
			shell[i + 0] = 22;
			shell[i + 1] = 8;
			shell[i + 2] = 11;
			shell[i + 3] = 255;
		}
	}
	crop_resize(src, sw, sh, 82, 301, 996, 302, 340, 118, 1600, 0, 1.0f, shell);
	stbi_write_png(argv[3], 1600, 118, 4, shell.data(), 1600 * 4);

	// Foundry-native time controls for the nation identity card. These replace
	// the wide parchment and black Vic2 speed-control backing sprites.
	std::vector<unsigned char> step_up(48 * 24 * 4, 0);
	std::vector<unsigned char> step_down(48 * 24 * 4, 0);
	for(int frame = 0; frame < 2; ++frame) {
		framed_button(step_up, 48, 24, frame * 24, 0, 24, 24, frame == 1);
		framed_button(step_down, 48, 24, frame * 24, 0, 24, 24, frame == 1);
		draw_triangle(step_up, 48, 24, frame * 24 + 12, 12, true);
		draw_triangle(step_down, 48, 24, frame * 24 + 12, 12, false);
	}
	stbi_write_png(argv[4], 48, 24, 4, step_up.data(), 48 * 4);
	stbi_write_png(argv[5], 48, 24, 4, step_down.data(), 48 * 4);

	std::vector<unsigned char> pause(48 * 24 * 4, 0);
	framed_button(pause, 48, 24, 0, 0, 24, 24, false);
	framed_button(pause, 48, 24, 24, 0, 24, 24, true);
	for(int frame = 0; frame < 2; ++frame) {
		int const ox = frame * 24;
		fill_rect(pause, 48, 24, ox + 8, 7, ox + 10, 17, 237, 198, 89);
		fill_rect(pause, 48, 24, ox + 14, 7, ox + 16, 17, 237, 198, 89);
	}
	stbi_write_png(argv[6], 48, 24, 4, pause.data(), 48 * 4);

	std::vector<unsigned char> indicator(144 * 24 * 4, 0);
	for(int frame = 0; frame < 6; ++frame) {
		int const ox = frame * 24;
		framed_button(indicator, 144, 24, ox, 0, 24, 24, frame > 0);
		if(frame == 0) {
			fill_rect(indicator, 144, 24, ox + 7, 11, ox + 17, 13, 237, 198, 89);
		} else {
			for(int bar = 0; bar < frame; ++bar) {
				int const bar_height = 4 + bar * 2;
				fill_rect(indicator, 144, 24, ox + 5 + bar * 3, 18 - bar_height,
					ox + 7 + bar * 3, 18, 237, 198, 89);
			}
		}
	}
	stbi_write_png(argv[7], 144, 24, 4, indicator.data(), 144 * 4);

	// Matched 84px circular frame and mask for the nation identity card.
	std::vector<unsigned char> flag_frame(84 * 84 * 4, 0);
	std::vector<unsigned char> flag_mask(84 * 84 * 4, 0);
	for(int y = 0; y < 84; ++y) {
		for(int x = 0; x < 84; ++x) {
			float const dx = (x + 0.5f) - 42.0f;
			float const dy = (y + 0.5f) - 42.0f;
			float const d = std::sqrt(dx * dx + dy * dy);
			if(d <= 34.0f) put_pixel(flag_mask, 84, 84, x, y, 255, 255, 255, 255);
			if(d >= 33.0f && d <= 39.0f) {
				float const shine = std::clamp((39.0f - d) / 6.0f, 0.0f, 1.0f);
				put_pixel(flag_frame, 84, 84, x, y,
					uint8_t(105 + 120 * shine), uint8_t(55 + 95 * shine), uint8_t(12 + 25 * shine), 255);
			}
		}
	}
	stbi_write_png(argv[8], 84, 84, 4, flag_frame.data(), 84 * 4);
	stbi_write_png(argv[9], 84, 84, 4, flag_mask.data(), 84 * 4);
	stbi_image_free(src);
}
