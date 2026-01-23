#include "DX12Context.h"
#include <d3dcompiler.h>
#include <iostream>
#include <stdexcept>


#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

namespace Forge {

DX12Context::DX12Context(void *windowHandle, int width, int height)
    : m_WindowHandle(windowHandle), m_Width(width), m_Height(height) {}

DX12Context::~DX12Context() { CleanUp(); }

bool DX12Context::Initialize() {
  HRESULT hr;

  // 1. Enable Debug Layer
#ifdef _DEBUG
  ComPtr<ID3D12Debug> debugController;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
    debugController->EnableDebugLayer();
    std::cout << "[DX12] Debug Layer Enabled" << std::endl;
  }
#endif

  // 2. Create Factory
  ComPtr<IDXGIFactory4> factory;
  if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
    std::cout << "[DX12] Failed to create DXGI Factory" << std::endl;
    return false;
  }

  // 3. Create Device
  if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0,
                               IID_PPV_ARGS(&m_Device)))) {
    std::cout << "[DX12] Failed to create D3D12 Device" << std::endl;
    return false;
  }
  std::cout << "[DX12] Device Created" << std::endl;

  // 4. Create Command Queue
  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  if (FAILED(m_Device->CreateCommandQueue(&queueDesc,
                                          IID_PPV_ARGS(&m_CommandQueue)))) {
    std::cout << "[DX12] Failed to create Command Queue" << std::endl;
    return false;
  }

  // 5. Create Swap Chain
  DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
  swapChainDesc.BufferCount = FrameCount;
  swapChainDesc.Width = m_Width;
  swapChainDesc.Height = m_Height;
  swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapChainDesc.SampleDesc.Count = 1;

  ComPtr<IDXGISwapChain1> swapChain;
  if (FAILED(factory->CreateSwapChainForHwnd(
          m_CommandQueue.Get(), static_cast<HWND>(m_WindowHandle),
          &swapChainDesc, nullptr, nullptr, &swapChain))) {
    std::cout << "[DX12] Failed to create Swap Chain" << std::endl;
    return false;
  }

  swapChain.As(&m_SwapChain);
  m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();
  std::cout << "[DX12] Swap Chain Created" << std::endl;

  // 6. Create Descriptor Heaps
  {
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(m_Device->CreateDescriptorHeap(&rtvHeapDesc,
                                              IID_PPV_ARGS(&m_RtvHeap))))
      return false;
    m_RtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  }

  {
    // Increased to 64 to be safe for ImGui or other resources
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 64;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(m_Device->CreateDescriptorHeap(&srvHeapDesc,
                                              IID_PPV_ARGS(&m_SrvHeap))))
      return false;
  }
  std::cout << "[DX12] Descriptor Heaps Created" << std::endl;

  // 7. Create Frame Resources
  CreateRenderTarget();

  if (FAILED(m_Device->CreateCommandAllocator(
          D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator))))
    return false;

  // 8. Create Command List
  if (FAILED(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                         m_CommandAllocator.Get(), nullptr,
                                         IID_PPV_ARGS(&m_CommandList))))
    return false;
  m_CommandList->Close();

  // 9. Create Synchronization Objects
  if (FAILED(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                   IID_PPV_ARGS(&m_Fence))))
    return false;
  m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  std::cout
      << "[DX12] Synchronization Objects Created. Initialization Complete."
      << std::endl;

  return true;
}

void DX12Context::CleanUp() {
  if (m_CommandQueue && m_Fence) {
    WaitForPreviousFrame();
  }
  CloseHandle(m_FenceEvent);
}

void DX12Context::CreateRenderTarget() {
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
      m_RtvHeap->GetCPUDescriptorHandleForHeapStart();
  for (UINT i = 0; i < FrameCount; i++) {
    m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_RenderTargets[i]));
    m_Device->CreateRenderTargetView(m_RenderTargets[i].Get(), nullptr,
                                     rtvHandle);
    rtvHandle.ptr += m_RtvDescriptorSize;
  }
}

void DX12Context::BeginFrame() {
  m_CommandAllocator->Reset();
  m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);

  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = m_RenderTargets[m_FrameIndex].Get();
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  m_CommandList->ResourceBarrier(1, &barrier);

  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
      m_RtvHeap->GetCPUDescriptorHandleForHeapStart();
  rtvHandle.ptr += (m_FrameIndex * m_RtvDescriptorSize);

  m_CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

  const float clearColor[] = {0.1f, 0.11f, 0.12f, 1.0f};
  m_CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

  ID3D12DescriptorHeap *descriptorHeaps[] = {m_SrvHeap.Get()};
  m_CommandList->SetDescriptorHeaps(1, descriptorHeaps);
}

void DX12Context::EndFrame() {
  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = m_RenderTargets[m_FrameIndex].Get();
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  m_CommandList->ResourceBarrier(1, &barrier);

  m_CommandList->Close();

  ID3D12CommandList *ppCommandLists[] = {m_CommandList.Get()};
  m_CommandQueue->ExecuteCommandLists(1, ppCommandLists);

  m_SwapChain->Present(1, 0);

  WaitForPreviousFrame();
}

void DX12Context::WaitForPreviousFrame() {
  const UINT64 fence = m_FenceValue;
  m_CommandQueue->Signal(m_Fence.Get(), fence);
  m_FenceValue++;

  if (m_Fence->GetCompletedValue() < fence) {
    m_Fence->SetEventOnCompletion(fence, m_FenceEvent);
    WaitForSingleObject(m_FenceEvent, INFINITE);
  }

  m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();
}

} // namespace Forge
