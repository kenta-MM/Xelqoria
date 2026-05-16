#include "Win32Clipboard.h"

#include <Windows.h>
#include <cstring>

namespace Xelqoria::Platform::Win32
{
    namespace
    {
        [[nodiscard]] HWND ToHwnd(NativeWindowHandle handle)
        {
            return static_cast<HWND>(handle);
        }
    }

    Win32Clipboard::Win32Clipboard(NativeWindowHandle ownerWindow)
        : m_ownerWindow(ownerWindow)
    {
    }

    bool Win32Clipboard::SetText(const std::wstring& text)
    {
        if (text.empty() || FALSE == OpenClipboard(ToHwnd(m_ownerWindow)))
        {
            return false;
        }

        EmptyClipboard();
        const SIZE_T byteSize = (text.size() + 1u) * sizeof(wchar_t);
        HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, byteSize);
        if (nullptr == memory)
        {
            CloseClipboard();
            return false;
        }

        void* data = GlobalLock(memory);
        if (nullptr == data)
        {
            GlobalFree(memory);
            CloseClipboard();
            return false;
        }

        std::memcpy(data, text.c_str(), byteSize);
        GlobalUnlock(memory);
        SetClipboardData(CF_UNICODETEXT, memory);
        CloseClipboard();
        return true;
    }

    std::wstring Win32Clipboard::GetText() const
    {
        if (FALSE == OpenClipboard(ToHwnd(m_ownerWindow)))
        {
            return {};
        }

        HANDLE clipboardData = GetClipboardData(CF_UNICODETEXT);
        if (nullptr == clipboardData)
        {
            CloseClipboard();
            return {};
        }

        const wchar_t* text = static_cast<const wchar_t*>(GlobalLock(clipboardData));
        if (nullptr == text)
        {
            CloseClipboard();
            return {};
        }

        const std::wstring result = text;
        GlobalUnlock(clipboardData);
        CloseClipboard();
        return result;
    }
}
