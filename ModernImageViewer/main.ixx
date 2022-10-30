module;
#include "framework.h"

export module main;
import window;
import shared;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, [[maybe_unused]] _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    shared::hinstance = hInstance;
    shared::config.read();
    auto window{ std::make_unique<Window>(nCmdShow) };
    window->Renderer::initialize();

    //open file from windows file explorer
    if (*lpCmdLine != 0) {
        int num_args{ 1 };
        auto args{ CommandLineToArgvW(lpCmdLine, &num_args) };
        window->open_file(args[0]);
    }
        
    //message loop
    MSG msg{};
    while (msg.message != WM_QUIT) {
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        else {
            window->draw_frame();

            //hide mouse cursor in fullscreen
            if (window->is_fullscreen)
                while (ShowCursor(FALSE) >= 0);
            
            WaitMessage();
        }
    }

    return static_cast<int>(msg.wParam);
}