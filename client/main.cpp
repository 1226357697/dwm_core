#include <iostream>
#include "../dwm_comm/dwm_painter.h"
#include <Windows.h>






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

    const char* test_txt = (const char*)u8"¦²×ÔÃé×ÔÃé×ÔÃé£¡£¡£¡\nÎäÆ÷\n ×°±¸\n MI6A4\n AK\n ¿ÕÍ¶\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷Îä\nÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\nÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷ÎäÆ÷\n";

    dwm_painter_add_text(painter, test_txt, 0.0f, 0.0f , IM_COL32(100, 100, 0, 255), 50, false);

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