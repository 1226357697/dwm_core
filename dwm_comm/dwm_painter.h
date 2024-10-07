#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <Windows.h>

#define IM_COL32_R_SHIFT    0
#define IM_COL32_G_SHIFT    8
#define IM_COL32_B_SHIFT    16
#define IM_COL32_A_SHIFT    24
#define IM_COL32_A_MASK     0xFF000000

#define IM_COL32(R,G,B,A)    (((uint32_t)(A)<<IM_COL32_A_SHIFT) | ((uint32_t)(B)<<IM_COL32_B_SHIFT) | ((uint32_t)(G)<<IM_COL32_G_SHIFT) | ((uint32_t)(R)<<IM_COL32_R_SHIFT))
#define IM_COL32_WHITE       IM_COL32(255,255,255,255)  // Opaque white = 0xFFFFFFFF
#define IM_COL32_BLACK       IM_COL32(0,0,0,255)        // Opaque black
#define IM_COL32_BLACK_TRANS IM_COL32(0,0,0,0)          // Transparent black = 0x00000000

#if defined(__cplusplus)
extern "C" {
#endif

void* dwm_painter_init(size_t init_size, size_t screen_w, size_t screen_h, HWND wnd);

void* dwm_painter_buffer(void* painter);

size_t dwm_painter_buffer_size(void* painter);

float dwm_painter_width(void* painter);

float dwm_painter_height(void* painter);

HWND dwm_painter_window(void* painter);

void dwm_painter_new_frame(void* painter);

void dwm_painter_clear(void* painter);

void dwm_painter_add_text(void* painter, const char* str, float x, float y, int color, int size, bool outline);

void dwm_painter_add_line(void* painter, float p1_x, float p1_y, float p2_x, float p2_y, int color, float thickness);

void dwm_painter_add_rect(void* painter, float x, float y, float w, float h, int color, float thickness, float rounding);

void dwm_painter_add_rect_filled(void* painter, float x, float y, float w, float h, int color, float thickness, float rounding);

void dwm_painter_add_circle(void* painter, float x, float y, float radius, int color, float thickness);

void dwm_painter_add_circle_filled(void* painter, float x, float y, float radius, int color, float thickness);

bool dwm_painter_present(void* painter);

void dwm_painter_desory(void* painter);

#if defined(__cplusplus)
}
#endif