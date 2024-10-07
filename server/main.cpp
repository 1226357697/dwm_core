#include <iostream>
#include "../dwm_comm/dwm_comm.h"


enum class draw_type : int32_t
{
	unknown = 0,
	text,
	circle,
	rect,
	line,

};

#pragma pack(push) 
#pragma pack(4) 
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

static
BOOL EnablePrivilege(LPCTSTR szPrivilege, BOOL fEnable)
{
  BOOL fOk = FALSE;
  HANDLE hToken = NULL;
  if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    LookupPrivilegeValue(NULL, szPrivilege, &tp.Privileges[0].Luid);
    tp.Privileges->Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;
    AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    fOk = (GetLastError() == ERROR_SUCCESS);
    CloseHandle(hToken);
  }
  return fOk;
}

int main()
{
  LoadLibraryA("d3d11.dll"); 
  EnablePrivilege(SE_DEBUG_NAME, TRUE);

  void* commer = dwm_comm_create();
  printf("%p\n", commer);

  void* buffer = NULL;
  size_t buffer_size = 0;
  while (true)
  {
    
    if (dwm_comm_get_data(commer, &buffer, &buffer_size))
    {
      //printf("buffer size:%d", buffer_size);
			int count = 0;
			draw_info* info_ptr = (draw_info*)buffer;
			while (info_ptr->type != draw_type::unknown)
			{

				printf("type :%d\n", info_ptr->type);
				info_ptr++;
				count++;
			}
			if (count > 0)
			{

				printf("--------------------count: %d---------------------------\n", count);
			}
    }
    else
    {

    }
    if (buffer)
    {
      free(buffer);
      buffer = NULL;
    }
    Sleep(200);
  }

  getchar();
  return 0;
}