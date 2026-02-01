#include "EditorUI.h"
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>

namespace Forge {

EditorUI::EditorUI() = default;
EditorUI::~EditorUI() = default;

void EditorUI::SetActiveScene(std::shared_ptr<Scene> scene) {
  m_ActiveScene = scene;
}

void EditorUI::Initialize(void *windowHandle, ID3D12Device *device,
                          int numFrames, DXGI_FORMAT rtvFormat,
                          ID3D12DescriptorHeap *srvHeap,
                          D3D12_CPU_DESCRIPTOR_HANDLE fontSrvCpu,
                          D3D12_GPU_DESCRIPTOR_HANDLE fontSrvGpu) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  ImGui::StyleColorsDark();

  ImGui_ImplWin32_Init(windowHandle);
  ImGui_ImplDX12_Init(device, numFrames, rtvFormat, srvHeap, fontSrvCpu,
                      fontSrvGpu);

  // Fix: Re-enable RendererHasTextures and build font atlas
  io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
  unsigned char *pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

  // Initialize isolated Scene View Renderer
  m_SceneViewRenderer.Initialize(device, srvHeap);
}

void EditorUI::Shutdown() {
  m_SceneViewRenderer.Shutdown();
  ImGui_ImplDX12_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
}

void EditorUI::NewFrame() {
  ImGui_ImplDX12_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
}

void EditorUI::Draw(ID3D12GraphicsCommandList *commandList) {
  // Render Scene View (isolated from UI) - PHASE 2: pass camera
  m_SceneViewRenderer.Render(commandList, &m_EditorCamera);

  // Draw Editor UI panels
  DrawDockSpace();
  DrawHierarchy();
  DrawInspector();
  DrawContentBrowser();
  DrawViewport();

  ImGui::Render();
}

void EditorUI::Render(ID3D12GraphicsCommandList *commandList) {
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

  ImGuiIO &io = ImGui::GetIO();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault(nullptr, (void *)commandList);
  }
}

void EditorUI::DrawDockSpace() {
  ImGuiWindowFlags windowFlags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
  const ImGuiViewport *viewport = ImGui::GetMainViewport();

  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  windowFlags |=
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("DockSpace", nullptr, windowFlags);
  ImGui::PopStyleVar(3);

  ImGuiID dockSpaceId = ImGui::GetID("MainDockSpace");

  static bool firstTime = true;
  if (firstTime) {
    firstTime = false;
    ImGui::DockBuilderRemoveNode(dockSpaceId);
    ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockSpaceId, viewport->WorkSize);

    ImGuiID dockMain = dockSpaceId;
    ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left,
                                                   0.15f, nullptr, &dockMain);
    ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right,
                                                    0.20f, nullptr, &dockMain);
    ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down,
                                                     0.25f, nullptr, &dockMain);

    ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
    ImGui::DockBuilderDockWindow("Viewport", dockMain);
    ImGui::DockBuilderDockWindow("Inspector", dockRight);
    ImGui::DockBuilderDockWindow("Content Browser", dockBottom);

    ImGui::DockBuilderFinish(dockSpaceId);
  }

  ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

  // Menu Bar
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
      }
      if (ImGui::MenuItem("Open Scene", "Ctrl+O")) {
      }
      if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Exit", "Alt+F4")) {
        PostQuitMessage(0);
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
      }
      if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Window")) {
      ImGui::MenuItem("Hierarchy");
      ImGui::MenuItem("Inspector");
      ImGui::MenuItem("Content Browser");
      ImGui::MenuItem("Viewport");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("About")) {
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  ImGui::End();
}

void EditorUI::DrawHierarchy() {
  ImGui::Begin("Hierarchy");

  if (ImGui::Button("Create Empty")) {
    if (m_ActiveScene) {
      m_ActiveScene->CreateEntity();
    }
  }

  ImGui::Separator();

  if (m_ActiveScene) {
    for (Entity *entity : m_ActiveScene->GetRootEntities()) {
      DrawEntityNode(entity);
    }
  }

  // Deselect on empty space click
  if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) {
    m_SelectedEntity = nullptr;
  }

  ImGui::End();
}

