#include "Panels/SpritePanelController.h"

#include <filesystem>
#include <utility>

#include "Shell/EditorShell.h"
#include "Utils/EditorStringUtils.h"

namespace Xelqoria::Editor
{
    void SpritePanelController::Bind(const EditorShell& shell)
    {
        m_spriteSummaryLabel = shell.GetSpriteSummaryLabel();
        m_spriteDetailsSectionLabel = shell.GetSpriteDetailsSectionLabel();
        m_spriteDetailLabels = shell.GetSpriteDetailLabels();
        m_spriteDetailEditControls = shell.GetSpriteDetailEditControls();
    }

    void SpritePanelController::OpenSprite(
        Core::AssetId spriteAssetId,
        const Game::Assets::ISpriteAssetResolver& spriteAssetResolver)
    {
        m_openSpriteAssetId = std::move(spriteAssetId);
        Refresh(spriteAssetResolver);
    }

    void SpritePanelController::Refresh(const Game::Assets::ISpriteAssetResolver& spriteAssetResolver)
    {
        if (m_openSpriteAssetId.IsEmpty())
        {
            SetWindowTextW(m_spriteSummaryLabel, L"Sprite: no sprite selected");
            for (HWND editControl : m_spriteDetailEditControls)
            {
                SetWindowTextW(editControl, L"");
            }
            SetSpriteFieldsEnabled(false);
            return;
        }

        const auto spriteAsset = spriteAssetResolver.ResolveSpriteAsset(m_openSpriteAssetId);
        if (false == spriteAsset.has_value())
        {
            std::wstring summary = L"Sprite: missing ";
            summary += FormatSpriteDisplayText(m_openSpriteAssetId);
            SetWindowTextW(m_spriteSummaryLabel, summary.c_str());
            for (HWND editControl : m_spriteDetailEditControls)
            {
                SetWindowTextW(editControl, L"");
            }
            SetSpriteFieldsEnabled(false);
            return;
        }

        std::wstring summary = L"Sprite: ";
        summary += FormatSpriteDisplayText(m_openSpriteAssetId);
        SetWindowTextW(m_spriteSummaryLabel, summary.c_str());
        SetWindowTextW(m_spriteDetailEditControls[0], FormatAssetDisplayText(spriteAsset->textureAssetId).c_str());
        SetWindowTextW(m_spriteDetailEditControls[1], FormatAssetDisplayText(spriteAsset->materialAssetId).c_str());
        SetWindowTextW(m_spriteDetailEditControls[2], FormatAssetDisplayText(spriteAsset->scriptAssetId).c_str());
        SetWindowTextW(m_spriteDetailEditControls[3], FormatAssetDisplayText(spriteAsset->collider2DAssetId).c_str());
        SetSpriteFieldsEnabled(true);
    }

    std::wstring SpritePanelController::FormatSpriteDisplayText(const Core::AssetId& spriteAssetId)
    {
        if (spriteAssetId.IsEmpty())
        {
            return L"None";
        }

        std::string spriteRefValue = spriteAssetId.GetValue();
        constexpr std::string_view spriteAssetPrefix = "sprites/";
        if (spriteRefValue.starts_with(spriteAssetPrefix))
        {
            spriteRefValue = spriteRefValue.substr(spriteAssetPrefix.size());
        }

        const std::filesystem::path displayPath(ToWideString(spriteRefValue));
        const std::wstring fileName = displayPath.filename().wstring();
        return fileName.empty() ? ToWideString(spriteRefValue) : fileName;
    }

    std::wstring SpritePanelController::FormatAssetDisplayText(const Core::AssetId& assetId)
    {
        if (assetId.IsEmpty())
        {
            return L"None";
        }

        return ToWideString(assetId.GetValue());
    }

    void SpritePanelController::SetSpriteFieldsEnabled(bool enabled) const
    {
        const BOOL enabledValue = enabled ? TRUE : FALSE;
        EnableWindow(m_spriteDetailsSectionLabel, enabledValue);
        for (std::size_t index = 0; index < m_spriteDetailLabels.size(); ++index)
        {
            EnableWindow(m_spriteDetailLabels[index], enabledValue);
            EnableWindow(m_spriteDetailEditControls[index], enabledValue);
        }
    }
}
