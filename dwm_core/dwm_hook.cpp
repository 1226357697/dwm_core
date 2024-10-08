#include "dwm_hook.h"
#include <dxgi1_2.h>
#include <d3d11.h>
#include <dxgi.h>
#include <algorithm>
#include <vector>
#include <functional>
#include "importer.h"
#include "dwm.h"
#include "imgui/imgui_impl_dx11.h"
#include "debug.hpp"
#include "fonts.h"


#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)
#pragma comment(lib, "ntdll.lib")

extern PVOID NTAPI RtlAddVectoredExceptionHandler(IN ULONG FirstHandler, IN PVECTORED_EXCEPTION_HANDLER VectoredHandler);
extern ULONG NTAPI RtlRemoveVectoredExceptionHandler(IN PVOID VectoredHandlerHandle);

static void* s_veh_hook_target = nullptr;
static void* s_old_Present_ptr = nullptr;
static com_ptr<IDXGISwapChain> s_dxgi_swapchain = nullptr;
static com_ptr<ID3D11Device> s_d3d_device = nullptr;
static com_ptr<ID3D11DeviceContext> s_d3d_device_context = nullptr;
static com_ptr<ID3D11RenderTargetView> s_main_render_targetview = nullptr;
static com_ptr<ID3D11Texture2D> s_back_buffer = nullptr;
static com_ptr<ID3D11Texture2D> s_anit_capture_texture2D;
static D3D11_TEXTURE2D_DESC s_back_buffer_desc;

static void* s_font_data = font_fang_zheng_zhun_yuan_jianti;
static size_t s_font_size_ = sizeof(font_fang_zheng_zhun_yuan_jianti);


dwm_hook::dwm_hook()
  :veh_handle_(nullptr)
{
}

bool dwm_hook::hook()
{
  quick_import_function("ntdll.dll", RtlGetVersion);

  RTL_OSVERSIONINFOW osversion;
  if (NT_SUCCESS(RtlGetVersion(&osversion)))
  {

    TRACE_V("osverion major: %d buildnumber: %d", osversion.dwMajorVersion, osversion.dwBuildNumber);
    if (osversion.dwMajorVersion == 10 && osversion.dwBuildNumber >= 10240)
    {
      // win10
      TRACE("hook win10 osverion");
      return hook_win10();
    }
    else
    {
      // win 7
      TRACE("hook win7 osverion");
      return hook_win7();
    }
  }
  else
  {
    TRACE_ERR("RtlGetVersion Failed");
  }
  return false;
}

dwm_hook& dwm_hook::instance()
{
  static dwm_hook dwm_hook_instance;
  return dwm_hook_instance;
}

bool dwm_hook::hook_win7()
{
  return false;
}

