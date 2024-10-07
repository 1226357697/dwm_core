// dwm_comm.cpp : 定义静态库的函数。
//

#include <wrl.h>
#include <dxgi.h>
#include <algorithm>

#include "ntdll.h"
#include "dwm_comm.h"
#include "importer.h"
#include "debug.hpp"

template <typename T>
using com_ptr = Microsoft::WRL::ComPtr<T>;

#define TEMP_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)\
GUID name = {l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8};

typedef struct _dwm_shared_buffer_t {
  ID3D11Device* d3dDevice;
  ID3D11DeviceContext* d3dDeviceContext;
  ID3D11Texture2D* SharedTexture;
  ID3D11Texture2D* LocalTexture;
  IDXGIKeyedMutex* Mutex;
  size_t buffer_size;
  int64_t handle;
  HANDLE shared_handle;
}dwm_shared_buffer_t;

static uint64_t make_handle(HANDLE SharedTextureHandle, UINT DeviceId)
{
  uint64_t handle = (uint64_t)SharedTextureHandle;
  handle <<= sizeof(UINT) * 8;
  handle |= DeviceId;

  return handle;
}
static bool extract_handle(uint64_t handle, HANDLE* SharedTextureHandle, UINT* DeviceId)
{
  if (handle ==0 || SharedTextureHandle == NULL || DeviceId == NULL)
  {
    return false;
  }

  *DeviceId = UINT(handle & (1ULL << sizeof(UINT) * 8) -1);
  *SharedTextureHandle = (HANDLE)(handle >> sizeof(UINT) * 8);
  return true;
}


