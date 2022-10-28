module;
#include "framework.h"
#include "resource.h"

export module window;
import renderer;
import current_folder;
import config;
import shared;
import settings;

namespace {
    //only internaly have to be unique
    constexpr wchar_t class_name[]{ L"$MIV" };

    constexpr DWORD style{ WS_OVERLAPPEDWINDOW };
    constexpr DWORD ex_style{ WS_EX_ACCEPTFILES };
}

export class Window final : public Renderer {
public:
    Window(int cmd_show)
    {
        register_window_class();
        create_window(cmd_show);
    }

    void open_file(const wchar_t* path)
    {
        folder.open(path);
        SetWindowTextW(shared::hwnd, path);
        Renderer::set_image(path);
    }

    bool is_fullscreen;
private:
    void register_window_class()
    {
        WNDCLASSEXW wndclassexw{
            .cbSize{ sizeof(WNDCLASSEXW) },
            .style{ CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS },
            .lpfnWndProc{ window_procedure },
            .hInstance{ shared::hinstance },
            .hIcon{ LoadIconW(shared::hinstance, MAKEINTRESOURCE(IDI_MODERNIMAGEVIEWER)) },
            .hCursor{ LoadCursorW(nullptr, IDC_ARROW) },
            .lpszClassName{ class_name },
        };
        RegisterClassExW(&wndclassexw);
    }

    void create_window(int cmd_show)
    {
        RECT rect{
            .right{ shared::config.window_width },
            .bottom{ shared::config.window_height },
        };
        AdjustWindowRectEx(&rect, style, FALSE, ex_style);
        shared::hwnd = CreateWindowExW(ex_style, class_name, L"Modern Image Viewer", style, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, shared::hinstance, this);
        if (!shared::hwnd)
            throw;
        ShowWindow(shared::hwnd, cmd_show);
    }

