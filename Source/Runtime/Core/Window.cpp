#include "Window.h"
#include <imgui.h>

// Forward declare logic from backend
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);

namespace Forge {

Window::Window(int width, int height, const std::wstring &title)
    : m_Width(width), m_Height(height), m_Title(title) {}

Window::~Window() { Shutdown(); }

bool Window::Initialize() {
  WNDCLASSEX wc = {0};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.lpszClassName = L"ForgeEngineWindowClass";

  RegisterClassEx(&wc);

  RECT wr = {0, 0, m_Width, m_Height};
  AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

  m_hWnd = CreateWindowEx(0, L"ForgeEngineWindowClass", m_Title.c_str(),
                          WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                          wr.right - wr.left, wr.bottom - wr.top, nullptr,
                          nullptr, GetModuleHandle(nullptr), this);

  if (!m_hWnd)
    return false;

  ShowWindow(m_hWnd, SW_SHOW);
  return true;
}

void Window::Shutdown() {
  if (m_hWnd) {
    DestroyWindow(m_hWnd);
    m_hWnd = nullptr;
  }
  UnregisterClass(L"ForgeEngineWindowClass", GetModuleHandle(nullptr));
}

bool Window::ProcessMessages() {
  MSG msg = {0};
  while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT)
      return false;
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return true;
}

LRESULT CALLBACK Window::WindowProc(HWND hWnd, UINT message, WPARAM wParam,
                                    LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    return true;

  Window *pThis =
      reinterpret_cast<Window *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

  if (message == WM_CREATE) {
    LPCREATESTRUCT pCreate = reinterpret_cast<LPCREATESTRUCT>(lParam);
    pThis = reinterpret_cast<Window *>(pCreate->lpCreateParams);
    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
  }

  switch (message) {
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}
} // namespace Forge
