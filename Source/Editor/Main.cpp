#include "../Runtime/Core/Window.h"
#include "../Runtime/Renderer/DX12Context.h"
#include "EditorUI.h"
#include <Windows.h>
#include <fstream>
#include <iostream>

// Global Logging Setup
void SetupLogging() {
  FILE *fp;
  // Redirect stdout to log.txt (Overwrite mode)
  freopen_s(&fp, "log.txt", "w", stdout);
  // Redirect stderr to the same log.txt (Append mode)
  freopen_s(&fp, "log.txt", "a", stderr);

  // Set stdout to be unbuffered to ensure logs are written immediately during
  // crashes
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  std::cout << "[System] Logging Initialized in log.txt" << std::endl;
}

int RunGameLoop() {
  std::cout << "[Main] Initializing Wrapper..." << std::endl;

  // 1. Initialize Window
  std::unique_ptr<Forge::Window> window =
      std::make_unique<Forge::Window>(1600, 900, L"DirectXForge Engine");
  if (!window->Initialize()) {
    MessageBox(nullptr, L"Failed to initialize Window", L"Fatal Error",
               MB_OK | MB_ICONERROR);
    return -1;
  }
  std::cout << "[Main] Window Initialized." << std::endl;

  // 2. Initialize DX12
  std::unique_ptr<Forge::DX12Context> renderer =
      std::make_unique<Forge::DX12Context>(
          window->GetHandle(), window->GetWidth(), window->GetHeight());
  if (!renderer->Initialize()) {
    MessageBox(nullptr, L"Failed to initialize DirectX 12", L"Fatal Error",
               MB_OK | MB_ICONERROR);
    return -1;
  }
  std::cout << "[Main] DX12 Context Initialized." << std::endl;

  // 3. Initialize Editor UI
  std::unique_ptr<Forge::EditorUI> editor = std::make_unique<Forge::EditorUI>();
  editor->Initialize(window->GetHandle(), renderer->GetDevice(), 2,
                     DXGI_FORMAT_R8G8B8A8_UNORM, renderer->GetSRVHeap(),
                     renderer->GetSRVDescriptorHandleStartCPU(),
                     renderer->GetSRVDescriptorHandleStartGPU());

  // --- Step 1 Init: Create Scene and Entities ---
  auto scene = std::make_shared<Forge::Scene>();

  auto *cam = scene->CreateEntity("Main Camera");
  auto *light = scene->CreateEntity("Directional Light");
  auto *player = scene->CreateEntity("Player");
  auto *model = scene->CreateEntity("Character Model");
  model->SetParent(player);

  editor->SetActiveScene(scene);
  // ----------------------------------------------

  std::cout << "[Main] Editor UI Initialized." << std::endl;

  // 4. Main Loop
  std::cout << "[Main] Entering Game Loop..." << std::endl;

  // Force a flush
  std::cout << std::flush;

  MSG msg = {};
  while (msg.message != WM_QUIT) {
    // Process Window Messages
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } else {
      // Idle Loop (Game Logic)
      renderer->BeginFrame();

      // UI
      editor->NewFrame();
      editor->Draw(renderer->GetCommandList());
      editor->Render(renderer->GetCommandList());

      renderer->EndFrame();
    }
  }

  std::cout << "[Main] Loop Exited. Shutting down..." << std::endl;
  editor->Shutdown();
  renderer->CleanUp();
  window->Shutdown();

  return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow) {
  SetupLogging();

  __try {
    return RunGameLoop();
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    unsigned long code = GetExceptionCode();
    std::cerr << "[CRASH] Exception Code: 0x" << std::hex << code << std::dec
              << std::endl;

    wchar_t buf[128];
    swprintf_s(buf, L"Crash Detected!\nCode: 0x%X\nSee debug_log.txt", code);
    MessageBox(nullptr, buf, L"Fatal Crash", MB_OK | MB_ICONERROR);
    return -1;
  }
}
