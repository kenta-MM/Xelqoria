#pragma once

#include <filesystem>

#include "AssetId.h"

namespace Xelqoria::Editor::EditorAssetPathUtils
{
    /// <summary>
    /// 指定パスが Texture として扱える画像ファイルかを判定する。
    /// </summary>
    /// <param name="path">判定対象パス。</param>
    /// <returns>画像ファイルの場合は true。</returns>
    [[nodiscard]] bool IsTextureImageFile(const std::filesystem::path& path);

    /// <summary>
    /// プロジェクトルート基準の画像ファイルパスから TextureAssetId を生成する。
    /// </summary>
    /// <param name="path">画像ファイルパス。</param>
    /// <param name="assetsRootDirectory">AssetId の基準になるプロジェクトルート。</param>
    /// <returns>TextureAssetId。生成できない場合は空。</returns>
    [[nodiscard]] Core::AssetId BuildTextureAssetId(
        const std::filesystem::path& path,
        const std::filesystem::path& assetsRootDirectory);

    /// <summary>
    /// プロジェクトルート基準の画像ファイルパスから SpriteAssetId を生成する。
    /// </summary>
    /// <param name="path">画像ファイルパス。</param>
    /// <param name="assetsRootDirectory">AssetId の基準になるプロジェクトルート。</param>
    /// <returns>SpriteAssetId。生成できない場合は空。</returns>
    [[nodiscard]] Core::AssetId BuildSpriteAssetId(
        const std::filesystem::path& path,
        const std::filesystem::path& assetsRootDirectory);
}
