#include <iostream>
#include "../dwm_comm/dwm_painter.h"
#include <Windows.h>



std::string CharToUtf8(const std::string& asciiStr)
{

  int wideCharLength = MultiByteToWideChar(CP_ACP, 0, asciiStr.c_str(), -1, NULL, 0);
  if (wideCharLength == 0)
    return "";

  std::wstring wideStr(wideCharLength, 0);
  MultiByteToWideChar(CP_ACP, 0, asciiStr.c_str(), -1, &wideStr[0], wideCharLength);

  int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
  if (utf8Length == 0)
    return "";

  std::string utf8Str(utf8Length, 0);
  WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], utf8Length, NULL, NULL);

  return utf8Str;
}


int main()
{
  printf("%X\n%X\n", LoadLibraryA("d3d11.dll"), LoadLibraryA("dxgi.dll"));

  HWND hwnd = (HWND)0X00090B0E;
  void* painter = dwm_painter_init(800, 0, 0, hwnd, true);
  printf("painter: %p\n", painter);
  if(painter == NULL)
    return 1;


  dwm_painter_clear(painter);
  while (!(GetAsyncKeyState(VK_END) & 0x8000))
  {
    dwm_painter_new_frame(painter);

    const char* test_txt = (const char*)"¦²×ÔÃé×ÔÃé×ÔÃé£¡£¡£¡\nÎäÆ÷\n ×°±¸\n MI6A4\n AK\n ¿ÕÍ¶\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷Îä\nÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\n";

    dwm_painter_add_text(painter, CharToUtf8(test_txt).c_str(), 0.0f, 0.0f, IM_COL32(100, 100, 0, 255), 50, false);

    dwm_painter_add_line(painter, 100, 100, 500, 300, 0xff0000ff, 1.0f);
    dwm_painter_add_rect(painter, 600, 300, 50, 50, 0xff0000ff, 1.0f, 5.0f);
    dwm_painter_add_rect_filled(painter, 800, 300, 50, 50, 0xff0000ff, 1.0f, 5.0f);
    dwm_painter_add_circle(painter, 100, 900, 50, 0xff0000ff, 1.0f);
    dwm_painter_add_circle_filled(painter, 700, 900, 50, 0xffffff00, 1.0f);

    dwm_painter_present(painter);
  }
  dwm_painter_clear(painter);
  dwm_painter_desory(painter);

  return 0;
}