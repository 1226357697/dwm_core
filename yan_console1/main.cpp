#include <iostream>
#include <algorithm>
#include <vector>
#include <functional>
#include <assert.h>
#include <thread>
#include <Windows.h>
#include <iostream>
#include <fstream>
#
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


std::vector<char> readBinaryFile(const char* filepath)
{
  // 打开二进制文件
  std::ifstream inputFile(filepath, std::ios::binary); // 以二进制模式打开文件

  // 检查文件是否成功打开
  if (!inputFile)
  {
    std::cerr << "Error opening file: " << filepath << std::endl;
    return {};
  }

  // 获取文件的大小
  inputFile.seekg(0, std::ios::end);
  std::streampos fileSize = inputFile.tellg();
  inputFile.seekg(0, std::ios::beg);

  // 创建一个缓冲区来存储文件数据
  std::vector<char> buffer(fileSize);

  // 读取文件内容到缓冲区
  inputFile.read(buffer.data(), fileSize);

  // 检查读取是否成功
  if (inputFile)
  {
    std::cout << "File read successfully! File size: " << fileSize << " bytes." << std::endl;
  }
  else
  {
    std::cerr << "Error reading file!" << std::endl;
  }

  // 输出读取的内容（这里仅展示前几个字节）
  //std::cout << "First 10 bytes of the file:" << std::endl;
  //for (int i = 0; i < 10 && i < buffer.size(); ++i)
  //{
  //    std::cout << std::hex << (0xFF & static_cast<unsigned char>(buffer[i])) << " ";
  //}
  //std::cout << std::dec << std::endl;

  // 关闭文件
  inputFile.close();
  return buffer;
}
bool convertbmp(void* data, size_t size)
{
  BITMAPFILEHEADER bfh;
  BITMAPINFOHEADER bih;
  memset(&bfh, 0, sizeof(bfh));
  memset(&bih, 0, sizeof(bih));
  bfh.bfType = 0x4D42;
  bfh.bfSize = sizeof(bfh) + sizeof(bih) + size;
  bfh.bfOffBits = sizeof(bfh) + sizeof(bih);
  bih.biSize = sizeof(bih);
  bih.biWidth = 1920;
  bih.biHeight = 1080;
  bih.biPlanes = 1;
  bih.biBitCount = 32;
  bih.biCompression = BI_RGB;
  bih.biSizeImage = size;
  HANDLE f = CreateFileA("screenshot.bmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (f == INVALID_HANDLE_VALUE)
  {
    return false;
  }
  DWORD bytesWritten = 0;
  bool result = WriteFile(f, &bfh, sizeof(bfh), &bytesWritten, NULL);
  result = WriteFile(f, &bih, sizeof(bih), &bytesWritten, NULL);
  result = WriteFile(f, data, size, &bytesWritten, NULL);
  CloseHandle(f);
  return result;
}

int main()
{
  auto buffer = readBinaryFile("D:\\dwm_screenshot.bmp");
  convertbmp(buffer.data(), buffer.size()-44);
  return 0;
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