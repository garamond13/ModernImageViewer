module;
#include "framework.h"
#include "resource.h"

export module settings;
import shared;
import config;


export class Settings {
public:
    Settings()
    {
        std::array pages{
            PROPSHEETPAGEW{
                .dwSize{ sizeof(PROPSHEETPAGEW) },
                .dwFlags{ PSP_USETITLE },
                .hInstance{ shared::hinstance },
                .pszTemplate{ MAKEINTRESOURCE(IDD_PROPPAGE_GENERAL) },
                .pszTitle{ L"General" },
                .pfnDlgProc{ general_proc },
                .lParam{ reinterpret_cast<LPARAM>(this) },
            },
            PROPSHEETPAGEW{
                .dwSize{ sizeof(PROPSHEETPAGEW) },
                .dwFlags{ PSP_USETITLE },
                .hInstance{ shared::hinstance },
                .pszTemplate{ MAKEINTRESOURCE(IDD_PROPPAGE_COLOR_MANAGMENT) },
                .pszTitle{ L"Color managment" },
                .pfnDlgProc{ color_managment_procedure },
                .lParam{ reinterpret_cast<LPARAM>(this) },
            },
            PROPSHEETPAGEW{
                .dwSize{ sizeof(PROPSHEETPAGEW) },
                .dwFlags{ PSP_USETITLE },
                .hInstance{ shared::hinstance },
                .pszTemplate{ MAKEINTRESOURCE(IDD_PROPPAGE_SCALING) },
                .pszTitle{ L"Scaling" },
                .pfnDlgProc{ scaling_procedure },
                .lParam{ reinterpret_cast<LPARAM>(this) },
            },
        };
        PROPSHEETHEADERW header{
            .dwSize{ sizeof(PROPSHEETHEADERW) },
            .dwFlags{ PSH_PROPSHEETPAGE | PSH_NOCONTEXTHELP },
            .hwndParent{ shared::hwnd },
            .hInstance{ shared::hinstance },
            .pszCaption{ L"Settings" },
            .nPages{ pages.size() },
            .ppsp{ pages.data() },
        };
        PropertySheetW(&header);
    }

private:
    static LRESULT CALLBACK general_proc(HWND hdlg, UINT uMessage, WPARAM wParam, LPARAM lparam)
    {
        static Settings* settings;
        switch (uMessage) {
        case WM_INITDIALOG:
            settings = reinterpret_cast<Settings*>(lparam);
            return settings->general_wm_initdialog(hdlg);
        case WM_COMMAND:
            PropSheet_Changed(GetParent(hdlg), hdlg); //on any command notification, enable the Apply button
            return settings->general_wm_command(hdlg, wParam);
        case WM_NOTIFY:
            //sent when OK or Apply button pressed
            if (reinterpret_cast<LPNMHDR>(lparam)->code == PSN_APPLY) {
                shared::config.window_width = settings->config.window_width;
                shared::config.window_height = settings->config.window_height;
                shared::config.general = settings->config.general;
                shared::config.background_color = settings->config.background_color;
                shared::config.custom_colors = settings->config.custom_colors;
                shared::config.write();
                settings->is_change_made = false; //reset value
            }
            break;
        }
        return 0;
    }

    LRESULT general_wm_initdialog(HWND hdlg)
    {
        config.general = shared::config.general;
        config.background_color = shared::config.background_color;
        config.custom_colors = shared::config.custom_colors;

        //set checks
        if (shared::config.general & Config::General::fixed_window_dimensions)
            Button_SetCheck(GetDlgItem(hdlg, IDC_FIXED_WINDOW_SIZE), BST_CHECKED);

        //set edit controls
        SetDlgItemTextW(hdlg, IDC_WIDTH, (std::to_wstring(shared::config.window_width)).c_str());
        SetDlgItemTextW(hdlg, IDC_HEIGHT, (std::to_wstring(shared::config.window_height)).c_str());

        return 1;
    }

