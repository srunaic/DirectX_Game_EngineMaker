#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

namespace Forge {

class EditorCamera;

// 독립적인 Scene View 렌더 리소스
// EditorUI와 완전 분리됨
class SceneViewRenderer {
public:
  SceneViewRenderer();
  ~SceneViewRenderer();

  // 초기화 (Device, SRV Heap 전달)
  void Initialize(ID3D12Device *device, ID3D12DescriptorHeap *srvHeap);
  void Shutdown();

  // 크기 변경 시 RT 재생성
  void Resize(int width, int height);

  // 렌더링 (카메라 기반)
  void Render(ID3D12GraphicsCommandList *commandList,
              const EditorCamera *camera);

  // ImGui Image용 SRV Handle
  D3D12_GPU_DESCRIPTOR_HANDLE GetSRV() const { return m_SrvHandle; }

  // 크기 조회
  int GetWidth() const { return m_Width; }
  int GetHeight() const { return m_Height; }

  // 리소스 유효 여부
  bool IsValid() const { return m_ColorRT != nullptr; }

private:
  void CreateResources();
  void ReleaseResources();

  ID3D12Device *m_Device = nullptr;
  ID3D12DescriptorHeap *m_SrvHeap = nullptr;

  // Render Target Resources
  Microsoft::WRL::ComPtr<ID3D12Resource> m_ColorRT;
  Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthRT;

  // Descriptor Heaps (RTV/DSV는 자체 보유)
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DsvHeap;

  D3D12_CPU_DESCRIPTOR_HANDLE m_RtvHandle = {};
  D3D12_CPU_DESCRIPTOR_HANDLE m_DsvHandle = {};
  D3D12_GPU_DESCRIPTOR_HANDLE m_SrvHandle = {};

  int m_Width = 0;
  int m_Height = 0;

  // Grid Rendering Resources
  Microsoft::WRL::ComPtr<ID3D12RootSignature> m_GridRootSig;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> m_GridPSO;
  Microsoft::WRL::ComPtr<ID3D12Resource> m_GridVB;
  D3D12_VERTEX_BUFFER_VIEW m_GridVBV = {};
  UINT m_GridVertexCount = 0;

  void CreateGridPSO();
  void CreateGridGeometry();

  // SRV Heap 내 슬롯 인덱스 (고정)
  static const UINT SRV_SLOT = 10;
};

} // namespace Forge
