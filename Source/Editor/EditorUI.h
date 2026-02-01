#pragma once
#include "../Runtime/Scene/Entity.h"
#include "../Runtime/Scene/Scene.h"
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <memory>
#include <string>
#include <vector>

#include "EditorCamera.h"
#include "SceneViewRenderer.h"
#include <imgui.h>

namespace Forge {

class DX12Context;
class Camera;

class EditorUI {
public:
  EditorUI();
  ~EditorUI();

  void SetActiveScene(std::shared_ptr<Scene> scene);

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

  void DrawEntityNode(Entity *entity);

  // State
  std::shared_ptr<Scene> m_ActiveScene;
  Entity *m_SelectedEntity = nullptr;
  Entity *m_RenamingEntity = nullptr;
  EditorCamera m_EditorCamera;

  // Isolated Scene View Renderer
  SceneViewRenderer m_SceneViewRenderer;
};

} // namespace Forge