    LRESULT general_wm_command(HWND hdlg, WPARAM wparam)
    {
        switch (LOWORD(wparam)) {
        case IDC_BACKGROUND_COLOR: {
            CHOOSECOLORW choosecolor{
                .lStructSize{ sizeof(CHOOSECOLORW) },
                .hwndOwner{ hdlg },
                .rgbResult{ shared::config.background_color },
                .lpCustColors{ config.custom_colors.data() },
                .Flags{ CC_FULLOPEN | CC_RGBINIT },
            };
            if (ChooseColorW(&choosecolor) == TRUE)
                config.background_color = choosecolor.rgbResult;
        }
                                 is_change_made = true;
                                 break;
        case IDC_FIXED_WINDOW_SIZE:
            if (config.general & Config::General::fixed_window_dimensions) {
                config.general ^= Config::General::fixed_window_dimensions;
                Button_SetCheck(GetDlgItem(hdlg, IDC_FIXED_WINDOW_SIZE), BST_UNCHECKED);
            }
            else {
                config.general |= Config::General::fixed_window_dimensions;
                Button_SetCheck(GetDlgItem(hdlg, IDC_FIXED_WINDOW_SIZE), BST_CHECKED);
            }
            is_change_made = true;
            break;
        case IDC_WIDTH:
            if (HIWORD(wparam) == EN_CHANGE) {
                constexpr int buffer_size{ 6 };
                wchar_t buffer[buffer_size];
                GetDlgItemTextW(hdlg, IDC_WIDTH, buffer, buffer_size);
                config.window_width = std::wcstol(buffer, nullptr, 10);
                is_change_made = true;
            }
            break;
        case IDC_HEIGHT:
            if (HIWORD(wparam) == EN_CHANGE) {
                constexpr int buffer_size{ 6 };
                wchar_t buffer[buffer_size];
                GetDlgItemTextW(hdlg, IDC_HEIGHT, buffer, buffer_size);
                config.window_height = std::wcstol(buffer, nullptr, 10);
                is_change_made = true;
            }
        }
        return 0;
    }

    static LRESULT CALLBACK color_managment_procedure(HWND hdlg, UINT uMessage, WPARAM wparam, LPARAM lparam)
    {
        static Settings* settings;
        switch (uMessage) {
        case WM_INITDIALOG:
            settings = reinterpret_cast<Settings*>(lparam);
            settings->config.color_managment = shared::config.color_managment;
            return settings->color_managment_wm_initdialog(hdlg);
        case WM_COMMAND:
            PropSheet_Changed(GetParent(hdlg), hdlg); //on any command notification, enable the Apply button
            return settings->color_managment_wm_command(hdlg, wparam, lparam);
        case WM_NOTIFY:
            //sent when OK or Apply button pressed
            if (reinterpret_cast<LPNMHDR>(lparam)->code == PSN_APPLY) {
                shared::config.color_managment = settings->config.color_managment;
                shared::config.write();
                settings->is_change_made = false; //reset value
            }
            break;
        }
        return 0;
    }

    LRESULT color_managment_wm_initdialog(HWND hdlg)
    {
        //set checks
        if (config.color_managment & Config::Color_managment::enable)
            Button_SetCheck(GetDlgItem(hdlg, IDC_ENABLE_COLOR_MANAGMENT), BST_CHECKED);
        if (config.color_managment & Config::Color_managment::blackpointcompensation)
            Button_SetCheck(GetDlgItem(hdlg, IDC_USE_BLACKPOINT_COMPENSATION), BST_CHECKED);
        if (config.color_managment & Config::Color_managment::optimise)
            Button_SetCheck(GetDlgItem(hdlg, IDC_OPTIMISE), BST_CHECKED);

        //color intent
        constexpr std::array intents{
                L"perceptual", //0
                L"relative colormetric", //1
                L"saturation", //2
                L"absolute colormetric", //3
        };
        auto dlg_item{ GetDlgItem(hdlg, IDC_RENDERING_INTENT) };
        for (int i{}; i < intents.size(); ++i)
            SendMessageW(dlg_item, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(intents.at(i)));

        //displays currently set intent, the order of WPARAM values is the as in the array above
        if (config.color_managment & Config::Color_managment::intent_perceptual) {
            SendMessageW(dlg_item, CB_SETCURSEL, 0, 0);
            intent = Config::Color_managment::intent_perceptual;
        }
        if (config.color_managment & Config::Color_managment::intent_relative_colormetric) {
            SendMessageW(dlg_item, CB_SETCURSEL, 1, 0);
            intent = Config::Color_managment::intent_relative_colormetric;
        }
        if (config.color_managment & Config::Color_managment::intent_saturation) {
            SendMessageW(dlg_item, CB_SETCURSEL, 2, 0);
            intent = Config::Color_managment::intent_saturation;
        }
        if (config.color_managment & Config::Color_managment::intent_absolute_colormetric) {
            SendMessageW(dlg_item, CB_SETCURSEL, 3, 0);
            intent = Config::Color_managment::intent_absolute_colormetric;
        }

        return 1;
    }

