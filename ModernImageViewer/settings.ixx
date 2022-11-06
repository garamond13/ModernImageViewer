module;
#include "framework.h"
#include "resource.h"

export module settings;
import shared;
import config;


export class Settings : Config {
public:
    Settings()
    {
        std::array<PROPSHEETPAGEW, 2> pages{
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
                shared::config.window_width = settings->window_width;
                shared::config.window_height = settings->window_height;
                shared::config.general = settings->general;
                shared::config.background_color = settings->background_color;
                shared::config.custom_colors = settings->custom_colors;
                shared::config.write();
                settings->is_change_made = false; //reset value
            }
            break;
        }
        return 0;
    }

    LRESULT general_wm_initdialog(HWND hdlg)
    {
        general = shared::config.general;
        background_color = shared::config.background_color;
        custom_colors = shared::config.custom_colors;
        
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
                .lpCustColors{ custom_colors.data() },
                .Flags{ CC_FULLOPEN | CC_RGBINIT },
            };
            if (ChooseColorW(&choosecolor) == TRUE)
                background_color = choosecolor.rgbResult;
        }
            is_change_made = true;
            break;
        case IDC_FIXED_WINDOW_SIZE:
            if (general & Config::General::fixed_window_dimensions) {
                general ^= Config::General::fixed_window_dimensions;
                Button_SetCheck(GetDlgItem(hdlg, IDC_FIXED_WINDOW_SIZE), BST_UNCHECKED);
            }
            else {
                general |= Config::General::fixed_window_dimensions;
                Button_SetCheck(GetDlgItem(hdlg, IDC_FIXED_WINDOW_SIZE), BST_CHECKED);
            }
            is_change_made = true;
            break;
        case IDC_WIDTH:
            if (HIWORD(wparam) == EN_CHANGE) {
                constexpr int buffer_size{ 6 };
                wchar_t buffer[buffer_size];
                GetDlgItemTextW(hdlg, IDC_WIDTH, buffer, buffer_size);
                window_width = std::wcstol(buffer, nullptr, 10);
                is_change_made = true;
            }
            break;
        case IDC_HEIGHT:
            if (HIWORD(wparam) == EN_CHANGE) {
                constexpr int buffer_size{ 6 };
                wchar_t buffer[buffer_size];
                GetDlgItemTextW(hdlg, IDC_HEIGHT, buffer, buffer_size);
                window_height = std::wcstol(buffer, nullptr, 10);
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
            settings->color_managment = shared::config.color_managment;
            return settings->color_managment_wm_initdialog(hdlg);
        case WM_COMMAND:
            PropSheet_Changed(GetParent(hdlg), hdlg); //on any command notification, enable the Apply button
            return settings->color_managment_wm_command(hdlg, wparam, lparam);
        case WM_NOTIFY:
            //sent when OK or Apply button pressed
            if (reinterpret_cast<LPNMHDR>(lparam)->code == PSN_APPLY) {
                shared::config.color_managment = settings->color_managment;
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
        if (color_managment & Config::Color_managment::enable)
            Button_SetCheck(GetDlgItem(hdlg, IDC_ENABLE_COLOR_MANAGMENT), BST_CHECKED);
        if (color_managment & Config::Color_managment::blackpointcompensation)
            Button_SetCheck(GetDlgItem(hdlg, IDC_USE_BLACKPOINT_COMPENSATION), BST_CHECKED);
        if (color_managment & Config::Color_managment::optimise)
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
        if (color_managment & Config::Color_managment::intent_perceptual) {
            SendMessageW(dlg_item, CB_SETCURSEL, 0, 0);
            intent = Config::Color_managment::intent_perceptual;
        }
        if (color_managment & Config::Color_managment::intent_relative_colormetric) {
            SendMessageW(dlg_item, CB_SETCURSEL, 1, 0);
            intent = Config::Color_managment::intent_relative_colormetric;
        }
        if (color_managment & Config::Color_managment::intent_saturation) {
            SendMessageW(dlg_item, CB_SETCURSEL, 2, 0);
            intent = Config::Color_managment::intent_saturation;
        }
        if (color_managment & Config::Color_managment::intent_absolute_colormetric) {
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
                color_managment ^= intent;
                color_managment |= Config::Color_managment::intent_perceptual;
                break;
            case 1:
                color_managment ^= intent;
                color_managment |= Config::Color_managment::intent_relative_colormetric;
                break;
            case 2:
                color_managment ^= intent;
                color_managment |= Config::Color_managment::intent_saturation;
                break;
            case 3:
                color_managment ^= intent;
                color_managment |= Config::Color_managment::intent_absolute_colormetric;
                break;
            }
            is_change_made = true;
        }
        switch (LOWORD(wparam)) {
        case IDC_ENABLE_COLOR_MANAGMENT:
            if (color_managment & Config::Color_managment::enable) {
                color_managment ^= Config::Color_managment::enable;
                Button_SetCheck(GetDlgItem(hdlg, IDC_ENABLE_COLOR_MANAGMENT), BST_UNCHECKED);
            }
            else {
                color_managment |= Config::Color_managment::enable;
                Button_SetCheck(GetDlgItem(hdlg, IDC_ENABLE_COLOR_MANAGMENT), BST_CHECKED);
            }
            is_change_made = true;
            break;
        case IDC_USE_BLACKPOINT_COMPENSATION:
            if (color_managment & Config::Color_managment::blackpointcompensation) {
                color_managment ^= Config::Color_managment::blackpointcompensation;
                Button_SetCheck(GetDlgItem(hdlg, IDC_USE_BLACKPOINT_COMPENSATION), BST_UNCHECKED);
            }
            else {
                color_managment |= Config::Color_managment::blackpointcompensation;
                Button_SetCheck(GetDlgItem(hdlg, IDC_USE_BLACKPOINT_COMPENSATION), BST_CHECKED);
            }
            is_change_made = true;
            break;
        case IDC_OPTIMISE:
            if (color_managment & Config::Color_managment::optimise) {
                color_managment ^= Config::Color_managment::optimise;
                Button_SetCheck(GetDlgItem(hdlg, IDC_OPTIMISE), BST_UNCHECKED);
            }
            else {
                color_managment |= Config::Color_managment::optimise;
                Button_SetCheck(GetDlgItem(hdlg, IDC_OPTIMISE), BST_CHECKED);
            }
            is_change_made = true;
        }
        return 0;
    }

    bool is_change_made;
    uint16_t intent;
};
