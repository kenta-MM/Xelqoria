#include "Assets/EditorAssetPathUtils.h"

#include <string>
#include <system_error>

#include "Utils/EditorPathSecurity.h"
#include "Utils/EditorStringUtils.h"

namespace Xelqoria::Editor::EditorAssetPathUtils
{
    namespace
    {
        /// <summary>
        /// プロジェクトルート基準の画像ファイルパスから指定種別の AssetId を生成する。
        /// </summary>
        /// <param name="assetKind">AssetId の先頭に付ける種別名。</param>
        /// <param name="path">画像ファイルパス。</param>
        /// <param name="assetsRootDirectory">AssetId の基準になるプロジェクトルート。</param>
        /// <returns>AssetId。生成できない場合は空。</returns>
        [[nodiscard]] Core::AssetId BuildAssetId(
            const char* assetKind,
            const std::filesystem::path& path,
            const std::filesystem::path& assetsRootDirectory)
        {
            if (nullptr == assetKind || true == path.empty() || true == assetsRootDirectory.empty())
            {
                return {};
            }

            std::error_code errorCode;
            const std::filesystem::path relativePath = std::filesystem::relative(path, assetsRootDirectory, errorCode);
            if (errorCode || false == EditorPathSecurity::IsSafeRelativePath(relativePath))
            {
                return {};
            }

            return Core::AssetId(std::string(assetKind) + "/" + ToNarrowString(relativePath.generic_wstring()));
        }
    }

    bool IsTextureImageFile(const std::filesystem::path& path)
    {
        const std::wstring extension = path.extension().wstring();
        return extension == L".png"
            || extension == L".jpg"
            || extension == L".jpeg"
            || extension == L".bmp";
    }

    bool IsSpriteAssetFile(const std::filesystem::path& path)
    {
        return path.extension() == L".sprite";
    }

    bool IsMaterialAssetFile(const std::filesystem::path& path)
    {
        return path.extension() == L".material";
    }

    bool IsCollider2DAssetFile(const std::filesystem::path& path)
    {
        return path.extension() == L".collider2d";
    }

    bool CanPlaceAssetFileInSceneView(const std::filesystem::path& path)
    {
        return IsSpriteAssetFile(path);
    }

    Core::AssetId BuildTextureAssetId(
        const std::filesystem::path& path,
        const std::filesystem::path& assetsRootDirectory)
    {
        return BuildAssetId("textures", path, assetsRootDirectory);
    }

    Core::AssetId BuildSpriteAssetId(
        const std::filesystem::path& path,
        const std::filesystem::path& assetsRootDirectory)
    {
        return BuildAssetId("sprites", path, assetsRootDirectory);
    }

    Core::AssetId BuildMaterialAssetId(
        const std::filesystem::path& path,
        const std::filesystem::path& assetsRootDirectory)
    {
        return BuildAssetId("materials", path, assetsRootDirectory);
    }

    Core::AssetId BuildCollider2DAssetId(
        const std::filesystem::path& path,
        const std::filesystem::path& assetsRootDirectory)
    {
        return BuildAssetId("colliders2d", path, assetsRootDirectory);
    }
}