void EditorUI::DrawEntityNode(Entity *entity) {
  if (!entity)
    return;

  ImGuiTreeNodeFlags flags =
      ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

  bool hasChildren = !entity->GetChildren().empty();
  if (!hasChildren) {
    flags |= ImGuiTreeNodeFlags_Leaf;
  }

  if (m_SelectedEntity == entity) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }

  // Renaming Mode
  bool isRenaming = (m_RenamingEntity == entity);

  ImGui::PushID((int)entity->GetID());

  if (isRenaming) {
    static char renameBuf[128];
    if (ImGui::IsWindowAppearing()) {
      strncpy_s(renameBuf, entity->GetName().c_str(), sizeof(renameBuf));
    }
    ImGui::SetKeyboardFocusHere();
    if (ImGui::InputText("##Rename", renameBuf, sizeof(renameBuf),
                         ImGuiInputTextFlags_EnterReturnsTrue |
                             ImGuiInputTextFlags_AutoSelectAll)) {
      entity->SetName(renameBuf);
      m_RenamingEntity = nullptr;
    }
    if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0)) {
      m_RenamingEntity = nullptr;
    }
  } else {
    bool opened = ImGui::TreeNodeEx(entity->GetName().c_str(), flags);

    // Selection
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
      m_SelectedEntity = entity;
    }

    // Double Click -> Rename
    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
      m_RenamingEntity = entity;
    }

    // Context Menu
    if (ImGui::BeginPopupContextItem()) {
      if (ImGui::MenuItem("Rename")) {
        m_RenamingEntity = entity;
      }
      if (ImGui::MenuItem("Delete")) {
        if (m_ActiveScene) {
          if (m_SelectedEntity == entity) {
            m_SelectedEntity = nullptr;
          }
          m_ActiveScene->DestroyEntity(entity);
          ImGui::EndPopup();
          ImGui::PopID();
          if (opened)
            ImGui::TreePop();
          return;
        }
      }
      ImGui::EndPopup();
    }

    // F2 to Rename
    if (m_SelectedEntity == entity && ImGui::IsKeyPressed(ImGuiKey_F2)) {
      m_RenamingEntity = entity;
    }
    // Delete Key
    if (m_SelectedEntity == entity && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
      if (m_ActiveScene) {
        m_ActiveScene->DestroyEntity(entity);
        m_SelectedEntity = nullptr;
        ImGui::PopID();
        if (opened)
          ImGui::TreePop();
        return;
      }
    }

    if (opened) {
      for (Entity *child : entity->GetChildren()) {
        DrawEntityNode(child);
      }
      ImGui::TreePop();
    }
  }

  ImGui::PopID();
}

void EditorUI::DrawInspector() {
  ImGui::Begin("Inspector");

  if (m_SelectedEntity) {
    // Name
    char nameBuf[128];
    strncpy_s(nameBuf, m_SelectedEntity->GetName().c_str(), sizeof(nameBuf));
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
      m_SelectedEntity->SetName(nameBuf);
    }

    ImGui::Separator();

    // Transform
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
      TransformComponent &transform = m_SelectedEntity->GetTransform();
      ImGui::DragFloat3("Position", &transform.Position.x, 0.1f);
      ImGui::DragFloat3("Rotation", &transform.Rotation.x, 0.1f);
      ImGui::DragFloat3("Scale", &transform.Scale.x, 0.1f);
    }
  } else {
    ImGui::Text("No Entity Selected");
  }

  ImGui::End();
}

