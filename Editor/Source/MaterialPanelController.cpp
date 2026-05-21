#include "MaterialPanelController.h"

#include <Windows.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cwctype>
#include <filesystem>
#include <iterator>
#include <optional>
#include <string>
#include <utility>

#include "AssetsPanelController.h"
#include "EditorShell.h"
#include "EditorStringUtils.h"

namespace Xelqoria::Editor
{
    namespace
    {
        [[nodiscard]] POINT ToWin32Point(Platform::Point point)
        {
            return POINT{ static_cast<LONG>(point.x), static_cast<LONG>(point.y) };
        }

        [[nodiscard]] std::wstring_view Trim(std::wstring_view value)
        {
            const auto isSpace =
                [](wchar_t character)
                {
                    return 0 != std::iswspace(character);
                };

            while (false == value.empty() && isSpace(value.front()))
            {
                value.remove_prefix(1);
            }

            while (false == value.empty() && isSpace(value.back()))
            {
                value.remove_suffix(1);
            }

            return value;
        }

        [[nodiscard]] std::wstring GetWindowTextString(HWND window)
        {
            std::array<wchar_t, 512> buffer{};
            if (nullptr == window)
            {
                return {};
            }

            GetWindowTextW(window, buffer.data(), static_cast<int>(buffer.size()));
            return std::wstring(buffer.data());
        }

        [[nodiscard]] bool TryParseFloatText(std::wstring_view text, float& value)
        {
            const std::wstring trimmedText(Trim(text));
            if (trimmedText.empty())
            {
                return false;
            }

            wchar_t* end = nullptr;
            const float parsed = std::wcstof(trimmedText.c_str(), &end);
            if (end == trimmedText.c_str() || nullptr == end || L'\0' != *end)
            {
                return false;
            }

            value = parsed;
            return true;
        }
    }

    void MaterialPanelController::Bind(const EditorShell& shell, Platform::ICursor& cursor)
    {
        m_materialSummaryLabel = shell.GetMaterialSummaryLabel();
        m_materialSharedNoticeLabel = shell.GetMaterialSharedNoticeLabel();
        m_materialDetailsSectionLabel = shell.GetMaterialDetailsSectionLabel();
        m_materialDetailLabels = shell.GetMaterialDetailLabels();
        m_materialDetailEditControls = shell.GetMaterialDetailEditControls();
        m_materialTextureDropHighlight = shell.GetMaterialTextureDropHighlight();
        m_cursor = &cursor;
    }

    void MaterialPanelController::OpenMaterial(
        Core::AssetId materialAssetId,
        const Game::Assets::IMaterialAssetResolver& materialAssetResolver)
    {
        m_openMaterialAssetId = std::move(materialAssetId);
        Refresh(materialAssetResolver);
    }

    void MaterialPanelController::Refresh(const Game::Assets::IMaterialAssetResolver& materialAssetResolver)
    {
        if (m_openMaterialAssetId.IsEmpty())
        {
            SetWindowTextW(m_materialSummaryLabel, L"Material: no material selected");
            ShowWindow(m_materialSharedNoticeLabel, SW_HIDE);
            SetWindowTextW(m_materialDetailEditControls[0], L"");
            SetWindowTextW(m_materialDetailEditControls[1], L"");
            SetWindowTextW(m_materialDetailEditControls[2], L"");
            SetWindowTextW(m_materialDetailEditControls[3], L"");
            SetWindowTextW(m_materialDetailEditControls[4], L"");
            SetMaterialFieldsEnabled(false);
            return;
        }

        const std::optional<Game::Assets::SpriteMaterialAsset> materialAsset =
            materialAssetResolver.ResolveMaterialAsset(m_openMaterialAssetId);
        if (false == materialAsset.has_value())
        {
            std::wstring summary = L"Material: missing ";
            summary += FormatMaterialDisplayText(m_openMaterialAssetId);
            SetWindowTextW(m_materialSummaryLabel, summary.c_str());
            ShowWindow(m_materialSharedNoticeLabel, SW_HIDE);
            SetWindowTextW(m_materialDetailEditControls[0], L"");
            SetWindowTextW(m_materialDetailEditControls[1], L"");
            SetWindowTextW(m_materialDetailEditControls[2], L"");
            SetWindowTextW(m_materialDetailEditControls[3], L"");
            SetWindowTextW(m_materialDetailEditControls[4], L"");
            SetMaterialFieldsEnabled(false);
            return;
        }

        std::wstring summary = L"Material: ";
        summary += FormatMaterialDisplayText(m_openMaterialAssetId);
        SetWindowTextW(m_materialSummaryLabel, summary.c_str());
        SetWindowTextW(m_materialDetailEditControls[0], ToWideString(materialAsset->textureAssetId.GetValue()).c_str());
        SetWindowTextW(m_materialDetailEditControls[1], FormatColorText(materialAsset->color).c_str());
        SetWindowTextW(m_materialDetailEditControls[2], FormatBoolText(materialAsset->outlineEnabled).c_str());
        SetWindowTextW(m_materialDetailEditControls[4], FormatColorText(materialAsset->outlineColor).c_str());

        wchar_t thicknessText[64]{};
        std::swprintf(thicknessText, std::size(thicknessText), L"%.6g", materialAsset->outlineThickness);
        SetWindowTextW(m_materialDetailEditControls[3], thicknessText);
        ShowWindow(m_materialSharedNoticeLabel, SW_SHOW);
        SetMaterialFieldsEnabled(true);
    }

