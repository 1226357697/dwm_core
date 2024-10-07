#pragma once

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>

#pragma pack(push) 
#pragma pack(4) 
enum class draw_type : int32_t
{
	unknown = 0,
	text,
	circle,
	rect,
	line,

};

struct text_buffer {
	float_t x;
	float_t y;
	uint32_t color;
	int32_t size;
	bool outline;
	char text[0x1EB];
	text_buffer(const text_buffer& copy) :x(copy.x), y(copy.y), color(copy.color), size(copy.size), outline(copy.outline) {
		memcpy(text, copy.text, sizeof(text));
	}
	text_buffer() = default;
};

struct circle_buffer {
	float_t x;
	float_t y;
	float_t radius;
	uint32_t color;
	float_t thickness;
	bool fill;
	circle_buffer() = default;
};
struct rect_buffer {
	float_t x;
	float_t y;
	float_t w;
	float_t h;
	uint32_t color;
	float_t thickness;
	float_t rounding;
	bool fill;
	rect_buffer() = default;
};
struct line_buffer {
	float_t x1;
	float_t y1;
	float_t x2;
	float_t y2;
	uint32_t color;
	float_t thickness;
	line_buffer() = default;
};

union _info
{
	text_buffer	  text;
	circle_buffer circle;
	rect_buffer   rect;
	line_buffer	  line;
	_info() = default;
};

struct draw_info {
	draw_type type;
	_info info;
	draw_info(draw_type type) :type(type) { memset(&info, 0, sizeof(info)); }
	draw_info(const draw_info& copy) {
		this->type = copy.type;
		if (copy.type == draw_type::text) {
			this->info.text = copy.info.text;
		}
		else if (copy.type == draw_type::line) {
			this->info.line = copy.info.line;
		}
		else if (copy.type == draw_type::rect) {
			this->info.rect = copy.info.rect;
		}
		else if (copy.type == draw_type::circle) {
			this->info.circle = copy.info.circle;
		}
	}
};

#pragma pack(pop) 