void* dwm_comm_create()
{
  quick_import_function("d3d11.dll", D3D11CreateDevice);
  quick_import_function("User32.dll", SetWindowLongPtrA);
  quick_import_function("User32.dll", GetWindowLongPtrA);
  quick_import_function("User32.dll", FindWindowA);
  HRESULT hr = 0;
  dwm_shared_buffer_t* dwm_comm = NULL;
  com_ptr < ID3D11Device > d3dDevice = 0;
  com_ptr < ID3D11DeviceContext > d3dDeviceContext = 0;
  com_ptr < IDXGIDevice > dxgiDevice = 0;
  com_ptr < IDXGIAdapter > dxgiAdapter = 0;
  DXGI_ADAPTER_DESC adapter_desc{};
  D3D_FEATURE_LEVEL featureLevel;
  UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT; 
  const D3D_FEATURE_LEVEL featureLevelArray[4] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0 ,D3D_FEATURE_LEVEL_9_3 };
  
  do
  {
    hr = D3D11CreateDevice(
      NULL,
      D3D_DRIVER_TYPE_HARDWARE,
      NULL,
      createDeviceFlags,
      featureLevelArray,
      std::size(featureLevelArray),
      D3D11_SDK_VERSION,
      &d3dDevice,
      &featureLevel,
      &d3dDeviceContext
    );
    if (FAILED(hr))
    {
      TRACE_ERR_V("D3D11CreateDevice %08X", hr);
      break;
    }

    hr = d3dDevice.As(&dxgiDevice);
    if (FAILED(hr))
    {
      TRACE_ERR_V("d3dDevice.As  %08X", hr);
      break;
    }

    hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    if (FAILED(hr))
    {
      TRACE_ERR_V("dxgiDevice->GetAdapter  %08X", hr);
      break;
    }

    hr = dxgiAdapter->GetDesc(&adapter_desc);
    if (FAILED(hr))
    {
      TRACE_ERR_V("dxgiAdapter->GetDesc  %08X", hr);
      break;
    }

    D3D11_TEXTURE2D_DESC share_desc;
    share_desc.Width = 512;
    share_desc.Height = 512;
    share_desc.MipLevels = 1;
    share_desc.ArraySize = 1;
    share_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    share_desc.SampleDesc.Count = 1;
    share_desc.SampleDesc.Quality = 0;
    share_desc.Usage = D3D11_USAGE_DEFAULT;
    share_desc.BindFlags = 40;
    share_desc.CPUAccessFlags = 0;
    share_desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

    com_ptr < ID3D11Texture2D > pSharedTexture = 0;
    hr = d3dDevice->CreateTexture2D(&share_desc, NULL, &pSharedTexture);
    if (FAILED(hr))
    {
      TRACE_ERR_V("d3dDevice->CreateTexture2D  %08X", hr);
      break;
    }

    D3D11_TEXTURE2D_DESC local_desc = share_desc;
    local_desc.Usage = D3D11_USAGE_STAGING;
    local_desc.BindFlags = 0;
    local_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
    local_desc.MiscFlags = 0;
    com_ptr < ID3D11Texture2D > pLocalTexture = 0;
    hr = d3dDevice->CreateTexture2D(&local_desc, NULL, &pLocalTexture);
    if (FAILED(hr))
    {
      TRACE_ERR_V("d3dDevice->CreateTexture2D  %08X", hr);
      break;
    }

    com_ptr < IDXGIResource > pResource = 0;
    hr = pSharedTexture.As(&pResource);
    if (FAILED(hr))
    {
      TRACE_ERR_V("pSharedTexture.As  %08X", hr);
      break;
    }

    HANDLE SharedTextureHandle = 0;
    com_ptr < IDXGIKeyedMutex > pSharedKeyedMutex = 0;
    hr = pResource->GetSharedHandle(&(SharedTextureHandle));
    if (FAILED(hr))
    {
      TRACE_ERR_V("pResource->GetSharedHandle  %08X", hr);
      break;
    }

    hr = pResource.As(&pSharedKeyedMutex);
    if (FAILED(hr))
    {
      TRACE_ERR_V("pResource.As  %08X", hr);
      break;
    }

    uint64_t handle = make_handle(SharedTextureHandle, adapter_desc.DeviceId);
    TRACE_V("handle: %p SharedTextureHandle:%p DeviceId:%p\n", handle, SharedTextureHandle, adapter_desc.DeviceId);
    HWND dwm_window = FindWindowA(("Dwm"), 0);
    if (dwm_window == NULL)
    {
      TRACE_ERR_V("FindWindowA %08X", dwm_window);
      break;
    }

    //printf("handle: %p\n", handle);
    if (SetWindowLongPtrA(dwm_window, GWLP_USERDATA, handle) == 0 && NtCurrentTeb()->LastErrorValue != ERROR_SUCCESS)
    {
      TRACE_ERR_V("SetWindowLongPtrA %08X", GetLastError());
      break;
    }

    dwm_comm = (dwm_shared_buffer_t*)malloc(sizeof(dwm_shared_buffer_t));
    if (dwm_comm == NULL)
      break;

    memset(dwm_comm, 0, sizeof(dwm_shared_buffer_t));

    dwm_comm->buffer_size = share_desc.Width * share_desc.Height * 4;
    dwm_comm->shared_handle = SharedTextureHandle;
    dwm_comm->d3dDevice = d3dDevice.Get();
    dwm_comm->d3dDeviceContext = d3dDeviceContext.Get();
    dwm_comm->Mutex = pSharedKeyedMutex.Get();
    dwm_comm->SharedTexture = pSharedTexture.Get();
    dwm_comm->LocalTexture = pLocalTexture.Get();
    dwm_comm->handle = handle;

    d3dDevice->AddRef();
    d3dDeviceContext->AddRef();
    pSharedKeyedMutex->AddRef();
    pSharedTexture->AddRef();
    pLocalTexture->AddRef();
  } while (false);
  
  return dwm_comm;
}