    MaterialPanelApplyResult MaterialPanelController::ApplyEdits(
        const Game::Assets::IMaterialAssetResolver& materialAssetResolver) const
    {
        MaterialPanelApplyResult result{};
        if (m_openMaterialAssetId.IsEmpty())
        {
            return result;
        }

        const std::optional<Game::Assets::SpriteMaterialAsset> currentMaterialAsset =
            materialAssetResolver.ResolveMaterialAsset(m_openMaterialAssetId);
        if (false == currentMaterialAsset.has_value())
        {
            return result;
        }

        Game::Assets::SpriteMaterialAsset editedMaterialAsset = *currentMaterialAsset;
        bool changed = false;

        const std::wstring textureText = GetWindowTextString(m_materialDetailEditControls[0]);
        const std::wstring_view trimmedTextureText = Trim(textureText);
        const Core::AssetId textureAssetId(ToNarrowString(trimmedTextureText));
        if (editedMaterialAsset.textureAssetId != textureAssetId)
        {
            editedMaterialAsset.textureAssetId = textureAssetId;
            changed = true;
        }

        std::array<float, 4> tintColor{};
        if (TryParseColorText(GetWindowTextString(m_materialDetailEditControls[1]), tintColor)
            && editedMaterialAsset.color != tintColor)
        {
            editedMaterialAsset.color = tintColor;
            changed = true;
        }

        bool outlineEnabled = false;
        if (TryParseBoolText(GetWindowTextString(m_materialDetailEditControls[2]), outlineEnabled)
            && editedMaterialAsset.outlineEnabled != outlineEnabled)
        {
            editedMaterialAsset.outlineEnabled = outlineEnabled;
            changed = true;
        }

        float outlineThickness = 0.0f;
        if (TryParseFloatText(GetWindowTextString(m_materialDetailEditControls[3]), outlineThickness)
            && editedMaterialAsset.outlineThickness != outlineThickness)
        {
            editedMaterialAsset.outlineThickness = outlineThickness;
            changed = true;
        }

        std::array<float, 4> outlineColor{};
        if (TryParseColorText(GetWindowTextString(m_materialDetailEditControls[4]), outlineColor)
            && editedMaterialAsset.outlineColor != outlineColor)
        {
            editedMaterialAsset.outlineColor = outlineColor;
            changed = true;
        }

        if (false == changed)
        {
            return result;
        }

        result.materialAssetChanged = true;
        result.materialTargetAssetId = m_openMaterialAssetId;
        result.materialAsset = editedMaterialAsset;
        result.operationName = "Edit Material";
        return result;
    }

    MaterialPanelApplyResult MaterialPanelController::ApplyTextureDrop(
        const AssetsPanelController& assetsPanelController,
        const Game::Assets::IMaterialAssetResolver& materialAssetResolver) const
    {
        MaterialPanelApplyResult result{};
        if (m_openMaterialAssetId.IsEmpty()
            || true == assetsPanelController.GetDraggingTextureAssetId().IsEmpty()
            || false == assetsPanelController.WasDragReleasedThisFrame()
            || false == IsTextureDropTargetHovered(assetsPanelController))
        {
            return result;
        }

        const std::optional<Game::Assets::SpriteMaterialAsset> currentMaterialAsset =
            materialAssetResolver.ResolveMaterialAsset(m_openMaterialAssetId);
        if (false == currentMaterialAsset.has_value()
            || currentMaterialAsset->textureAssetId == assetsPanelController.GetDraggingTextureAssetId())
        {
            return result;
        }

        result.materialAssetChanged = true;
        result.materialTargetAssetId = m_openMaterialAssetId;
        result.materialAsset = *currentMaterialAsset;
        result.materialAsset.textureAssetId = assetsPanelController.GetDraggingTextureAssetId();
        result.operationName = "Change Material Texture";
        return result;
    }

