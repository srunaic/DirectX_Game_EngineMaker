#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <vector>
#include <wrl/client.h>


using Microsoft::WRL::ComPtr;

namespace Forge {

class DX12Context {
public:
  DX12Context(void *windowHandle, int width, int height);
  ~DX12Context();

  bool Initialize();
  void CleanUp();

  void BeginFrame();
  void EndFrame();

  ID3D12Device *GetDevice() const { return m_Device.Get(); }
  ID3D12GraphicsCommandList *GetCommandList() const {
    return m_CommandList.Get();
  }

  // ImGui needs these
  ID3D12DescriptorHeap *GetSRVHeap() const { return m_SrvHeap.Get(); }
  D3D12_CPU_DESCRIPTOR_HANDLE GetSRVDescriptorHandleStartCPU() const {
    return m_SrvHeap->GetCPUDescriptorHandleForHeapStart();
  }
  D3D12_GPU_DESCRIPTOR_HANDLE GetSRVDescriptorHandleStartGPU() const {
    return m_SrvHeap->GetGPUDescriptorHandleForHeapStart();
  }

private:
  void CreateRenderTarget();
  void WaitForPreviousFrame();

  void *m_WindowHandle;
  int m_Width;
  int m_Height;

  static const int FrameCount = 2;

  ComPtr<ID3D12Device> m_Device;
  ComPtr<ID3D12CommandQueue> m_CommandQueue;
  ComPtr<IDXGISwapChain3> m_SwapChain;
  ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
  ComPtr<ID3D12DescriptorHeap> m_SrvHeap;
  UINT m_RtvDescriptorSize;
  ComPtr<ID3D12Resource> m_RenderTargets[FrameCount];
  ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
  ComPtr<ID3D12GraphicsCommandList> m_CommandList;
  ComPtr<ID3D12Fence> m_Fence;
  UINT64 m_FenceValue = 0;
  HANDLE m_FenceEvent;
  UINT m_FrameIndex = 0;
};

} // namespace Forge