void* dwm_comm_open()
{
  HRESULT hr = 0;
  int64_t handle = 0;
  HANDLE shared_handle = 0;
  UINT   device_id = 0;
  TEMP_GUID(IID_IDXGIFactory1, 0x770aae78, 0xf26f, 0x4dba, 0xa8, 0x29, 0x25, 0x3c, 0x83, 0xd1, 0xb3, 0x87);
  TEMP_GUID(IID_IDXGIResource, 0x035f3ab4, 0x482e, 0x4e50, 0xb4, 0x1f, 0x8a, 0x7f, 0x8b, 0xd8, 0x96, 0x0b);
  com_ptr< IDXGIFactory1 > pFactory = nullptr;
  com_ptr< IDXGIAdapter1 > pRecommendedAdapter = nullptr;
  com_ptr < IDXGIAdapter1 > pAdapter = nullptr; 
  com_ptr < ID3D11Device > d3dDevice = 0;
  com_ptr < ID3D11DeviceContext > d3dDeviceContext = 0;
  com_ptr < IDXGIResource > pResource = 0;
  com_ptr < ID3D11Texture2D >pSharedTexture = 0;
  com_ptr < ID3D11Texture2D > pLocalTexture = 0;
  com_ptr < IDXGIKeyedMutex > pMutex = 0;
  DXGI_ADAPTER_DESC1 desc; 
  dwm_shared_buffer_t* dwm_comm = NULL;


  quick_import_function("d3d11.dll", D3D11CreateDevice);
  quick_import_function("dxgi.dll", CreateDXGIFactory1);
  quick_import_function("User32.dll", SetWindowLongPtrA);
  quick_import_function("User32.dll", GetWindowLongPtrA);
  quick_import_function("User32.dll", FindWindowA);
  
  do
  {
    handle = GetWindowLongPtrA(FindWindowA("Dwm", 0), GWLP_USERDATA);
    if (handle == 0)
      break;

    if (!extract_handle(handle, &shared_handle, &device_id))
      break;

    hr = CreateDXGIFactory1(IID_IDXGIFactory1, &pFactory);
    if (FAILED(hr))
      break;

    hr = pFactory->EnumAdapters1(0, &pAdapter);
    if (FAILED(hr))
      break;

    hr = pAdapter->GetDesc1(&desc);
    if (FAILED(hr))
      break;

    pRecommendedAdapter = pAdapter;
    com_ptr < IDXGIAdapter1 > tempAdapter;
    UINT index = 0;

    while (pFactory->EnumAdapters1(index, &tempAdapter) != DXGI_ERROR_NOT_FOUND) {
      DXGI_ADAPTER_DESC1 desc;
      tempAdapter->GetDesc1(&desc);
      if (desc.DeviceId == device_id)
      {
        pRecommendedAdapter = tempAdapter;
        break;
      }

      index++;
    }
    if (!pRecommendedAdapter)
      break;

    UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[4] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0 ,D3D_FEATURE_LEVEL_9_3 };
    hr = D3D11CreateDevice(
      pRecommendedAdapter.Get(),
      D3D_DRIVER_TYPE_UNKNOWN,
      NULL,
      createDeviceFlags,
      featureLevelArray,
      std::size(featureLevelArray),
      D3D11_SDK_VERSION,
      &d3dDevice,
      &featureLevel,
      &d3dDeviceContext
    );
    if (FAILED(hr))
      break;

    hr = d3dDevice->OpenSharedResource(shared_handle, IID_IDXGIResource, &pResource);
    if (FAILED(hr))
      break;

    hr = pResource.As(&pSharedTexture);
    if (FAILED(hr))
      break;

    hr = pResource.As(&pMutex);
    if (FAILED(hr))
      break;

    D3D11_TEXTURE2D_DESC share_desc;
    pSharedTexture->GetDesc(&share_desc);

    D3D11_TEXTURE2D_DESC local_desc = share_desc;
    local_desc.Usage = D3D11_USAGE_STAGING;
    local_desc.BindFlags = 0;
    local_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
    local_desc.MiscFlags = 0;
    hr = d3dDevice->CreateTexture2D(&local_desc, NULL, &pLocalTexture);
    if (FAILED(hr))
      break;

    dwm_comm = (dwm_shared_buffer_t*)malloc(sizeof(dwm_shared_buffer_t));
    if (dwm_comm == NULL)
      break;

    dwm_comm->buffer_size = share_desc.Width * share_desc.Height * 4;
    dwm_comm->shared_handle = shared_handle;
    dwm_comm->d3dDevice = d3dDevice.Get();
    dwm_comm->d3dDeviceContext = d3dDeviceContext.Get();
    dwm_comm->Mutex = pMutex.Get();
    dwm_comm->SharedTexture = pSharedTexture.Get();
    dwm_comm->LocalTexture = pLocalTexture.Get();
    dwm_comm->handle = handle;

    d3dDevice->AddRef();
    d3dDeviceContext->AddRef();
    pMutex->AddRef();
    pSharedTexture->AddRef();
    pLocalTexture->AddRef();
    return dwm_comm;

  } while (false);


  return NULL;
}

