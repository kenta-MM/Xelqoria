#pragma once

#include <Windows.h>
#include <array>
#include <optional>
#include <string>
#include <string_view>

#include "AssetId.h"
#include "Assets/IMaterialAssetResolver.h"
#include "Assets/SpriteMaterialAsset.h"
#include "ICursor.h"

namespace Xelqoria::Editor
{
    class AssetsPanelController;
    class EditorShell;

    /// <summary>
    /// Material パネルで行われた編集適用結果を表す。
    /// </summary>
    struct MaterialPanelApplyResult
    {
        /// <summary>
        /// Material Asset の保存が必要かを表す。
        /// </summary>
        bool materialAssetChanged = false;

        /// <summary>
        /// 保存対象の MaterialAssetId を表す。
        /// </summary>
        Core::AssetId materialTargetAssetId{};

        /// <summary>
        /// 保存する Material 内容を表す。
        /// </summary>
        Game::Assets::SpriteMaterialAsset materialAsset{};

        /// <summary>
        /// 履歴やログで使う操作名を表す。
        /// </summary>
        std::string operationName{};
    };

    /// <summary>
    /// 開いている Material Asset の表示同期と編集反映を管理する。
    /// </summary>
    class MaterialPanelController
    {
    public:
        /// <summary>
        /// EditorShell の Material パネル HWND 群へ接続する。
        /// </summary>
        /// <param name="shell">接続先の EditorShell。</param>
        /// <param name="cursor">カーソル位置を取得する Platform 実装。</param>
        void Bind(const EditorShell& shell, Platform::ICursor& cursor);

        /// <summary>
        /// 指定 MaterialAssetId を Material パネルで開く。
        /// </summary>
        /// <param name="materialAssetId">開く MaterialAssetId。</param>
        /// <param name="materialAssetResolver">Material 内容を解決する Resolver。</param>
        void OpenMaterial(
            Core::AssetId materialAssetId,
            const Game::Assets::IMaterialAssetResolver& materialAssetResolver);

        /// <summary>
        /// 現在開いている Material 表示を最新の Asset 内容へ同期する。
        /// </summary>
        /// <param name="materialAssetResolver">Material 内容を解決する Resolver。</param>
        void Refresh(const Game::Assets::IMaterialAssetResolver& materialAssetResolver);

        /// <summary>
        /// Material パネルの入力値を Material Asset へ反映する。
        /// </summary>
        /// <param name="materialAssetResolver">現在保存されている Material 内容を解決する Resolver。</param>
        /// <returns>適用結果。</returns>
        [[nodiscard]] MaterialPanelApplyResult ApplyEdits(
            const Game::Assets::IMaterialAssetResolver& materialAssetResolver) const;

        /// <summary>
        /// Assets から Texture 欄へドロップされた画像を開いている Material へ反映する。
        /// </summary>
        /// <param name="assetsPanelController">Assets パネルのドラッグ状態。</param>
        /// <param name="materialAssetResolver">現在保存されている Material 内容を解決する Resolver。</param>
        /// <returns>適用結果。</returns>
        [[nodiscard]] MaterialPanelApplyResult ApplyTextureDrop(
            const AssetsPanelController& assetsPanelController,
            const Game::Assets::IMaterialAssetResolver& materialAssetResolver) const;

        /// <summary>
        /// Texture 欄のドロップ先ハイライトを現在のドラッグ状態へ同期する。
        /// </summary>
        /// <param name="assetsPanelController">Assets パネルのドラッグ状態。</param>
        void UpdateTextureDropHighlight(const AssetsPanelController& assetsPanelController);

        /// <summary>
        /// 開いている Material の追跡状態を破棄する。
        /// </summary>
        void ResetOpenMaterial();

        /// <summary>
        /// MaterialAssetId から Material パネル表示用のファイル名を取得する。
        /// </summary>
        /// <param name="materialAssetId">表示対象 MaterialAssetId。</param>
        /// <returns>Material パネル表示文字列。</returns>
        [[nodiscard]] static std::wstring FormatMaterialDisplayText(const Core::AssetId& materialAssetId);

        /// <summary>
        /// RGBA 色を Material パネルの編集文字列へ変換する。
        /// </summary>
        /// <param name="color">表示対象色。</param>
        /// <returns>r,g,b,a 形式の文字列。</returns>
        [[nodiscard]] static std::wstring FormatColorText(const std::array<float, 4>& color);

        /// <summary>
        /// 真偽値を Material パネルの編集文字列へ変換する。
        /// </summary>
        /// <param name="value">表示対象の真偽値。</param>
        /// <returns>true または false。</returns>
        [[nodiscard]] static std::wstring FormatBoolText(bool value);

        /// <summary>
        /// Material パネルの色入力文字列を RGBA 色へ変換する。
        /// </summary>
        /// <param name="text">r,g,b,a 形式の文字列。</param>
        /// <param name="color">変換結果の出力先。</param>
        /// <returns>変換できた場合は true。</returns>
        [[nodiscard]] static bool TryParseColorText(std::wstring_view text, std::array<float, 4>& color);

        /// <summary>
        /// Material パネルの真偽値入力文字列を bool へ変換する。
        /// </summary>
        /// <param name="text">true または false。</param>
        /// <param name="value">変換結果の出力先。</param>
        /// <returns>変換できた場合は true。</returns>
        [[nodiscard]] static bool TryParseBoolText(std::wstring_view text, bool& value);

    private:
        /// <summary>
        /// Material Texture 欄のドロップ対象判定を行う。
        /// </summary>
        /// <param name="assetsPanelController">Assets パネルのドラッグ状態。</param>
        /// <returns>Texture 欄上にドロップ可能な場合は true。</returns>
        [[nodiscard]] bool IsTextureDropTargetHovered(const AssetsPanelController& assetsPanelController) const;

        /// <summary>
        /// Material 入力欄の有効状態を一括設定する。
        /// </summary>
        /// <param name="enabled">有効化する場合は true。</param>
        void SetMaterialFieldsEnabled(bool enabled) const;

        HWND m_materialSummaryLabel = nullptr;
        HWND m_materialSharedNoticeLabel = nullptr;
        HWND m_materialDetailsSectionLabel = nullptr;
        std::array<HWND, 5> m_materialDetailLabels{};
        std::array<HWND, 5> m_materialDetailEditControls{};
        HWND m_materialTextureDropHighlight = nullptr;
        Platform::ICursor* m_cursor = nullptr;
        Core::AssetId m_openMaterialAssetId{};
        bool m_isTextureDropHighlightVisible = false;
    };
}
