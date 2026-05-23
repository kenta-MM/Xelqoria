#pragma once

namespace Xelqoria::Editor
{
    /// <summary>
    /// Editor UI で共有する OS 非依存の RGBA 色を表す。
    /// </summary>
    struct EditorColor
    {
        /// <summary>
        /// 赤成分。0.0 から 1.0 の範囲で扱う。
        /// </summary>
        float red = 0.0f;

        /// <summary>
        /// 緑成分。0.0 から 1.0 の範囲で扱う。
        /// </summary>
        float green = 0.0f;

        /// <summary>
        /// 青成分。0.0 から 1.0 の範囲で扱う。
        /// </summary>
        float blue = 0.0f;

        /// <summary>
        /// 透明度。0.0 から 1.0 の範囲で扱う。
        /// </summary>
        float alpha = 1.0f;

        /// <summary>
        /// 8bit RGB 値から不透明な EditorColor を作成する。
        /// </summary>
        /// <param name="redValue">赤成分。0 から 255 の範囲で扱う。</param>
        /// <param name="greenValue">緑成分。0 から 255 の範囲で扱う。</param>
        /// <param name="blueValue">青成分。0 から 255 の範囲で扱う。</param>
        /// <returns>正規化済み RGBA 色。</returns>
        [[nodiscard]] static constexpr EditorColor FromRgb8(int redValue, int greenValue, int blueValue)
        {
            constexpr float MaxChannelValue = 255.0f;
            return EditorColor{
                static_cast<float>(redValue) / MaxChannelValue,
                static_cast<float>(greenValue) / MaxChannelValue,
                static_cast<float>(blueValue) / MaxChannelValue,
                1.0f
            };
        }
    };

    /// <summary>
    /// Editor UI の見た目に使う共通テーマ色を保持する。
    /// </summary>
    struct EditorTheme
    {
        /// <summary>
        /// Editor Window 全体の背景色。
        /// </summary>
        EditorColor windowBackground{};

        /// <summary>
        /// パネル本文の背景色。
        /// </summary>
        EditorColor panelBackground{};

        /// <summary>
        /// パネルヘッダーの背景色。
        /// </summary>
        EditorColor panelHeaderBackground{};

        /// <summary>
        /// パネル境界線の色。
        /// </summary>
        EditorColor panelBorder{};

        /// <summary>
        /// 主要テキスト色。
        /// </summary>
        EditorColor textPrimary{};

        /// <summary>
        /// 補助テキスト色。
        /// </summary>
        EditorColor textSecondary{};

        /// <summary>
        /// 強調表示に使うアクセント色。
        /// </summary>
        EditorColor accent{};

        /// <summary>
        /// 選択状態の背景色。
        /// </summary>
        EditorColor selection{};

        /// <summary>
        /// hover 状態の背景色。
        /// </summary>
        EditorColor hover{};

        /// <summary>
        /// 警告ログなどに使う色。
        /// </summary>
        EditorColor warning{};

        /// <summary>
        /// エラーログなどに使う色。
        /// </summary>
        EditorColor error{};

        /// <summary>
        /// 標準パネルの角丸半径。96 DPI 基準の pixel 単位で扱う。
        /// </summary>
        int panelCornerRadius = 0;

        /// <summary>
        /// 標準コントロールの角丸半径。96 DPI 基準の pixel 単位で扱う。
        /// </summary>
        int controlCornerRadius = 0;

        /// <summary>
        /// パネル境界線の太さ。96 DPI 基準の pixel 単位で扱う。
        /// </summary>
        int panelBorderThickness = 1;
    };

    namespace EditorThemes
    {
        /// <summary>
        /// Xelqoria Editor の既定暗色テーマ。
        /// </summary>
        inline constexpr EditorTheme XelqoriaDark{
            EditorColor::FromRgb8(0x08, 0x0B, 0x13),
            EditorColor::FromRgb8(0x0E, 0x13, 0x20),
            EditorColor::FromRgb8(0x12, 0x17, 0x28),
            EditorColor::FromRgb8(0x1C, 0x24, 0x38),
            EditorColor::FromRgb8(0xE9, 0xEE, 0xFF),
            EditorColor::FromRgb8(0x8D, 0x96, 0xAF),
            EditorColor::FromRgb8(0x8B, 0x5C, 0xFF),
            EditorColor::FromRgb8(0x2A, 0x1D, 0x5F),
            EditorColor::FromRgb8(0x19, 0x22, 0x37),
            EditorColor::FromRgb8(0xD9, 0xB0, 0x58),
            EditorColor::FromRgb8(0xFF, 0x6E, 0x8A),
            6,
            4,
            1
        };
    }
}