    LRESULT color_managment_wm_command(HWND hdlg, WPARAM wparam, LPARAM lparam)
    {
        if (HIWORD(wparam) == CBN_SELCHANGE) {
            auto index{ SendMessage((HWND)lparam, (UINT)CB_GETCURSEL, 0, 0) };
            switch (index) {
            case 0:
                config.color_managment ^= intent;
                config.color_managment |= Config::Color_managment::intent_perceptual;
                break;
            case 1:
                config.color_managment ^= intent;
                config.color_managment |= Config::Color_managment::intent_relative_colormetric;
                break;
            case 2:
                config.color_managment ^= intent;
                config.color_managment |= Config::Color_managment::intent_saturation;
                break;
            case 3:
                config.color_managment ^= intent;
                config.color_managment |= Config::Color_managment::intent_absolute_colormetric;
                break;
            }
            is_change_made = true;
        }
        switch (LOWORD(wparam)) {
        case IDC_ENABLE_COLOR_MANAGMENT:
            if (config.color_managment & Config::Color_managment::enable) {
                config.color_managment ^= Config::Color_managment::enable;
                Button_SetCheck(GetDlgItem(hdlg, IDC_ENABLE_COLOR_MANAGMENT), BST_UNCHECKED);
            }
            else {
                config.color_managment |= Config::Color_managment::enable;
                Button_SetCheck(GetDlgItem(hdlg, IDC_ENABLE_COLOR_MANAGMENT), BST_CHECKED);
            }
            is_change_made = true;
            break;
        case IDC_USE_BLACKPOINT_COMPENSATION:
            if (config.color_managment & Config::Color_managment::blackpointcompensation) {
                config.color_managment ^= Config::Color_managment::blackpointcompensation;
                Button_SetCheck(GetDlgItem(hdlg, IDC_USE_BLACKPOINT_COMPENSATION), BST_UNCHECKED);
            }
            else {
                config.color_managment |= Config::Color_managment::blackpointcompensation;
                Button_SetCheck(GetDlgItem(hdlg, IDC_USE_BLACKPOINT_COMPENSATION), BST_CHECKED);
            }
            is_change_made = true;
            break;
        case IDC_OPTIMISE:
            if (config.color_managment & Config::Color_managment::optimise) {
                config.color_managment ^= Config::Color_managment::optimise;
                Button_SetCheck(GetDlgItem(hdlg, IDC_OPTIMISE), BST_UNCHECKED);
            }
            else {
                config.color_managment |= Config::Color_managment::optimise;
                Button_SetCheck(GetDlgItem(hdlg, IDC_OPTIMISE), BST_CHECKED);
            }
            is_change_made = true;
        }
        return 0;
    }

    static LRESULT CALLBACK scaling_procedure(HWND hdlg, UINT uMessage, WPARAM wparam, LPARAM lparam)
    {
        static Settings* settings;
        switch (uMessage) {
        case WM_INITDIALOG:
            settings = reinterpret_cast<Settings*>(lparam);
            settings->config.kernel = shared::config.kernel;
            settings->config.radius = shared::config.radius;
            settings->config.kernel_blur = shared::config.kernel_blur;
            settings->config.param1 = shared::config.param1;
            settings->config.param2 = shared::config.param2;
            settings->config.antiringing = shared::config.antiringing;
            settings->config.blur_radius = shared::config.blur_radius;
            settings->config.blur_sigma = shared::config.blur_sigma;
            return settings->scaling_wm_initdialog(hdlg);
        case WM_COMMAND:
            PropSheet_Changed(GetParent(hdlg), hdlg); //on any command notification, enable the Apply button
            return settings->scaling_wm_command(hdlg, wparam, lparam);
        case WM_NOTIFY:
            //sent when OK or Apply button pressed
            if (reinterpret_cast<LPNMHDR>(lparam)->code == PSN_APPLY) {
                shared::config.kernel = settings->config.kernel;
                shared::config.radius = settings->config.radius;
                shared::config.kernel_blur = settings->config.kernel_blur;
                shared::config.param1 = settings->config.param1;
                shared::config.param2 = settings->config.param2;
                shared::config.antiringing = settings->config.antiringing;
                shared::config.blur_radius = settings->config.blur_radius;
                shared::config.blur_sigma = settings->config.blur_sigma;
                shared::config.write();
                settings->is_change_made = false; //reset value
            }
            break;
        }
        return 0;
    }

