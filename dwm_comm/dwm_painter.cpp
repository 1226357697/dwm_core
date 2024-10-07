#include "dwm_painter.h"
#include <vector>
#include "dwm_comm.h"
#include "../dwm_core/dwm_painter_common.h"
#include "importer.h"

struct dwm_painter
{
	std::vector<draw_info>draw_buffer;
  float screen_w;
	float screen_h;
	void* dwm_comm;
	double scale;
	HWND hwnd;
};

static inline dwm_painter* to_painter(void* object)
{
	return reinterpret_cast<dwm_painter*>(object);
}


static double GetScreenScale_Win7()
{
	SetProcessDPIAware();  // 设置应用为 DPI 感知
	HDC screen = GetDC(NULL);
	int dpi = GetDeviceCaps(screen, LOGPIXELSX);
	ReleaseDC(NULL, screen);
	return dpi / 96.0f;

}

static double GetScreenScale_Win10()
{
	SetProcessDPIAware();  // 设置应用为 DPI 感知
	HWND hd = GetDesktopWindow();
	int zoom = GetDpiForWindow(hd);
	return zoom / 96.0f;
}

static double GetScreenScale()
{
	quick_import_function("ntdll.dll", RtlGetVersion);

	RTL_OSVERSIONINFOW osversion;
	if (NT_SUCCESS(RtlGetVersion(&osversion)))
	{
		if (osversion.dwMajorVersion == 10 && osversion.dwBuildNumber >= 10240)
		{
			return GetScreenScale_Win10();
		}
		else
		{
			return GetScreenScale_Win7();
		}
	}
	return 1.0f;
}

void* dwm_painter_init(size_t init_size, size_t screen_w, size_t screen_h, HWND wnd)
{
	void* dwm_comm = dwm_comm_open();
	if(dwm_comm == NULL)
		return NULL;

	dwm_painter* painter = new(std::nothrow) dwm_painter;
	if(painter == NULL)
		return NULL;

	painter->screen_h = screen_w;
	painter->screen_w = screen_h;
	painter->draw_buffer.reserve(init_size);
	painter->dwm_comm = dwm_comm;
	painter->hwnd = wnd;
	painter->scale = GetScreenScale();
	return painter;
}

void* dwm_painter_buffer(void* painter)
{
	dwm_painter* _painter = to_painter(painter);
	return _painter->draw_buffer.data();
}

size_t dwm_painter_buffer_size(void* painter)
{
	dwm_painter* _painter = to_painter(painter);
	return _painter->draw_buffer.size();
}

float dwm_painter_width(void* painter)
{
	dwm_painter* _painter = to_painter(painter);

  return _painter->screen_w;
}

float dwm_painter_height(void* painter)
{
	dwm_painter* _painter = to_painter(painter);

	return _painter->screen_h;
}

HWND dwm_painter_window(void* painter)
{
	dwm_painter* _painter = to_painter(painter);
	return _painter->hwnd;
}

void dwm_painter_new_frame(void* painter)
{
	dwm_painter* _painter = to_painter(painter);
	return _painter->draw_buffer.clear();
}

void dwm_painter_clear(void* painter)
{
	dwm_painter* _painter = to_painter(painter);
	dwm_comm_update_data(_painter->dwm_comm, NULL, 0);
}

void dwm_painter_add_text(void* painter, const char* str, float x, float y, int color, int size, bool outline)
{
	dwm_painter* _painter = to_painter(painter);
	double sacle = _painter->scale;
	x *= sacle;
	y *= sacle;

	RECT rc = {};
	GetWindowRect(_painter->hwnd, &rc);
	draw_info info(draw_type::text);
	info.info.text.x = x + rc.left;
	info.info.text.y = y + rc.top;
	info.info.text.color = color;
	info.info.text.size = size;
	info.info.text.outline = outline;

	size_t copy_size = strlen(str) + 1;
	copy_size = copy_size > sizeof(info.info.text.text) ? sizeof(info.info.text.text) : copy_size;
	memcpy_s(info.info.text.text, sizeof(info.info.text.text), str, copy_size);
	_painter->draw_buffer.push_back(info);
}

