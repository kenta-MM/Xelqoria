#pragma once

#include <Windows.h>
#include <array>
#include <string>

#include "AssetId.h"
#include "Assets/ISpriteAssetResolver.h"

namespace Xelqoria::Editor
{
    class SpritePanelView;

    /// <summary>
    /// Sprite 専用パネルの表示同期を管理する。
    /// </summary>
    class SpritePanelController
    {
    public:
        /// <summary>
        /// Sprite Panel View の HWND 群へ接続する。
        /// </summary>
        /// <param name="view">接続先の Panel View。</param>
        void Bind(const SpritePanelView& view);

        /// <summary>
        /// 指定 SpriteAssetId を Sprite パネルで開く。
        /// </summary>
        /// <param name="spriteAssetId">開く SpriteAssetId。</param>
        /// <param name="spriteAssetResolver">Sprite 内容を解決する Resolver。</param>
        void OpenSprite(
            Core::AssetId spriteAssetId,
            const Game::Assets::ISpriteAssetResolver& spriteAssetResolver);

        /// <summary>
        /// 現在開いている Sprite 表示を最新の Asset 内容へ同期する。
        /// </summary>
        /// <param name="spriteAssetResolver">Sprite 内容を解決する Resolver。</param>
        void Refresh(const Game::Assets::ISpriteAssetResolver& spriteAssetResolver);

        /// <summary>
        /// SpriteAssetId から Sprite パネル表示用のファイル名を取得する。
        /// </summary>
        /// <param name="spriteAssetId">表示対象 SpriteAssetId。</param>
        /// <returns>Sprite パネル表示文字列。</returns>
        [[nodiscard]] static std::wstring FormatSpriteDisplayText(const Core::AssetId& spriteAssetId);

        /// <summary>
        /// AssetId の表示文字列を取得する。
        /// </summary>
        /// <param name="assetId">表示対象 AssetId。</param>
        /// <returns>表示文字列。</returns>
        [[nodiscard]] static std::wstring FormatAssetDisplayText(const Core::AssetId& assetId);

    private:
        void SetSpriteFieldsEnabled(bool enabled) const;

        HWND m_spriteSummaryLabel = nullptr;
        HWND m_spriteDetailsSectionLabel = nullptr;
        std::array<HWND, 4> m_spriteDetailLabels{};
        std::array<HWND, 4> m_spriteDetailEditControls{};
        Core::AssetId m_openSpriteAssetId{};
    };
}