    LRESULT scaling_wm_initdialog(HWND hdlg)
    {
        constexpr std::array kernels{
                L"Lanczos", //0
                L"Cosine", //1
                L"Hann", //2
                L"Hamming", //3
                L"Blackman", //4
                L"Kaiser", //5
                L"Welch", //6
                L"Said", //7
                L"BC-Spline", //8
                L"Bicubic", //9
                L"Nearest neighbor", //10
                L"Linear", //11
        };
        auto dlg_item{ GetDlgItem(hdlg, IDC_KERNEL1) };
        for (int i{}; i < kernels.size(); ++i)
            SendMessageW(dlg_item, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(kernels.at(i)));

        //display currently set kernel, the order of WPARAM values is the as in the array above
        if (config.kernel == Config::Kernel::lanczos) {
            SendMessageW(dlg_item, CB_SETCURSEL, 0, 0);
        }
        else if (config.kernel == Config::Kernel::cosine) {
            SendMessageW(dlg_item, CB_SETCURSEL, 1, 0);
        }
        else if (config.kernel == Config::Kernel::hann) {
            SendMessageW(dlg_item, CB_SETCURSEL, 2, 0);
        }
        else if (config.kernel == Config::Kernel::hamming) {
            SendMessageW(dlg_item, CB_SETCURSEL, 3, 0);
        }
        else if (config.kernel == Config::Kernel::blackman) {
            SendMessageW(dlg_item, CB_SETCURSEL, 4, 0);
        }
        else if (config.kernel == Config::Kernel::kaiser) {
            SendMessageW(dlg_item, CB_SETCURSEL, 5, 0);
        }
        else if (config.kernel == Config::Kernel::welch) {
            SendMessageW(dlg_item, CB_SETCURSEL, 6, 0);
        }
        else if (config.kernel == Config::Kernel::said) {
            SendMessageW(dlg_item, CB_SETCURSEL, 7, 0);
        }
        else if (config.kernel == Config::Kernel::bc_spline) {
            SendMessageW(dlg_item, CB_SETCURSEL, 8, 0);
        }
        else if (config.kernel == Config::Kernel::bicubic) {
            SendMessageW(dlg_item, CB_SETCURSEL, 9, 0);
        }
        else if (config.kernel == Config::Kernel::nearest_neighbor) {
            SendMessageW(dlg_item, CB_SETCURSEL, 10, 0);
        }
        else { //linear
            SendMessageW(dlg_item, CB_SETCURSEL, 11, 0);
        }

        //set edit boxes
        SetDlgItemTextW(hdlg, IDC_RADIUS, (std::to_wstring(static_cast<int>(shared::config.radius))).c_str());
        SetDlgItemTextW(hdlg, IDC_KERNEL_BLUR, (std::to_wstring(shared::config.kernel_blur)).c_str());
        SetDlgItemTextW(hdlg, IDC_PARAM1, (std::to_wstring(shared::config.param1)).c_str());
        SetDlgItemTextW(hdlg, IDC_PARAM2, (std::to_wstring(shared::config.param2)).c_str());
        SetDlgItemTextW(hdlg, IDC_ANTIRINGING, (std::to_wstring(shared::config.antiringing)).c_str());
        SetDlgItemTextW(hdlg, IDC_BLUR_RADIUS, (std::to_wstring(shared::config.blur_radius)).c_str());
        SetDlgItemTextW(hdlg, IDC_BLUR_SIGMA, (std::to_wstring(shared::config.blur_sigma)).c_str());

        return 1;
    }