bool dwm_comm_update_data(void* dwm_comm, void* buffer, size_t buffer_size)
{
  dwm_shared_buffer_t* _dwm_comm = (dwm_shared_buffer_t*)dwm_comm;
  if (dwm_comm == NULL)
  {
    return false;
  }

  if (buffer_size > _dwm_comm->buffer_size)
  {
    return false;
  }

  D3D11_MAPPED_SUBRESOURCE mapped_resource{};
  HRESULT hr = _dwm_comm->d3dDeviceContext->Map(_dwm_comm->LocalTexture, 0, D3D11_MAP_READ_WRITE, 0, &mapped_resource);
  if (FAILED(hr))
    return false;

  if (buffer && buffer_size != 0 && buffer_size <= mapped_resource.DepthPitch)
  {
    memset(mapped_resource.pData, 0, mapped_resource.DepthPitch);
    memcpy(mapped_resource.pData, buffer, buffer_size);
  }
  else
  {
    memset(mapped_resource.pData, 0, mapped_resource.DepthPitch);
  }

  _dwm_comm->d3dDeviceContext->Unmap(_dwm_comm->LocalTexture, 0);

  if (_dwm_comm->Mutex->AcquireSync(0, 1000) != S_OK)
    return false; 

  _dwm_comm->d3dDeviceContext->CopyResource(_dwm_comm->SharedTexture, _dwm_comm->LocalTexture);
  _dwm_comm->Mutex->ReleaseSync(0);
  return false;
}

bool dwm_comm_get_data(void* dwm_comm, void** buffer, size_t* buffer_size)
{
  dwm_shared_buffer_t* _dwm_comm = (dwm_shared_buffer_t*)dwm_comm;
  if (dwm_comm == NULL || buffer == NULL || buffer_size == NULL)
  {
    return false;
  }

  if (FAILED(_dwm_comm->Mutex->AcquireSync(0, 1000)))
  {
    return false;
  }

  _dwm_comm->d3dDeviceContext->CopyResource(_dwm_comm->LocalTexture, _dwm_comm->SharedTexture);
  _dwm_comm->Mutex->ReleaseSync(0);
  D3D11_MAPPED_SUBRESOURCE mapped_resource{};
  HRESULT hr = _dwm_comm->d3dDeviceContext->Map(_dwm_comm->LocalTexture, 0, D3D11_MAP_READ_WRITE, 0, &mapped_resource);
  if (FAILED(hr)) 
    return false; 

  void* buffer_result = *buffer;
  if (*buffer_size < mapped_resource.DepthPitch)
  {
    buffer_result = realloc(buffer_result, mapped_resource.DepthPitch);
    if (buffer_result == NULL)
      return false;
  }

  memset(buffer_result, 0, mapped_resource.DepthPitch);
  memcpy(buffer_result, mapped_resource.pData, mapped_resource.DepthPitch);
  _dwm_comm->d3dDeviceContext->Unmap(_dwm_comm->LocalTexture, 0);

  *buffer = buffer_result;
  *buffer_size = mapped_resource.DepthPitch;
  return true;
}

void dwm_comm_close(void* dwm_comm)
{
  dwm_shared_buffer_t* _dwm_comm = (dwm_shared_buffer_t*)dwm_comm;
  if(dwm_comm == NULL)
    return ;

  _dwm_comm->LocalTexture->Release();
  _dwm_comm->Mutex->Release();
  _dwm_comm->SharedTexture->Release();
  _dwm_comm->d3dDeviceContext->Release();
  _dwm_comm->d3dDevice->Release();
}

void dwm_comm_free(void* dwm_comm)
{
  dwm_shared_buffer_t* _dwm_comm = (dwm_shared_buffer_t*)dwm_comm;
  if (dwm_comm == NULL)
    return;
  dwm_comm_close(dwm_comm);
  free(dwm_comm);
}
