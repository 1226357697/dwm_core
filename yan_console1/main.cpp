#include <iostream>
#include <algorithm>
#include <vector>
#include <functional>
#include <assert.h>
#include <thread>
#include <Windows.h>
#include "../dwm_core/importer.h"
#pragma comment(lib, "ntdll.lib")


#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)
extern PVOID NTAPI RtlAddVectoredExceptionHandler(IN ULONG FirstHandler, IN PVECTORED_EXCEPTION_HANDLER VectoredHandler);
extern ULONG NTAPI RtlRemoveVectoredExceptionHandler(IN PVOID VectoredHandlerHandle);

void* veh_handle_;
void* s_old_Present_ptr;

VOID foo()
{
  printf("over\n");
  return ;
}

static LONG NTAPI veh_callback(struct _EXCEPTION_POINTERS* ExceptionInfo)
{
  if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP 
    && ExceptionInfo->ExceptionRecord->ExceptionAddress == foo)
  {
    printf("veh_callback\n");
    ExceptionInfo->ContextRecord->Dr7 = 0;
    return EXCEPTION_CONTINUE_EXECUTION;
  }
  
  return EXCEPTION_CONTINUE_SEARCH;
}

static void enum_thread(std::function<bool(HANDLE)> cb)
{
  if (!cb)
    return;

  quick_import_function("ntdll.dll", NtQuerySystemInformation);

  NTSTATUS status = 0;
  ULONG bufferSize = 0x10000;
  std::vector<uint8_t> buffer(bufferSize);
  while (NtQuerySystemInformation(SystemProcessInformation, buffer.data(), buffer.size(), &bufferSize) == STATUS_INFO_LENGTH_MISMATCH)
  {
    buffer.resize(bufferSize);
  }

  DWORD pid = GetCurrentProcessId();
  PSYSTEM_PROCESS_INFORMATION processInfo = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(buffer.data());
  while (processInfo)
  {
    if (reinterpret_cast<DWORD>(processInfo->UniqueProcessId) == pid)
    {
      // 遍历线程
      for (ULONG i = 0; i < processInfo->NumberOfThreads; ++i)
      {
        SYSTEM_THREAD_INFORMATION& threadInfo = processInfo->Threads[i];
        if (!cb(threadInfo.ClientId.UniqueThread))
        {
          return;
        }
      }
      break;
    }

    if (processInfo->NextEntryOffset == 0) break;
    processInfo = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(reinterpret_cast<BYTE*>(processInfo) + processInfo->NextEntryOffset);
  }
}

bool veh_hook(void* address)
{
  assert(veh_handle_ == nullptr);
  if (veh_handle_)
    return false;

  quick_import_function("ntdll.dll", RtlAddVectoredExceptionHandler);
  quick_import_function("ntdll.dll", NtSetContextThread);
  quick_import_function("ntdll.dll", NtSuspendThread);
  quick_import_function("ntdll.dll", NtResumeThread);
  quick_import_function("ntdll.dll", NtOpenThread);

  // 设置全局硬件断点

  veh_handle_ = RtlAddVectoredExceptionHandler(1, &veh_callback);
  if (veh_handle_ == nullptr)
    return false;


  // 遍历所有进程
  DWORD tid = GetCurrentThreadId();
  enum_thread([&](HANDLE threadid)->bool {
    if (threadid != (HANDLE)tid)
    {
      // 设置硬断点
      HANDLE thread = NULL;
      OBJECT_ATTRIBUTES thread_attr;
      InitializeObjectAttributes(&thread_attr, NULL, 0, NULL, 0);

      CLIENT_ID thid_id;
      thid_id.UniqueProcess = 0;
      thid_id.UniqueThread = threadid;
      if (NT_SUCCESS(NtOpenThread(&thread, THREAD_SET_CONTEXT | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION | THREAD_SUSPEND_RESUME, &thread_attr, &thid_id)) && thread != NULL)
      {
        DWORD thread_status_count = 0;
        NtSuspendThread(thread, &thread_status_count);

        CONTEXT thread_context = {0 };
        thread_context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        thread_context.Dr0 = reinterpret_cast<decltype(thread_context.Dr0)> (address);
        thread_context.Dr7 = 1;
        NtSetContextThread(thread, &thread_context);

        NtResumeThread(thread, &thread_status_count);
        CloseHandle(thread);
      }
    }
      return true;
    });

  return true;
}

bool veh_unhook()
{
  assert(veh_handle_);
  if (veh_handle_ == nullptr)
    return false;

  quick_import_function("ntdll.dll", RtlRemoveVectoredExceptionHandler);

  // 遍历所有进程
  DWORD tid = GetCurrentThreadId();
  enum_thread([&](HANDLE threadid)->bool {
    if (threadid != (HANDLE)tid)
    {
      // 设置硬断点
      HANDLE thread = NULL;
      OBJECT_ATTRIBUTES thread_attr;
      InitializeObjectAttributes(&thread_attr, NULL, 0, NULL, 0);

      CLIENT_ID thid_id;
      thid_id.UniqueProcess = 0;
      thid_id.UniqueThread = threadid;
      if (NT_SUCCESS(NtOpenThread(&thread, THREAD_SET_CONTEXT | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, &thread_attr, &thid_id)) && thread != NULL)
      {
        DWORD thread_status_count = 0;
        NtSuspendThread(thread, &thread_status_count);

        CONTEXT thread_context = { 0 };
        thread_context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        thread_context.Dr0 = 0;
        thread_context.Dr7 = 0;
        NtSetContextThread(thread, &thread_context);

        NtResumeThread(thread, &thread_status_count);
        CloseHandle(thread);
      }
    }
    return true;
    });

  RtlRemoveVectoredExceptionHandler(veh_handle_);
  veh_handle_ = nullptr;

  return false;
}

int main()
{
  LoadLibraryA("dwm_core.dll");
  getchar();
  return 0;
  std::thread([]() {
    for (int i = 0; i < 1000; ++i)
    {
      foo();
      Sleep(1000);
    }
    }).detach();
  veh_hook(&foo);


  //veh_unhook();
  //foo();
  Sleep(500);
  //veh_unhook();
  for (size_t i = 0; i < 20; i++)
  {

    Sleep(500);
  }

  return 0;
}