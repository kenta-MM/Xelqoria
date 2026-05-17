#include "Win32FileDialog.h"

#include <Windows.h>
#include <array>
#include <commdlg.h>
#include <ShlObj_core.h>
#include <shtypes.h>
#include <string>

namespace Xelqoria::Platform::Win32
{
    namespace
    {
        [[nodiscard]] HWND ToHwnd(NativeWindowHandle handle)
        {
            return static_cast<HWND>(handle);
        }

        [[nodiscard]] std::wstring BuildFilterString(const std::vector<FileDialogFilter>& filters)
        {
            std::wstring filterString{};
            for (const FileDialogFilter& filter : filters)
            {
                filterString.append(filter.displayName);
                filterString.push_back(L'\0');
                filterString.append(filter.pattern);
                filterString.push_back(L'\0');
            }

            if (filterString.empty())
            {
                filterString.append(L"All Files (*.*)");
                filterString.push_back(L'\0');
                filterString.append(L"*.*");
                filterString.push_back(L'\0');
            }

            filterString.push_back(L'\0');
            return filterString;
        }

        [[nodiscard]] std::optional<std::filesystem::path> ShowFileDialog(
            const FileDialogOptions& options,
            bool isSaveDialog)
        {
            std::array<wchar_t, MAX_PATH> filePath{};
            const std::wstring filterString = BuildFilterString(options.filters);

            OPENFILENAMEW openFileName{};
            openFileName.lStructSize = sizeof(openFileName);
            openFileName.hwndOwner = ToHwnd(options.ownerWindow);
            openFileName.lpstrTitle = options.title.empty() ? nullptr : options.title.c_str();
            openFileName.lpstrFilter = filterString.c_str();
            openFileName.lpstrFile = filePath.data();
            openFileName.nMaxFile = static_cast<DWORD>(filePath.size());
            openFileName.lpstrInitialDir =
                options.initialDirectory.empty() ? nullptr : options.initialDirectory.c_str();
            openFileName.Flags = OFN_PATHMUSTEXIST | (isSaveDialog ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);

            const BOOL succeeded = isSaveDialog
                ? GetSaveFileNameW(&openFileName)
                : GetOpenFileNameW(&openFileName);
            if (FALSE == succeeded)
            {
                return std::nullopt;
            }

            return std::filesystem::path(filePath.data());
        }
    }

    std::optional<std::filesystem::path> Win32FileDialog::OpenFile(const FileDialogOptions& options)
    {
        return ShowFileDialog(options, false);
    }

    std::optional<std::filesystem::path> Win32FileDialog::SaveFile(const FileDialogOptions& options)
    {
        return ShowFileDialog(options, true);
    }

    std::optional<std::filesystem::path> Win32FileDialog::OpenFolder(const FolderDialogOptions& options)
    {
        BROWSEINFOW browseInfo{};
        browseInfo.hwndOwner = ToHwnd(options.ownerWindow);
        browseInfo.lpszTitle = options.title.empty() ? nullptr : options.title.c_str();
        browseInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

        PIDLIST_ABSOLUTE itemList = SHBrowseForFolderW(&browseInfo);
        if (nullptr == itemList)
        {
            return std::nullopt;
        }

        std::optional<std::filesystem::path> folderPath = std::nullopt;
        std::array<wchar_t, MAX_PATH> selectedPath{};
        if (SHGetPathFromIDListW(itemList, selectedPath.data()))
        {
            folderPath = std::filesystem::path(selectedPath.data());
        }

        CoTaskMemFree(itemList);
        return folderPath;
    }
}
