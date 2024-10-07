#include "ntdll.h"
#include <iostream>
#include "dwm_hook.h"
#include "debug.hpp"  

HINSTANCE moduleBase = 0;
DWORD WINAPI main_thread_entry(LPVOID lpThreadParameter)
{
  bool ret = false;
  ret = dwm_hook::instance().hook();


  if (!ret)
  {
    FreeLibraryAndExitThread(moduleBase, 1);
  }
  return 0;
}



BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) 
{

  switch (fdwReason)
  {
  case DLL_PROCESS_ATTACH:
  {
    moduleBase = hinstDLL;
    DisableThreadLibraryCalls(hinstDLL);
    HANDLE thread = CreateThread(NULL, NULL, &main_thread_entry, hinstDLL, 0, NULL);
    return thread != NULL && CloseHandle(thread);
  }
  case DLL_THREAD_ATTACH:
    break;

  case DLL_THREAD_DETACH:
    break;

  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}