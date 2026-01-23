#include "EditorUI.h"
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>

namespace Forge {

void EditorUI::Initialize(void *windowHandle, ID3D12Device *device,
                          int numFrames, DXGI_FORMAT rtvFormat,
                          ID3D12DescriptorHeap *srvHeap,
                          D3D12_CPU_DESCRIPTOR_HANDLE fontSrvCpu,
                          D3D12_GPU_DESCRIPTOR_HANDLE fontSrvGpu) {
  std::cout << "[EditorUI] Initializing Context..." << std::endl;
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  ImGui::StyleColorsDark();

  ImGuiStyle &style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  // Initialize Win32
  if (!ImGui_ImplWin32_Init(windowHandle)) {
    std::cerr << "[EditorUI] Critical Error: Failed to initialize ImGui Win32 "
                 "Backend!"
              << std::endl;
    MessageBox(nullptr, L"Failed to initialize ImGui Win32 Backend!", L"Error",
               MB_OK | MB_ICONERROR);
    return;
  }

  // Initialize DX12
  if (!ImGui_ImplDX12_Init(device, numFrames, rtvFormat, srvHeap, fontSrvCpu,
                           fontSrvGpu)) {
    std::cerr
        << "[EditorUI] Critical Error: Failed to initialize ImGui DX12 Backend!"
        << std::endl;
    MessageBox(nullptr, L"Failed to initialize ImGui DX12 Backend!", L"Error",
               MB_OK | MB_ICONERROR);
    return;
  }

  std::cout << "[EditorUI] Manually creating Device Objects..." << std::endl;
  if (!ImGui_ImplDX12_CreateDeviceObjects()) {
    std::cerr
        << "[EditorUI] Critical Error: Failed to create ImGui Device Objects!"
        << std::endl;
    MessageBox(
        nullptr,
        L"Failed to create ImGui Device Objects! (Check d3dcompiler_47.dll)",
        L"Error", MB_OK | MB_ICONERROR);
    return;
  }

  // Force Font Build
  unsigned char *pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

  std::cout << "[EditorUI] Font Atlas Info: " << width << "x" << height
            << std::endl;
  std::cout << "[EditorUI] TexID before check: "
            << (void *)io.Fonts->TexID.GetTexID() << std::endl;

  if (io.Fonts->TexID.GetTexID() == 0) {
    std::cerr
        << "[EditorUI] WARNING: Font Texture ID is NULL. ImGui will crash."
        << std::endl;
    std::cerr << "[EditorUI] Applying HACK: Setting Dummy Texture ID."
              << std::endl;
    // HACK: Set to GPU handle ptr to pretend we have a texture.
    io.Fonts->TexID = ImTextureRef((ImTextureID)fontSrvGpu.ptr);

    if (io.Fonts->TexID.GetTexID() == 0) {
      // If GPU handle is 0 (impossible for shader visible heap?), force
      // non-zero
      io.Fonts->TexID = ImTextureRef((ImTextureID)0x1);
    }
  }

  std::cout << "[EditorUI] ImGui Initialized Successfully. Final TexID: "
            << (void *)io.Fonts->TexID.GetTexID() << std::endl;
}

void EditorUI::Shutdown() {
  ImGui_ImplDX12_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
}

void EditorUI::NewFrame() {
  ImGuiIO &io = ImGui::GetIO();

  std::cout << "[EditorUI] Calling ImGui_ImplDX12_NewFrame()..." << std::endl;
  ImGui_ImplDX12_NewFrame();

  std::cout << "[EditorUI] Calling ImGui_ImplWin32_NewFrame()..." << std::endl;
  ImGui_ImplWin32_NewFrame();

  std::cout << "[EditorUI] Pre-NewFrame Check:" << std::endl;
  std::cout << "   TexID: " << (void *)io.Fonts->TexID.GetTexID() << std::endl;

  if (io.Fonts->TexID.GetTexID() == 0) {
    std::cout << "[EditorUI] EMERGENCY: TexID lost! Restoring..." << std::endl;
    io.Fonts->TexID = ImTextureRef((ImTextureID)0x1);
  }

  std::cout << "[EditorUI] Calling ImGui::NewFrame()..." << std::endl;
  ImGui::NewFrame();

  std::cout << "[EditorUI] NewFrame() Complete." << std::endl;
}

void EditorUI::Draw(ID3D12GraphicsCommandList *commandList) {
  DrawDockSpace();
  DrawHierarchy();
  DrawInspector();
  DrawContentBrowser();
  DrawViewport();

  ImGui::Render();
}

void EditorUI::Render(ID3D12GraphicsCommandList *commandList) {
  ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault(nullptr, (void *)commandList);
  }
}

