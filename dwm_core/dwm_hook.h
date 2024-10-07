#pragma once
#include <Windows.h>
#include "dwm_render.h"

class dwm_hook
{
public:
  bool hook();
  static dwm_hook& instance();

private:
  dwm_hook();
private:
  bool hook_win7();
  bool hook_win10();
  bool veh_hook(void* address);
  bool veh_unhook();

  static __int64 __fastcall detours_present_hook(__int64 a1, __int64 a2, __int64 a3, __int64 a4, __int64 a5, __int64 a6, __int64 a7, __int64 a8);
  static LONG NTAPI veh_callback(struct _EXCEPTION_POINTERS* ExceptionInfo);

private:
  dwm_render dwm_render_;
  void* veh_handle_;
};