bool dwm_hook::hook_win10()
{

  quick_import_function("Kernel32.dll", Sleep);
  quick_import_function("dxgi.dll", CreateDXGIFactory);
  quick_import_function("d3d11.dll", D3D11CreateDevice);
  TEMP_GUID(IID_IDXGIFactory, 0x7b7166ec, 0x21c7, 0x44ae, 0xb2, 0x1a, 0xc9, 0xae, 0x32, 0x1a, 0xe3, 0x69);
  TEMP_GUID(IID_IDXGIFactoryDWM8, 0x1DDD77AA, 0x9A4Au, 0x4CC8, 0x9Eu, 0x55, 0x98u, 0xC1u, 0x96u, 0xBAu, 0xFCu, 0x8Fu);
  TEMP_GUID(IID_IDXGIDevice, 0x54ec77fa, 0x1377, 0x44e6, 0x8c, 0x32, 0x88, 0xfd, 0x5f, 0x44, 0xc8, 0x4c);

  HRESULT hr;
  com_ptr<IDXGIFactory> dxgi_factory;
  com_ptr<IDXGIFactoryDWM8> dxgi_factory_dwm8;
  com_ptr<ID3D11Device> d3d_device;
  com_ptr<ID3D11DeviceContext> d3d_device_context;
  com_ptr<IDXGIDevice>dxgi_device;
  com_ptr<IDXGIAdapter>dxgi_adapter;
  com_ptr<IDXGIOutput>dxgi_output;
  com_ptr<IDXGISwapChainDWM8>dxgi_swapchain_dwm8;
  // 创建dwm工厂
  //
  TRACE("CreateDXGIFactory");
  hr = CreateDXGIFactory(IID_IDXGIFactory, &dxgi_factory);
  if (FAILED(hr))
    return false;

  // 创建DX设备
  // 
  TRACE("D3D11CreateDevice");
  UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
  D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0 };
  hr = D3D11CreateDevice(
    NULL, 
    D3D_DRIVER_TYPE_HARDWARE, 
    NULL, 
    createDeviceFlags, 
    featureLevelArray, 
    std::size(featureLevelArray), 
    D3D11_SDK_VERSION, 
    &d3d_device,
    NULL,
    &d3d_device_context
  );
  if (FAILED(hr))
    return false;

  TRACE("QueryInterface IID_IDXGIDevice");
  hr = d3d_device->QueryInterface(IID_IDXGIDevice, &dxgi_device);
  if (FAILED(hr))
    return false;

  TRACE("GetAdapter");
  hr = dxgi_device->GetAdapter(&dxgi_adapter);
  if (FAILED(hr))
    return false;

  TRACE("EnumOutputs");
  hr = dxgi_adapter->EnumOutputs(0, &dxgi_output);
  if (FAILED(hr))
    return false;
  
  // 创建SwapChain
  // 
  TRACE("QueryInterface IID_IDXGIFactoryDWM8");
  hr = dxgi_factory->QueryInterface(IID_IDXGIFactoryDWM8, &dxgi_factory_dwm8);
  if (FAILED(hr))
    return false;

  DXGI_SWAP_CHAIN_DESC1 swapchain_desc {0};
  swapchain_desc.Width = 0;
  swapchain_desc.Height = 0;
  swapchain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  swapchain_desc.Stereo = FALSE;
  swapchain_desc.SampleDesc.Count = 1;
  swapchain_desc.SampleDesc.Quality = 0;
  swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapchain_desc.BufferCount = 2;
  swapchain_desc.Scaling = DXGI_SCALING_STRETCH;
  swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapchain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
  swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  DXGI_SWAP_CHAIN_FULLSCREEN_DESC pSwapChainFullScreenDesc1 {0};
  pSwapChainFullScreenDesc1.Windowed = TRUE;
  TRACE("CreateSwapChainDWM");
  hr = dxgi_factory_dwm8->CreateSwapChainDWM(dxgi_device.Get(), &swapchain_desc, &pSwapChainFullScreenDesc1, dxgi_output.Get(), &dxgi_swapchain_dwm8);
  if (FAILED(hr))
    return false;
  
  // HOOK
  //  Present                  = dxgi_swapchain_dwm8->vtable[8] 
  //  PresentDWM               = dxgi_swapchain_dwm8->vtable[16] 
  //  PresentMultiplaneOverlay = dxgi_swapchain_dwm8->vtable[23] 
  //
  void** vtable = *(void***)(dxgi_swapchain_dwm8.Get());
  TRACE_V("Hook PresentXXX dxgi_swapchain_dwm8:%p dwm8_vtable:%p", dxgi_swapchain_dwm8.Get(), vtable);
  const int try_hook_Present_index[] = {8, 16, 23};
  for (int i = 0; i < std::size(try_hook_Present_index); ++i)
  {
    TRACE_V("veh hook address:%p", vtable[try_hook_Present_index[i]]);
    if (!veh_hook(vtable[try_hook_Present_index[i]]))
      return false;

    int try_count = 5;
    do
    {
      Sleep(40);
    } while (s_old_Present_ptr == nullptr && (--try_count) > 0);

    if(s_old_Present_ptr)
      break;

    veh_unhook();
  }

  TRACE_V("s_old_Present_ptr %p", s_old_Present_ptr);
  return s_old_Present_ptr != NULL;

}