    LRESULT scaling_wm_command(HWND hdlg, WPARAM wparam, LPARAM lparam)
    {
        if (HIWORD(wparam) == CBN_SELCHANGE) {
            auto index{ SendMessageW((HWND)lparam, (UINT)CB_GETCURSEL, 0, 0) };
            switch (index) {
            case 0:
                config.kernel = Config::Kernel::lanczos;
                break;
            case 1:
                config.kernel = Config::Kernel::cosine;
                break;
            case 2:
                config.kernel = Config::Kernel::hann;
                break;
            case 3:
                config.kernel = Config::Kernel::hamming;
                break;
            case 4:
                config.kernel = Config::Kernel::blackman;
                break;
            case 5:
                config.kernel = Config::Kernel::kaiser;
                break;
            case 6:
                config.kernel = Config::Kernel::welch;
                break;
            case 7:
                config.kernel = Config::Kernel::said;
                break;
            case 8:
                config.kernel = Config::Kernel::bc_spline;
                break;
            case 9:
                config.kernel = Config::Kernel::bicubic;
                break;
            case 10:
                config.kernel = Config::Kernel::nearest_neighbor;
                break;
            default:
                config.kernel = 0; //linear
                break;
            }
            is_change_made = true;
        }
        switch (LOWORD(wparam)) {
        case IDC_RADIUS:
            if (HIWORD(wparam) == EN_CHANGE) {
                constexpr int buffer_size{ 6 };
                wchar_t buffer[buffer_size];
                GetDlgItemTextW(hdlg, IDC_RADIUS, buffer, buffer_size);
                config.radius = std::wcstof(buffer, nullptr);
                is_change_made = true;
            }
            break;
        case IDC_KERNEL_BLUR:
            if (HIWORD(wparam) == EN_CHANGE) {
                constexpr int buffer_size{ 6 };
                wchar_t buffer[buffer_size];
                GetDlgItemTextW(hdlg, IDC_KERNEL_BLUR, buffer, buffer_size);
                config.kernel_blur = std::wcstof(buffer, nullptr);
                is_change_made = true;
            }
            break;
        case IDC_PARAM1:
            if (HIWORD(wparam) == EN_CHANGE) {
                constexpr int buffer_size{ 6 };
                wchar_t buffer[buffer_size];
                GetDlgItemTextW(hdlg, IDC_PARAM1, buffer, buffer_size);
                config.param1 = std::wcstof(buffer, nullptr);
                is_change_made = true;
            }
            break;
        case IDC_PARAM2:
            if (HIWORD(wparam) == EN_CHANGE) {
                constexpr int buffer_size{ 6 };
                wchar_t buffer[buffer_size];
                GetDlgItemTextW(hdlg, IDC_PARAM2, buffer, buffer_size);
                config.param2 = std::wcstof(buffer, nullptr);
                is_change_made = true;
            }
            break;
        case IDC_ANTIRINGING:
            if (HIWORD(wparam) == EN_CHANGE) {
                constexpr int buffer_size{ 6 };
                wchar_t buffer[buffer_size];
                GetDlgItemTextW(hdlg, IDC_ANTIRINGING, buffer, buffer_size);
                config.antiringing = std::wcstof(buffer, nullptr);
                is_change_made = true;
            }
            break;
        case IDC_BLUR_RADIUS:
            if (HIWORD(wparam) == EN_CHANGE) {
                constexpr int buffer_size{ 6 };
                wchar_t buffer[buffer_size];
                GetDlgItemTextW(hdlg, IDC_BLUR_RADIUS, buffer, buffer_size);
                config.blur_radius = std::wcstof(buffer, nullptr);
                is_change_made = true;
            }
            break;
        case IDC_BLUR_SIGMA:
            if (HIWORD(wparam) == EN_CHANGE) {
                constexpr int buffer_size{ 6 };
                wchar_t buffer[buffer_size];
                GetDlgItemTextW(hdlg, IDC_BLUR_SIGMA, buffer, buffer_size);
                config.blur_sigma = std::wcstof(buffer, nullptr);
                is_change_made = true;
            }
            break;
        }
        return 0;
    }

    bool is_change_made;
    uint16_t intent;
    Config config;
};