    void MaterialPanelController::UpdateTextureDropHighlight(const AssetsPanelController& assetsPanelController)
    {
        const bool shouldShow = IsTextureDropTargetHovered(assetsPanelController);
        if (shouldShow == m_isTextureDropHighlightVisible)
        {
            return;
        }

        m_isTextureDropHighlightVisible = shouldShow;
        if (nullptr == m_materialTextureDropHighlight)
        {
            return;
        }

        ShowWindow(m_materialTextureDropHighlight, shouldShow ? SW_SHOW : SW_HIDE);
        if (shouldShow)
        {
            RECT targetRect{};
            HWND parentWindow = GetParent(m_materialTextureDropHighlight);
            GetWindowRect(m_materialDetailEditControls[0], &targetRect);
            MapWindowPoints(nullptr, parentWindow, reinterpret_cast<POINT*>(&targetRect), 2);
            InflateRect(&targetRect, 2, 2);
            SetWindowPos(
                m_materialTextureDropHighlight,
                m_materialDetailEditControls[0],
                targetRect.left,
                targetRect.top,
                targetRect.right - targetRect.left,
                targetRect.bottom - targetRect.top,
                SWP_NOACTIVATE);
            InvalidateRect(m_materialTextureDropHighlight, nullptr, TRUE);
        }
    }

    void MaterialPanelController::ResetOpenMaterial()
    {
        m_openMaterialAssetId = {};
        m_isTextureDropHighlightVisible = false;
        ShowWindow(m_materialTextureDropHighlight, SW_HIDE);
    }

    std::wstring MaterialPanelController::FormatMaterialDisplayText(const Core::AssetId& materialAssetId)
    {
        if (materialAssetId.IsEmpty())
        {
            return L"None";
        }

        std::string materialRefValue = materialAssetId.GetValue();
        constexpr std::string_view materialAssetPrefix = "materials/";
        if (materialRefValue.starts_with(materialAssetPrefix))
        {
            materialRefValue = materialRefValue.substr(materialAssetPrefix.size());
        }

        const std::wstring relativePath = ToWideString(materialRefValue);
        const std::filesystem::path displayPath(relativePath);
        const std::wstring fileName = displayPath.filename().wstring();
        if (false == fileName.empty())
        {
            return fileName;
        }

        return relativePath;
    }

    std::wstring MaterialPanelController::FormatColorText(const std::array<float, 4>& color)
    {
        wchar_t text[128]{};
        std::swprintf(
            text,
            std::size(text),
            L"%.6g,%.6g,%.6g,%.6g",
            color[0],
            color[1],
            color[2],
            color[3]);
        return text;
    }

    std::wstring MaterialPanelController::FormatBoolText(bool value)
    {
        return value ? L"true" : L"false";
    }

    bool MaterialPanelController::TryParseColorText(std::wstring_view text, std::array<float, 4>& color)
    {
        std::size_t cursor = 0;
        for (int index = 0; index < 4; ++index)
        {
            const std::size_t separator = text.find(L',', cursor);
            const std::wstring_view token = separator == std::wstring_view::npos
                ? text.substr(cursor)
                : text.substr(cursor, separator - cursor);

            float component = 0.0f;
            if (false == TryParseFloatText(token, component))
            {
                return false;
            }

            color[static_cast<std::size_t>(index)] = component;
            if (3 == index)
            {
                return separator == std::wstring_view::npos;
            }

            if (separator == std::wstring_view::npos)
            {
                return false;
            }

            cursor = separator + 1;
        }

        return false;
    }

    bool MaterialPanelController::TryParseBoolText(std::wstring_view text, bool& value)
    {
        const std::wstring trimmedText(Trim(text));
        std::wstring lowerText{};
        lowerText.reserve(trimmedText.size());
        for (wchar_t character : trimmedText)
        {
            lowerText.push_back(static_cast<wchar_t>(std::towlower(character)));
        }

        if (lowerText == L"true" || lowerText == L"1")
        {
            value = true;
            return true;
        }

        if (lowerText == L"false" || lowerText == L"0")
        {
            value = false;
            return true;
        }

        return false;
    }

    bool MaterialPanelController::IsTextureDropTargetHovered(const AssetsPanelController& assetsPanelController) const
    {
        const bool hasTextureDrag = assetsPanelController.IsDragActive()
            || assetsPanelController.WasDragReleasedThisFrame();
        if (false == hasTextureDrag
            || m_openMaterialAssetId.IsEmpty()
            || true == assetsPanelController.GetDraggingTextureAssetId().IsEmpty()
            || nullptr == m_materialDetailEditControls[0]
            || FALSE == IsWindowVisible(m_materialDetailEditControls[0]))
        {
            return false;
        }

        if (nullptr == m_cursor)
        {
            return false;
        }

        const POINT cursorPoint = ToWin32Point(m_cursor->GetScreenPosition());
        RECT textureRect{};
        GetWindowRect(m_materialDetailEditControls[0], &textureRect);
        return PtInRect(&textureRect, cursorPoint) != FALSE;
    }

    void MaterialPanelController::SetMaterialFieldsEnabled(bool enabled) const
    {
        const BOOL enabledValue = enabled ? TRUE : FALSE;
        EnableWindow(m_materialDetailsSectionLabel, enabledValue);
        for (std::size_t index = 0; index < m_materialDetailLabels.size(); ++index)
        {
            EnableWindow(m_materialDetailLabels[index], enabledValue);
            EnableWindow(m_materialDetailEditControls[index], enabledValue);
        }
    }
}
