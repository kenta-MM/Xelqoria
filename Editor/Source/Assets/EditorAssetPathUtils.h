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
    /// 指定パスが Sprite Asset ファイルかを判定する。
    /// </summary>
    /// <param name="path">判定対象パス。</param>
    /// <returns>Sprite Asset ファイルの場合は true。</returns>
    [[nodiscard]] bool IsSpriteAssetFile(const std::filesystem::path& path);

    /// <summary>
    /// 指定パスが Material Asset ファイルかを判定する。
    /// </summary>
    /// <param name="path">判定対象パス。</param>
    /// <returns>Material Asset ファイルの場合は true。</returns>
    [[nodiscard]] bool IsMaterialAssetFile(const std::filesystem::path& path);

    /// <summary>
    /// 指定パスが Collider2D Asset ファイルかを判定する。
    /// </summary>
    /// <param name="path">判定対象パス。</param>
    /// <returns>Collider2D Asset ファイルの場合は true。</returns>
    [[nodiscard]] bool IsCollider2DAssetFile(const std::filesystem::path& path);

    /// <summary>
    /// 指定アセットファイルを SceneView へ直接配置できるかを判定する。
    /// </summary>
    /// <param name="path">判定対象パス。</param>
    /// <returns>SceneView へ配置できるアセットの場合は true。</returns>
    [[nodiscard]] bool CanPlaceAssetFileInSceneView(const std::filesystem::path& path);

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
    /// プロジェクトルート基準のアセットファイルパスから SpriteAssetId を生成する。
    /// </summary>
    /// <param name="path">画像ファイルパス。</param>
    /// <param name="assetsRootDirectory">AssetId の基準になるプロジェクトルート。</param>
    /// <returns>SpriteAssetId。生成できない場合は空。</returns>
    [[nodiscard]] Core::AssetId BuildSpriteAssetId(
        const std::filesystem::path& path,
        const std::filesystem::path& assetsRootDirectory);

    /// <summary>
    /// プロジェクトルート基準の Material Asset ファイルパスから MaterialAssetId を生成する。
    /// </summary>
    /// <param name="path">Material Asset ファイルパス。</param>
    /// <param name="assetsRootDirectory">AssetId の基準になるプロジェクトルート。</param>
    /// <returns>MaterialAssetId。生成できない場合は空。</returns>
    [[nodiscard]] Core::AssetId BuildMaterialAssetId(
        const std::filesystem::path& path,
        const std::filesystem::path& assetsRootDirectory);

    /// <summary>
    /// プロジェクトルート基準の Collider2D Asset ファイルパスから Collider2DAssetId を生成する。
    /// </summary>
    /// <param name="path">Collider2D Asset ファイルパス。</param>
    /// <param name="assetsRootDirectory">AssetId の基準になるプロジェクトルート。</param>
    /// <returns>Collider2DAssetId。生成できない場合は空。</returns>
    [[nodiscard]] Core::AssetId BuildCollider2DAssetId(
        const std::filesystem::path& path,
        const std::filesystem::path& assetsRootDirectory);
}