LONG NTAPI dwm_hook::veh_callback(struct _EXCEPTION_POINTERS* ExceptionInfo)
{
  //char buffer[0x1000];
  //sprintf(buffer, "veh callback ExceptionCode: %08X ExceptionAddress: %p", ExceptionInfo->ExceptionRecord->ExceptionCode, ExceptionInfo->ExceptionRecord->ExceptionAddress);

  //TRACE("veh callback ExceptionCode: %08X ExceptionAddress: %p", ExceptionInfo->ExceptionRecord->ExceptionCode, ExceptionInfo->ExceptionRecord->ExceptionAddress);
  if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP
    && ExceptionInfo->ExceptionRecord->ExceptionAddress == s_veh_hook_target)
  {
    // HOOK dwm8swapchain.vtable
    IUnknown* pObject = (IUnknown*)(ExceptionInfo->ContextRecord->Rcx);

    void* vtable = *(void**)(pObject);
    void* cheat_vtable = (void*)VirtualAlloc(0, 0x200, MEM_COMMIT, PAGE_READWRITE);
    memcpy(cheat_vtable, vtable, 0x200);
    *(void**)(pObject) = (void*)cheat_vtable;

    //TRACE("HOOK dwm8swapchain.vtable  swapchain: %p vtable: %p", pObject, vtable);
    for (size_t i = 0; i < 0x200; i++)
    {
      //寻找当前hook的虚函数的指针
      if (((void**)cheat_vtable)[i] == s_veh_hook_target) {
        ((void**)cheat_vtable)[i] = (void*)(&dwm_hook::detours_present_hook);
        s_old_Present_ptr = s_veh_hook_target;
        break;
      }
    }

    ExceptionInfo->ContextRecord->Dr0 = 0;
    ExceptionInfo->ContextRecord->Dr7 &= ~1;

    if (s_old_Present_ptr)
    {
      dwm_hook::instance().veh_unhook();
    }
    return EXCEPTION_CONTINUE_EXECUTION;
  }

  return EXCEPTION_CONTINUE_SEARCH;
}

static void enum_thread(std::function<bool(HANDLE)> cb)
{
  if(!cb)
    return ;

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
          return ;
        }
      }
      break;
    }

    if (processInfo->NextEntryOffset == 0) break;
    processInfo = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(reinterpret_cast<BYTE*>(processInfo) + processInfo->NextEntryOffset);
  }
}

bool dwm_hook::veh_hook(void* address)
{
  TRACE_V("veh hook %p veh_handle_: %p", address, veh_handle_);
  assert(veh_handle_ == nullptr);
  if (veh_handle_)
    return false;

  quick_import_function("ntdll.dll", RtlAddVectoredExceptionHandler);
  quick_import_function("ntdll.dll", NtSetContextThread);
  quick_import_function("ntdll.dll", NtGetContextThread);
  quick_import_function("ntdll.dll", NtSuspendThread);
  quick_import_function("ntdll.dll", NtResumeThread);
  quick_import_function("ntdll.dll", NtOpenThread);

  // 设置全局硬件断点

  veh_handle_ = RtlAddVectoredExceptionHandler(1, &veh_callback);
  if (veh_handle_ == nullptr)
  {
    TRACE_ERR("veh veh_handle_ == nullptr");
    return false;
  }
  s_veh_hook_target = address;

  // 遍历所有进程
  TRACE("veh enum_thread before");
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
        if (NT_SUCCESS(NtGetContextThread(thread, &thread_context)))
        {
          assert(thread_context.Dr6 == 0 && (thread_context.Dr7 & 1) == 0);
          thread_context.Dr0 = reinterpret_cast<decltype(thread_context.Dr0)> (address);
          thread_context.Dr7 |= 1;
          NtSetContextThread(thread, &thread_context);
          // TRACE_V("Dr0: %p Dr7:%p", thread_context.Dr0, thread_context.Dr7);
        }

        NtResumeThread(thread, &thread_status_count);
        CloseHandle(thread);
      }
    }
      return true;
    });

  TRACE("veh enum_thread after");
  return true;
}

bool dwm_hook::veh_unhook()
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
        if (NT_SUCCESS(NtGetContextThread(thread,&thread_context)))
        {
          if (thread_context.Dr0 == reinterpret_cast<decltype(thread_context.Dr0)>( s_veh_hook_target)
            && (thread_context.Dr7 & 1) != 0)
          {
            // remove
            thread_context.Dr0 = 0 ;
            thread_context.Dr7 &= ~1;
            NtSetContextThread(thread, &thread_context);
          }
        }

        NtResumeThread(thread, &thread_status_count);
        CloseHandle(thread);
      }
    }
      return true;
    });

  RtlRemoveVectoredExceptionHandler(veh_handle_);
  veh_handle_ = nullptr;
  s_veh_hook_target = nullptr;

  return false;
}

