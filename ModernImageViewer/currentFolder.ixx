module;
#include "framework.h"

export module current_folder;

export class Current_folder {
public:
    const auto& get_next_file() noexcept
    {
        for (size_t i{}; i < file_paths.size(); ++i) {
            if (!file_paths[i].compare(current_file) && i + 1 < file_paths.size()) {
                current_file = file_paths[i + 1];
                return current_file;
            }
        }
        return current_file;
    }

    const auto& get_previous_file() noexcept
    {
        for (size_t i{}; i < file_paths.size(); ++i) {
            if (!file_paths[i].compare(current_file) && i > 0) {
                current_file = file_paths[i - 1];
                return current_file;
            }
        }
        return current_file;
    }

    //if only one file is dropped map entire folder
    //if multiple files are dropped map only those files
    void drag_and_drop(HDROP hdrop)
    {
        wchar_t path[MAX_PATH];
        auto count{ DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0) };
        if (count == 1) {
            DragQueryFileW(hdrop, 0, path, MAX_PATH);
            open(path);
        }
        else {
            file_paths.resize(0);
            for (UINT i{}; i < count; ++i)
                if (DragQueryFileW(hdrop, i, path, MAX_PATH) > 0)
                    file_paths.push_back(path);
            current_file = file_paths[0];
        }
        DragFinish(hdrop);
    }

    void open(std::filesystem::path path)
    {
        file_paths.resize(0);
        current_file = path;
        path.replace_filename(L"*");
        WIN32_FIND_DATA find_data;
        HANDLE find_file{ FindFirstFileExW(path.c_str(), FindExInfoBasic, &find_data, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH) };

        //the first outputed value will be trash, so we ignore it
        FindNextFileW(find_file, &find_data);

        while (FindNextFileW(find_file, &find_data) != 0) {
            path.replace_filename(find_data.cFileName);
            if (PathMatchSpecExW(path.c_str(), L"*.tif;*.jpg;*.png;*.bmp", PMSF_MULTIPLE) == S_OK)
                file_paths.push_back(path);
        }
    }

    std::filesystem::path current_file;
private:
    std::vector<std::filesystem::path> file_paths;
};