void dwm_painter_add_line(void* painter, float p1_x, float p1_y, float p2_x, float p2_y, int color, float thickness)
{
	dwm_painter* _painter = to_painter(painter);
	double sacle = _painter->scale;
	p1_x *= sacle;
	p1_y *= sacle;
	p2_x *= sacle;
	p2_y *= sacle;

	RECT rc = {};
	GetWindowRect(_painter->hwnd, &rc);
	draw_info info(draw_type::line);
	info.info.line.x1 = p1_x + rc.left;
	info.info.line.y1 = p1_y + rc.top;
	info.info.line.x2 = p2_x + rc.left;
	info.info.line.y2 = p2_y + rc.top;
	info.info.line.color = color;
	info.info.line.thickness = thickness;
	_painter->draw_buffer.push_back(info);
}

void dwm_painter_add_rect(void* painter, float x, float y, float w, float h, int color, float thickness, float rounding)
{
	dwm_painter* _painter = to_painter(painter);
	double sacle = _painter->scale;
	x *= sacle;
	y *= sacle;

	RECT rc = {};
	GetWindowRect(_painter->hwnd, &rc);
	draw_info info(draw_type::rect);
	info.info.rect.x = x + rc.left;
	info.info.rect.y = y + rc.top;
	info.info.rect.w = w;
	info.info.rect.h = h;
	info.info.rect.color = color;
	info.info.rect.thickness = thickness;
	info.info.rect.rounding = rounding;
	info.info.rect.fill = false;
	_painter->draw_buffer.push_back(info);
}

void dwm_painter_add_rect_filled(void* painter, float x, float y, float w, float h, int color, float thickness, float rounding)
{
	dwm_painter* _painter = to_painter(painter);
	double sacle = _painter->scale;
	x *= sacle;
	y *= sacle;

	RECT rc = {};
	GetWindowRect(_painter->hwnd, &rc);
	draw_info info(draw_type::rect);
	info.info.rect.x = x + rc.left;
	info.info.rect.y = y + rc.top;
	info.info.rect.w = w;
	info.info.rect.h = h;
	info.info.rect.color = color;
	info.info.rect.thickness = thickness;
	info.info.rect.rounding = rounding;
	info.info.rect.fill = true;
	_painter->draw_buffer.push_back(info);
}

void dwm_painter_add_circle(void* painter, float x, float y, float radius, int color, float thickness)
{
	dwm_painter* _painter = to_painter(painter);
	double sacle = _painter->scale;
	x *= sacle;
	y *= sacle;

	RECT rc = {};
	GetWindowRect(_painter->hwnd, &rc);
	draw_info info(draw_type::circle);
	info.info.circle.x = x + rc.left;
	info.info.circle.y = y + rc.top;
	info.info.circle.radius = radius;
	info.info.circle.color = color;
	info.info.circle.thickness = thickness;
	info.info.circle.fill = false;
	_painter->draw_buffer.push_back(info);
}

void dwm_painter_add_circle_filled(void* painter, float x, float y, float radius, int color, float thickness)
{
	dwm_painter* _painter = to_painter(painter);
	double sacle = _painter->scale;
	x *= sacle;
	y *= sacle;

	RECT rc = {};
	GetWindowRect(_painter->hwnd, &rc);
	draw_info info(draw_type::circle);
	info.info.circle.x = x + rc.left;
	info.info.circle.y = y + rc.top;
	info.info.circle.radius = radius;
	info.info.circle.color = color;
	info.info.circle.thickness = thickness;
	info.info.circle.fill = true;
	_painter->draw_buffer.push_back(info);
}

bool dwm_painter_present(void* painter)
{
	dwm_painter* _painter = to_painter(painter);
	return dwm_comm_update_data(_painter->dwm_comm, dwm_painter_buffer(_painter), dwm_painter_buffer_size(_painter) * sizeof(draw_info));
}

void dwm_painter_desory(void* painter)
{
	dwm_painter* _painter = to_painter(painter);
	dwm_comm_free(_painter->dwm_comm);
	delete _painter;
}
