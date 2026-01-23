#pragma once
#include <Windows.h>
#include <string>

namespace Forge {

    class Window {
    public:
        Window(int width, int height, const std::wstring& title);
        ~Window();

        bool Initialize();
        void Shutdown();
        bool ProcessMessages();

        HWND GetHandle() const { return m_hWnd; }
        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }

    private:
        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

        HWND m_hWnd = nullptr;
        int m_Width;
        int m_Height;
        std::wstring m_Title;
    };

}