void EditorUI::DrawContentBrowser() {
  ImGui::Begin("Content Browser");

  if (ImGui::BeginTabBar("ContentTabs")) {
    if (ImGui::BeginTabItem("Assets")) {
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Console")) {
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("History")) {
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  float thumbnailSize = 64.0f;
  float padding = 8.0f;

  if (ImGui::BeginTable("ContentBrowserTable", 2,
                        ImGuiTableFlags_Resizable |
                            ImGuiTableFlags_BordersInnerV)) {
    ImGui::TableSetupColumn("Tree", ImGuiTableColumnFlags_WidthFixed, 150.0f);
    ImGui::TableSetupColumn("Grid", ImGuiTableColumnFlags_WidthStretch);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    // Tree
    if (ImGui::TreeNode("Assets")) {
      ImGui::TreeNodeEx("Models", ImGuiTreeNodeFlags_Leaf);
      ImGui::TreePop();
      ImGui::TreeNodeEx("Textures", ImGuiTreeNodeFlags_Leaf);
      ImGui::TreePop();
      ImGui::TreeNodeEx("Materials", ImGuiTreeNodeFlags_Leaf);
      ImGui::TreePop();
      ImGui::TreeNodeEx("Scripts", ImGuiTreeNodeFlags_Leaf);
      ImGui::TreePop();
      ImGui::TreeNodeEx("Audio", ImGuiTreeNodeFlags_Leaf);
      ImGui::TreePop();
      ImGui::TreeNodeEx("Prefabs", ImGuiTreeNodeFlags_Leaf);
      ImGui::TreePop();
      ImGui::TreePop();
    }

    ImGui::TableSetColumnIndex(1);

    // Grid
    static char assetSearchBuf[64] = "";
    ImGui::InputTextWithHint("##AssetSearch", "Search...", assetSearchBuf,
                             IM_ARRAYSIZE(assetSearchBuf));
    ImGui::Separator();

    float cellSize = thumbnailSize + padding;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = (int)(panelWidth / cellSize);
    if (columnCount < 1)
      columnCount = 1;

    if (ImGui::BeginTable("AssetGrid", columnCount)) {
      const char *assets[] = {"Character_01.fbx", "grass_texture.png",
                              "metal.mat",        "PlayerController.cs",
                              "Jump.wav",         "MainScene.json"};

      for (int i = 0; i < 6; i++) {
        ImGui::TableNextColumn();
        ImGui::PushID(i);

        // Icon
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::Button("##Icon", ImVec2(thumbnailSize, thumbnailSize));
        ImGui::PopStyleColor();

        // Drag Drop Source
        if (ImGui::BeginDragDropSource()) {
          ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", &i, sizeof(int));
          ImGui::EndDragDropSource();
        }

        ImGui::TextWrapped(assets[i]);

        ImGui::PopID();
      }
      ImGui::EndTable();
    }

    ImGui::EndTable();
  }

  ImGui::End();
}

void EditorUI::DrawViewport() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::Begin("Viewport");

  // Resize Scene View RT if needed
  ImVec2 viewportSize = ImGui::GetContentRegionAvail();
  if (viewportSize.x > 0 && viewportSize.y > 0) {
    m_SceneViewRenderer.Resize((int)viewportSize.x, (int)viewportSize.y);
  }

  // Display Scene View RT as Image
  if (m_SceneViewRenderer.IsValid()) {
    ImGui::Image((ImTextureID)m_SceneViewRenderer.GetSRV().ptr, viewportSize);
  }

  // Input Isolation [GLOBAL RULE] - PHASE 3
  if (ImGui::IsWindowHovered()) {
    float wheel = ImGui::GetIO().MouseWheel;
    if (wheel != 0.0f) {
      m_EditorCamera.Zoom(wheel);
    }
  }

  // Overlay
  ImGui::SetCursorPos(ImVec2(10, 30));
  ImGui::BeginChild("Overlay", ImVec2(180, 50), true,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
  ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
  ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Zoom Dist: %.2f",
                     m_EditorCamera.GetDistance());
  ImGui::EndChild();

  ImGui::End();
  ImGui::PopStyleVar();
}

} // namespace Forge
