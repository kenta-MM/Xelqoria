#include "AssetsPanelController.h"

#include <algorithm>
#include <cstdlib>
#include "EditorStringUtils.h"
#include <Windows.h>
#include <cstdio>
#include <iterator>
#include <string>
#include <vector>
#include <AssetId.h>
#include "EditorShell.h"
#include <Assets/SpriteAssetRegistry.h>
#include <TextureAssetRegistry.h>

namespace Xelqoria::Editor
{
    namespace
    {
        constexpr int AssetDragStartThresholdPixels = 6;

        /// <summary>
        /// 2 点間のドラッグ開始判定に使う距離を取得する。
        /// </summary>
        /// <param name="lhs">始点座標。</param>
        /// <param name="rhs">終点座標。</param>
        /// <returns>各軸の絶対差分のうち大きい方。</returns>
        const int GetMaxAxisDragDelta(POINT lhs, POINT rhs)
        {
            const int dx = std::abs(lhs.x - rhs.x);
            const int dy = std::abs(lhs.y - rhs.y);
            return (std::max)(dx, dy);
        }
    }

    void AssetsPanelController::Bind(const EditorShell& shell)
    {
        m_assetsListBox = shell.GetAssetsListBox();
        m_assetsSummaryLabel = shell.GetAssetsSummaryLabel();
    }

    void AssetsPanelController::Refresh(
        const std::vector<Core::AssetId>& registeredSpriteAssetIds,
        const Game::Assets::SpriteAssetRegistry& spriteAssetRegistry,
        const Graphics::TextureAssetRegistry& textureAssetRegistry)
    {
        m_registeredSpriteAssetCount = registeredSpriteAssetIds.size();
        m_visibleSpriteAssetIds.clear();

        for (const Core::AssetId& assetId : registeredSpriteAssetIds)
        {
            if (assetId.IsEmpty())
            {
                continue;
            }

            const auto spriteAsset = spriteAssetRegistry.ResolveSpriteAsset(assetId);
            if (false == spriteAsset.has_value())
            {
                continue;
            }

            if (false == static_cast<bool>(textureAssetRegistry.ResolveTexture(spriteAsset->textureAssetId)))
            {
                continue;
            }

            m_visibleSpriteAssetIds.push_back(assetId);
        }

        SendMessageW(m_assetsListBox, LB_RESETCONTENT, 0, 0);
        for (const Core::AssetId& assetId : m_visibleSpriteAssetIds)
        {
            const std::wstring text = ToWideString(assetId.GetValue());
            SendMessageW(m_assetsListBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
        }

        if (m_selectedSpriteAssetId.IsEmpty() && false == m_visibleSpriteAssetIds.empty())
        {
            m_selectedSpriteAssetId = m_visibleSpriteAssetIds.front();
        }

        int selectedIndex = LB_ERR;
        for (std::size_t index = 0; index < m_visibleSpriteAssetIds.size(); ++index)
        {
            if (m_visibleSpriteAssetIds[index] == m_selectedSpriteAssetId)
            {
                selectedIndex = static_cast<int>(index);
                break;
            }
        }

        if (selectedIndex != LB_ERR)
        {
            SendMessageW(m_assetsListBox, LB_SETCURSEL, static_cast<WPARAM>(selectedIndex), 0);
        }

        RefreshSummaryLabel();
    }

    void AssetsPanelController::SyncSelection()
    {
        if (nullptr == m_assetsListBox)
        {
            return;
        }

        const LRESULT selectedIndex = SendMessageW(m_assetsListBox, LB_GETCURSEL, 0, 0);
        if (selectedIndex == LB_ERR)
        {
            return;
        }

        const auto index = static_cast<std::size_t>(selectedIndex);
        if (index >= m_visibleSpriteAssetIds.size())
        {
            return;
        }

        m_selectedSpriteAssetId = m_visibleSpriteAssetIds[index];
    }

    void AssetsPanelController::UpdateDragState()
    {
        if (nullptr == m_assetsListBox)
        {
            return;
        }

        m_assetDragReleasedThisFrame = false;

        POINT screenPoint{};
        GetCursorPos(&screenPoint);

        RECT assetsRect{};
        GetWindowRect(m_assetsListBox, &assetsRect);

        const bool isCursorInside = screenPoint.x >= assetsRect.left
            && screenPoint.x < assetsRect.right
            && screenPoint.y >= assetsRect.top
            && screenPoint.y < assetsRect.bottom;
        const bool isLeftButtonDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        if (isCursorInside && isLeftButtonDown && false == m_assetsListLeftButtonDown)
        {
            m_assetsListLeftButtonDown = true;
            m_assetDragStartScreenPoint = screenPoint;
        }

        if (m_assetsListLeftButtonDown
            && false == m_isAssetDragActive
            && isLeftButtonDown
            && false == m_selectedSpriteAssetId.IsEmpty()
            && GetMaxAxisDragDelta(m_assetDragStartScreenPoint, screenPoint) >= AssetDragStartThresholdPixels)
        {
            m_isAssetDragActive = true;
            m_draggingSpriteAssetId = m_selectedSpriteAssetId;

            const std::string debugLine =
                "Editor::AssetsPanelController began dragging Sprite AssetId '" + m_draggingSpriteAssetId.GetValue() + "'.\n";
            ::OutputDebugStringA(debugLine.c_str());
            RefreshSummaryLabel();
        }

        if (false == isLeftButtonDown)
        {
            const bool wasDragging = m_isAssetDragActive;
            m_assetsListLeftButtonDown = false;
            m_isAssetDragActive = false;
            m_assetDragReleasedThisFrame = wasDragging;
        }
    }

    void AssetsPanelController::CompleteReleasedDrag()
    {
        m_draggingSpriteAssetId = {};
        RefreshSummaryLabel();
    }

    const Core::AssetId& AssetsPanelController::GetSelectedAssetId() const
    {
        return m_selectedSpriteAssetId;
    }

    const Core::AssetId& AssetsPanelController::GetDraggingSpriteAssetId() const
    {
        return m_draggingSpriteAssetId;
    }

    bool AssetsPanelController::IsDragActive() const
    {
        return m_isAssetDragActive;
    }

    bool AssetsPanelController::WasDragReleasedThisFrame() const
    {
        return m_assetDragReleasedThisFrame;
    }

    bool AssetsPanelController::HasVisibleSpriteAssets() const
    {
        return false == m_visibleSpriteAssetIds.empty();
    }

    void AssetsPanelController::RefreshSummaryLabel()
    {
        wchar_t summaryText[256]{};
        if (m_isAssetDragActive && false == m_draggingSpriteAssetId.IsEmpty())
        {
            const std::wstring draggingAssetId = ToWideString(m_draggingSpriteAssetId.GetValue());
            std::swprintf(
                summaryText,
                std::size(summaryText),
                L"Sprite assets: dragging %ls / %u visible / %u registered",
                draggingAssetId.c_str(),
                static_cast<unsigned>(m_visibleSpriteAssetIds.size()),
                static_cast<unsigned>(m_registeredSpriteAssetCount));
        }
        else
        {
            std::swprintf(
                summaryText,
                std::size(summaryText),
                L"Sprite assets: %u visible / %u registered",
                static_cast<unsigned>(m_visibleSpriteAssetIds.size()),
                static_cast<unsigned>(m_registeredSpriteAssetCount));
        }

        SetWindowTextW(m_assetsSummaryLabel, summaryText);
    }
}