    static LRESULT CALLBACK window_procedure(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
    {
        Window* window{ reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)) };
        switch (message) {
            //early initialization of hwnd member and pointer to window (this);
        case WM_NCCREATE:
            window = reinterpret_cast<Window*>(reinterpret_cast<CREATESTRUCT*>(lparam)->lpCreateParams);
            shared::hwnd = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
            return DefWindowProcW(hwnd, message, wparam, lparam);

        case WM_ERASEBKGND:
            return 1;
        case WM_KEYDOWN:
            return window->wm_keydown(message, wparam, lparam);
        case WM_COMMAND:
            return window->wm_command(message, wparam, lparam);

            //set minimum window size
        case WM_GETMINMAXINFO:
            reinterpret_cast<MINMAXINFO*>(lparam)->ptMinTrackSize.x = 200;
            reinterpret_cast<MINMAXINFO*>(lparam)->ptMinTrackSize.y = 150;
            break;

        case WM_SIZE:
            window->Renderer::on_window_resize();
            break;
        case WM_CONTEXTMENU:
            while (ShowCursor(TRUE) < 0); //ensures that we get the cursor with the menu in fullscreen
            window->show_menu();
            break;
        case WM_LBUTTONDBLCLK:
            window->set_fullscreen();
            break;

            //provides the mosuse cursor in fullscren on mousemove
            //without this, after exiting from fullscreen the mosue cursor may stay hidden
        case WM_MOUSEMOVE:
            while (ShowCursor(TRUE) < 0);
            break;
        case WM_NCMOUSEMOVE:
            while (ShowCursor(TRUE) < 0);
            break;

        case WM_SYSKEYDOWN:
            return window->wm_syskeydown(message, wparam, lparam);
        case WM_DROPFILES:
            window->folder.drag_and_drop(reinterpret_cast<HDROP>(wparam));
            window->Renderer::set_image(window->folder.current_file.c_str());
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(hwnd, message, wparam, lparam);
        }
        return 0;
    }

    LRESULT wm_keydown(UINT message, WPARAM wparam, LPARAM lparam)
    {
        switch (wparam) {
        case VK_LEFT:
            previous_file();
            break;
        case VK_RIGHT:
            next_file();
            break;
        case VK_ESCAPE:
            if (is_fullscreen)
                set_fullscreen();
            else
                DestroyWindow(shared::hwnd);
            break;
        default:
            return DefWindowProcW(shared::hwnd, message, wparam, lparam);
        }
        return 0;
    }

    LRESULT wm_command(UINT message, WPARAM wparam, LPARAM lparam)
    {
        switch (LOWORD(wparam)) {
        case ID_MENU_ABOUT:
            DialogBoxParamW(shared::hinstance, MAKEINTRESOURCE(IDD_ABOUTBOX), shared::hwnd, about_procedure, 0);
            break;
        case ID_MENU_OPEN:
            file_open_dialog();
            break;
        case ID_MENU_NEXT:
            next_file();
            break;
        case ID_MENU_PREVIOUS:
            previous_file();
            break;
        case ID_MENU_FULLSCREEN:
            set_fullscreen();
            break;
        case ID_MENU_SETTINGS: {
            Settings settings;
        }
            break;
        case ID_MENU_EXIT:
            DestroyWindow(shared::hwnd);
            break;
        default:
            return DefWindowProcW(shared::hwnd, message, wparam, lparam);
        }
        return 0;
    }

    LRESULT wm_syskeydown(UINT message, WPARAM wParam, LPARAM lParam)
    {
        //alt + enter
        if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
            set_fullscreen();
        else
            return DefWindowProcW(shared::hwnd, message, wParam, lParam);
        return 0;
    }

    static INT_PTR CALLBACK about_procedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message) {
        case WM_INITDIALOG:
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hwnd, LOWORD(wParam));
                return TRUE;
            }
            break;
        }
        return FALSE;
    }

    void show_menu()
    {
        auto hmenu{ LoadMenuW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDR_MENU)) };
        auto sub_menu{ GetSubMenu(hmenu, 0) };
        POINT cursor_pos;
        GetCursorPos(&cursor_pos);
        TrackPopupMenuEx(sub_menu, 0, cursor_pos.x, cursor_pos.y, shared::hwnd, nullptr);
        DestroyMenu(hmenu);
    }

    void set_fullscreen()
    {
        static bool maximized_state;
        static RECT rect;
        if (is_fullscreen) {
            SetWindowLongPtrW(shared::hwnd, GWL_STYLE, style);
            SetWindowPos(shared::hwnd, HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_FRAMECHANGED);
            ShowWindow(shared::hwnd, maximized_state ? SW_MAXIMIZE : SW_NORMAL);
            if (!is_maximized())
                maximized_state = false;
        }
        else {
            if (is_maximized())
                maximized_state = true;
            GetWindowRect(shared::hwnd, &rect);
            SetWindowLongPtrW(shared::hwnd, GWL_STYLE, WS_POPUP);
            SetWindowPos(shared::hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
            ShowWindow(shared::hwnd, SW_MAXIMIZE);
        }
        is_fullscreen = !is_fullscreen;
    }

    //matches the window size with the image size
    void set_window_size()
    {
        using T = double;

        if (is_fullscreen || is_maximized())
            return;
        RECT rect{};

        //get the screen width and height
        const T cx{ static_cast<T>(GetSystemMetrics(SM_CXVIRTUALSCREEN)) };
        const T cy{ static_cast<T>(GetSystemMetrics(SM_CYVIRTUALSCREEN)) };

        //if the image resolution is larger than the screen resolution * 0.9, downsize the window to screen resolution * 0.9 with the aspect ratio of the image
        if (cx / cy > static_cast<T>(width) / static_cast<T>(height)) {
            rect.right = static_cast<T>(width) > cx * 0.9 ? std::lround(static_cast<T>(width) * cy / static_cast<T>(height) * 0.9) : width;
            rect.bottom = static_cast<T>(height) > cy * 0.9 ? std::lround(static_cast<T>(height) * static_cast<T>(rect.right) / static_cast<T>(width)) : height;
        }
        else {
            rect.bottom = static_cast<T>(height) > cy * 0.9 ? std::lround(static_cast<T>(height) * cx / static_cast<T>(width) * 0.9) : height;
            rect.right = static_cast<T>(width) > cx * 0.9 ? std::lround(static_cast<T>(width) * static_cast<T>(rect.bottom) / static_cast<T>(height)) : width;
        }

        AdjustWindowRectEx(&rect, style, FALSE, ex_style);
        SetWindowPos(shared::hwnd, HWND_TOP, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE);
    }

    void file_open_dialog()
    {
        if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) {
            Microsoft::WRL::ComPtr<IFileOpenDialog> file_open_dialog;
            if (SUCCEEDED(CoCreateInstance(__uuidof(FileOpenDialog), nullptr, CLSCTX_ALL, IID_PPV_ARGS(file_open_dialog.ReleaseAndGetAddressOf())))) {
                constexpr std::array<COMDLG_FILTERSPEC, 1> filterspec{
                    { L"All supported", L"*.tif;*.jpeg;*.png;*.bmp" },
                };
                file_open_dialog->SetFileTypes(filterspec.size(), filterspec.data());
                if (SUCCEEDED(file_open_dialog->Show(nullptr))) {
                    Microsoft::WRL::ComPtr<IShellItem> shell_item;
                    file_open_dialog->GetResult(shell_item.ReleaseAndGetAddressOf());
                    wchar_t* path;
                    shell_item->GetDisplayName(SIGDN_FILESYSPATH, &path);
                    open_file(path);
                    CoTaskMemFree(path);
                }
            }
        }
        CoUninitialize();
        if (!(shared::config.general & Config::General::fixed_window_dimensions))
            set_window_size();
    }

    void next_file()
    {
        const auto& path{ folder.get_next_file() };
        if (path.empty())
            return;
        SetWindowTextW(shared::hwnd, path.c_str());
        Renderer::set_image(path.c_str());
        if (!(shared::config.general & Config::General::fixed_window_dimensions))
            set_window_size();
    }

    void previous_file()
    {
        const auto& path{ folder.get_previous_file() };
        if (path.empty())
            return;
        SetWindowTextW(shared::hwnd, path.c_str());
        Renderer::set_image(path.c_str());
        if (!(shared::config.general & Config::General::fixed_window_dimensions))
            set_window_size();
    }

    bool is_maximized()
    {
        return GetWindowStyle(shared::hwnd) & WS_MAXIMIZE && !is_fullscreen;
    }

    Current_folder folder;
};