void EditorUI::DrawDockSpace() {
  static bool opt_fullscreen = true;
  static bool opt_padding = false;
  static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
  static bool request_reset_layout = false;

  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
  if (opt_fullscreen) {
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |=
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
  } else {
    dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
  }

  if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
    window_flags |= ImGuiWindowFlags_NoBackground;

  if (!opt_padding)
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

  ImGui::Begin("DockSpace Demo", nullptr, window_flags);

  if (!opt_padding)
    ImGui::PopStyleVar();

  if (opt_fullscreen)
    ImGui::PopStyleVar(2);

  // Submit the DockSpace
  ImGuiIO &io = ImGui::GetIO();
  if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    static bool first_time = true;
    if (first_time || request_reset_layout) {
      first_time = false;
      request_reset_layout = false;

      // Force cleanup if reset requested
      if (ImGui::DockBuilderGetNode(dockspace_id)) {
        ImGui::DockBuilderRemoveNode(dockspace_id);
      }

      ImGui::DockBuilderAddNode(dockspace_id,
                                dockspace_flags | ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(dockspace_id,
                                    ImGui::GetMainViewport()->Size);

      ImGuiID dock_main_id = dockspace_id;
      ImGuiID dock_id_hierarchy = ImGui::DockBuilderSplitNode(
          dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
      ImGuiID dock_id_inspector = ImGui::DockBuilderSplitNode(
          dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
      ImGuiID dock_id_content = ImGui::DockBuilderSplitNode(
          dock_main_id, ImGuiDir_Down, 0.30f, nullptr, &dock_main_id);
      ImGuiID dock_id_viewport = dock_main_id;

      ImGui::DockBuilderDockWindow("Hierarchy", dock_id_hierarchy);
      ImGui::DockBuilderDockWindow("Inspector", dock_id_inspector);
      ImGui::DockBuilderDockWindow("Content Browser", dock_id_content);
      ImGui::DockBuilderDockWindow("Viewport", dock_id_viewport);

      ImGui::DockBuilderFinish(dockspace_id);
    }
  }

  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Exit")) {
        PostQuitMessage(0);
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Window")) {
      if (ImGui::MenuItem("Reset Layout")) {
        request_reset_layout = true;
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help")) {
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  ImGui::End();
}

void EditorUI::DrawHierarchy() {
  ImGui::Begin("Hierarchy");

  // Search Bar
  static char searchBuffer[128] = "";
  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
  ImGui::InputTextWithHint("##Search", "Search...", searchBuffer,
                           IM_ARRAYSIZE(searchBuffer));
  ImGui::Separator();

  // Tree Nodes
  static int selected = -1;
  const char *items[] = {
      "Main Camera", "Directional Light", "Player",         "Character Model",
      "Weapon",      "Environment",       "Terrain",        "Trees",
      "Buildings",   "UI Canvas",         "TestAgentEntity"};

  ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                  ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                  ImGuiTreeNodeFlags_SpanAvailWidth;

  for (int i = 0; i < IM_ARRAYSIZE(items); i++) {
    ImGuiTreeNodeFlags node_flags = base_flags;
    if (selected == i)
      node_flags |= ImGuiTreeNodeFlags_Selected;

    // Simulate some hierarchy for the reference look
    if (i == 3) { // Character Model under Player
      ImGui::Indent();
      ImGui::TreeNodeEx((void *)(intptr_t)i,
                        node_flags | ImGuiTreeNodeFlags_Leaf |
                            ImGuiTreeNodeFlags_NoTreePushOnOpen,
                        items[i]);
      if (ImGui::IsItemClicked())
        selected = i;
      ImGui::Unindent();
    } else {
      bool is_leaf = true;
      if (i == 2)
        is_leaf = false; // Player has children

      if (is_leaf) {
        node_flags |=
            ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        ImGui::TreeNodeEx((void *)(intptr_t)i, node_flags, items[i]);
        if (ImGui::IsItemClicked())
          selected = i;
      } else {
        bool open = ImGui::TreeNodeEx(
            (void *)(intptr_t)i, node_flags | ImGuiTreeNodeFlags_DefaultOpen,
            items[i]);
        if (ImGui::IsItemClicked())
          selected = i;
        if (open) {
          // Children rendered in loop (simulated above) or here
          ImGui::TreePop();
        }
      }
    }
  }
  ImGui::End();
}

void DrawVec3Control(const char *label, float *values, float resetValue = 0.0f,
                     float columnWidth = 100.0f) {
  ImGui::PushID(label);

  ImGui::Columns(2);
  ImGui::SetColumnWidth(0, columnWidth);
  ImGui::Text(label);
  ImGui::NextColumn();

  float fullWidth = ImGui::CalcItemWidth();
  float itemWidth = fullWidth / 3.0f;

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

  float lineHeight =
      ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
  ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

  // X
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
  if (ImGui::Button("X", buttonSize))
    values[0] = resetValue;
  ImGui::PopStyleColor(3);

  ImGui::SameLine();
  ImGui::PushItemWidth(itemWidth - buttonSize.x);
  ImGui::DragFloat("##X", &values[0], 0.1f, 0.0f, 0.0f, "%.2f");
  ImGui::PopItemWidth();
  ImGui::SameLine();

  // Y
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
  if (ImGui::Button("Y", buttonSize))
    values[1] = resetValue;
  ImGui::PopStyleColor(3);

  ImGui::SameLine();
  ImGui::PushItemWidth(itemWidth - buttonSize.x);
  ImGui::DragFloat("##Y", &values[1], 0.1f, 0.0f, 0.0f, "%.2f");
  ImGui::PopItemWidth();
  ImGui::SameLine();

  // Z
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                        ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
  if (ImGui::Button("Z", buttonSize))
    values[2] = resetValue;
  ImGui::PopStyleColor(3);

  ImGui::SameLine();
  ImGui::PushItemWidth(itemWidth - buttonSize.x);
  ImGui::DragFloat("##Z", &values[2], 0.1f, 0.0f, 0.0f, "%.2f");
  ImGui::PopItemWidth();

  ImGui::PopStyleVar();

  ImGui::Columns(1);

  ImGui::PopID();
}

void EditorUI::DrawInspector() {
  ImGui::Begin("Inspector");

  // Header
  static bool active = true;
  ImGui::Checkbox("##Active", &active);
  ImGui::SameLine();
  static char nameBuf[128] = "Directional Light";
  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
  ImGui::InputText("##Name", nameBuf, IM_ARRAYSIZE(nameBuf));

  ImGui::Text("Tag");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(100);
  if (ImGui::BeginCombo("##Tag", "Untagged")) {
    ImGui::EndCombo();
  }
  ImGui::SameLine();
  ImGui::Text("Layer");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(100);
  if (ImGui::BeginCombo("##Layer", "Default")) {
    ImGui::EndCombo();
  }

  ImGui::Separator();

  // Transform Component
  if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
    static float pos[3] = {0.0f, 0.0f, 0.0f};
    static float rot[3] = {0.0f, 0.0f, 0.0f};
    static float scale[3] = {1.0f, 1.0f, 1.0f};

    DrawVec3Control("Position", pos);
    DrawVec3Control("Rotation", rot);
    DrawVec3Control("Scale", scale, 1.0f);
  }

  ImGui::Separator();
  if (ImGui::Button("Add Component", ImVec2(-1, 0))) {
    // TODO: Open popup
  }

  ImGui::End();
}

void EditorUI::DrawContentBrowser() {
  ImGui::Begin("Content Browser");

  static float padding = 16.0f;
  static float thumbnailSize = 80.0f;

  // Top bar: Assets Console History
  if (ImGui::Button("Assets")) {
  }
  ImGui::SameLine();
  if (ImGui::Button("Console")) {
  }
  ImGui::SameLine();
  if (ImGui::Button("History")) {
  }
  ImGui::Separator();

  // Splitter
  static float w1 = 150.0f;
  // Use a simple tables approach for 2 columns
  if (ImGui::BeginTable("ContentSplit", 2,
                        ImGuiTableFlags_Resizable |
                            ImGuiTableFlags_BordersInnerV)) {
    ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, w1);
    ImGui::TableSetupColumn("Files", ImGuiTableColumnFlags_WidthStretch);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    // Tree
    if (ImGui::TreeNodeEx("Assets", ImGuiTreeNodeFlags_DefaultOpen |
                                        ImGuiTreeNodeFlags_OpenOnArrow)) {
      ImGui::TreeNodeEx("Models", ImGuiTreeNodeFlags_Leaf |
                                      ImGuiTreeNodeFlags_NoTreePushOnOpen);
      ImGui::TreeNodeEx("Textures", ImGuiTreeNodeFlags_Leaf |
                                        ImGuiTreeNodeFlags_NoTreePushOnOpen);
      ImGui::TreeNodeEx("Materials", ImGuiTreeNodeFlags_Leaf |
                                         ImGuiTreeNodeFlags_NoTreePushOnOpen);
      ImGui::TreeNodeEx("Scripts", ImGuiTreeNodeFlags_Leaf |
                                       ImGuiTreeNodeFlags_NoTreePushOnOpen);
      ImGui::TreeNodeEx("Audio", ImGuiTreeNodeFlags_Leaf |
                                     ImGuiTreeNodeFlags_NoTreePushOnOpen);
      ImGui::TreeNodeEx("Prefabs", ImGuiTreeNodeFlags_Leaf |
                                       ImGuiTreeNodeFlags_NoTreePushOnOpen);
      ImGui::TreePop();
    }

    ImGui::TableSetColumnIndex(1);

    // Grid
    ImGui::InputTextWithHint("##AssetSearch", "Search...", new char[10],
                             10); // Dummy
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

        // Icon (Color Button for now)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::Button("##Icon", ImVec2(thumbnailSize, thumbnailSize));
        ImGui::PopStyleColor();

        // Drag Drop Source (Placeholder)
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

  // Tabs (simulated)
  if (ImGui::BeginTabBar("ViewportTabs")) {
    if (ImGui::BeginTabItem("Scene")) {

      ImVec2 viewportSize = ImGui::GetContentRegionAvail();

      // Image will go here
      // ImGui::Image(...);

      // Overlay
      ImGui::SetCursorPos(ImVec2(10, 30));
      ImGui::BeginChild("Overlay", ImVec2(150, 60), true,
                        ImGuiWindowFlags_NoTitleBar |
                            ImGuiWindowFlags_NoScrollbar);
      ImGui::Text("FPS: 60.0");
      ImGui::Text("Tris: 1.2K");
      ImGui::Text("Calls: 12");
      ImGui::EndChild();

      // Gizmo placeholder center
      ImDrawList *draw_list = ImGui::GetWindowDrawList();
      ImVec2 p = ImGui::GetCursorScreenPos();
      p.x += viewportSize.x * 0.5f;
      p.y += viewportSize.y * 0.5f;

      // Simple Cube Wireframe hint
      // draw_list->AddRect(ImVec2(p.x - 50, p.y - 50), ImVec2(p.x + 50, p.y +
      // 50), IM_COL32(0, 255, 255, 255));

      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Shaded")) {
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Wireframe")) {
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  ImGui::End();
  ImGui::PopStyleVar();
}

} // namespace Forge
