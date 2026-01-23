#pragma once
#include <Windows.h>
#include <d3d12.h>


namespace Forge {

class EditorUI {
public:
  void Initialize(void *windowHandle, ID3D12Device *device, int numFrames,
                  DXGI_FORMAT rtvFormat, ID3D12DescriptorHeap *srvHeap,
                  D3D12_CPU_DESCRIPTOR_HANDLE fontSrvCpu,
                  D3D12_GPU_DESCRIPTOR_HANDLE fontSrvGpu);
  void Shutdown();

  void NewFrame();
  void Draw(ID3D12GraphicsCommandList *commandList);
  void Render(ID3D12GraphicsCommandList *commandList);

private:
  void DrawDockSpace();
  void DrawHierarchy();
  void DrawInspector();
  void DrawContentBrowser();
  void DrawViewport();
};

} // namespace Forge