__int64 __fastcall dwm_hook::detours_present_hook(__int64 a1, __int64 a2, __int64 a3, __int64 a4, __int64 a5, __int64 a6, __int64 a7, __int64 a8)
{

  static bool binit = false;
  IDXGISwapChain* swap_chain = (IDXGISwapChain*)a1;
  if (!binit)
  {
    do
    {
      quick_import_function("User32.dll", EnumDisplaySettingsA);
      HRESULT hr = 0;
      TEMP_GUID(IID_ID3D11Device, 0xdb6f6ddb, 0xac77, 0x4e88, 0x82, 0x53, 0x81, 0x9d, 0xf9, 0xbb, 0xf1, 0x40);
      TEMP_GUID(IID_ID3D11Texture2D, 0x6f15aaf2, 0xd208, 0x4e89, 0x9a, 0xb4, 0x48, 0x95, 0x35, 0xd3, 0x4f, 0x9c);

      com_ptr<ID3D11Device> d3dDevice;
      com_ptr<ID3D11DeviceContext> d3dDeviceContext;
      com_ptr<ID3D11RenderTargetView> main_render_target_view;

      // init commium
      if (!dwm_hook::instance().dwm_render_.init())
      {
        TRACE_ERR("dwm_render_ init failed");
        break;
      }
      TRACE("dwm_render_ init success");

      hr = swap_chain->GetDevice(IID_ID3D11Device, &d3dDevice);
      if (FAILED(hr))
        break;

      hr = swap_chain->GetBuffer(0, IID_ID3D11Texture2D, &s_back_buffer);
      if (FAILED(hr))
        break;

      hr = d3dDevice->CreateRenderTargetView(s_back_buffer.Get(), NULL, &main_render_target_view);
      if (FAILED(hr))
        break;

      s_back_buffer->GetDesc(&s_back_buffer_desc);
      ImGui::CreateContext();
      auto& io = ImGui::GetIO();
      io.DisplaySize.x = s_back_buffer_desc.Width;
      io.DisplaySize.y = s_back_buffer_desc.Height;
      d3dDevice->GetImmediateContext(&d3dDeviceContext);

      if (!ImGui_ImplDX11_Init(d3dDevice.Get(), d3dDeviceContext.Get())) 
        break;

      io.BackendPlatformName = ("dwm");
      DEVMODEA dm;
      dm.dmSize = sizeof(DEVMODEA);
      dm.dmDriverExtra = 0;
      EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
      float fontsize = dm.dmPelsWidth > 2000 ? 16.0f : 13.0f;
      io.Fonts->AddFontFromMemoryTTF(s_font_data, s_font_size_, fontsize, NULL, io.Fonts->GetGlyphRangesChineseFull());

      hr = d3dDevice->CreateTexture2D(&s_back_buffer_desc, 0, s_anit_capture_texture2D.GetAddressOf());
      if (FAILED(hr))
        break;


      s_dxgi_swapchain.Attach(swap_chain);
      s_d3d_device.Attach(d3dDevice.Get());
      s_d3d_device_context.Attach(d3dDeviceContext.Get());
      s_main_render_targetview.Attach(main_render_target_view.Get());

      s_dxgi_swapchain->AddRef();
      s_d3d_device->AddRef();
      s_d3d_device_context->AddRef();
      s_main_render_targetview->AddRef();
      binit = true;

      TRACE("init finished");
    } while (false);
  }

  if(!binit)
  {

    TRACE("init failed");
  }

  if (binit)
  {
    // imgui render
    s_d3d_device_context->CopyResource(s_anit_capture_texture2D.Get(), s_back_buffer.Get());
    ImGui_ImplDX11_NewFrame();
    auto& io = ImGui::GetIO();
    io.DisplaySize.x = s_back_buffer_desc.Width;
    io.DisplaySize.y = s_back_buffer_desc.Height;

    ImGui::NewFrame();
    // render
    dwm_hook::instance().dwm_render_.render();
    ImGui::Render();

    //TRACE_V("RenderTargetView :%p %p s_d3d_device_context:%p" , RenderTargetView, *RenderTargetView, s_d3d_device_context);
    s_d3d_device_context->OMSetRenderTargets(1, s_main_render_targetview.GetAddressOf(), NULL);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    TRACE("detours_present_hook4");
  }
  __int64 ret_val = ((decltype(dwm_hook::detours_present_hook)*)s_old_Present_ptr)(a1, a2, a3, a4, a5, a6, a7, a8);

  if (binit)
  {
    s_d3d_device_context->CopyResource(s_back_buffer.Get(), s_anit_capture_texture2D.Get());
  }

  return ret_val;
}
