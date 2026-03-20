#include "AssetManager.h"

#include <utility>

#include "Texture2D.h"
#include <algorithm>
#include <memory>

namespace Xelqoria::Game
{
    namespace
    {
        /// <summary>
        /// AssetId 不正時の結果を生成する。
        /// </summary>
        /// <typeparam name="TAsset">結果が保持するアセット型。</typeparam>
        /// <param name="assetId">失敗対象の AssetId。</param>
        /// <returns>不正入力を表す解決結果。</returns>
        template <typename TAsset>
        AssetResolveResult<TAsset> MakeInvalidAssetIdResult(const AssetId& assetId)
        {
            return AssetResolveResult<TAsset>{
                nullptr,
                assetId,
                AssetResolveError::InvalidAssetId,
                "AssetId is empty: " + assetId
            };
        }

        /// <summary>
        /// 未登録アセット時の結果を生成する。
        /// </summary>
        /// <typeparam name="TAsset">結果が保持するアセット型。</typeparam>
        /// <param name="assetId">失敗対象の AssetId。</param>
        /// <returns>未登録を表す解決結果。</returns>
        template <typename TAsset>
        AssetResolveResult<TAsset> MakeAssetNotFoundResult(const AssetId& assetId)
        {
            return AssetResolveResult<TAsset>{
                nullptr,
                assetId,
                AssetResolveError::AssetNotFound,
                "Asset is not registered: " + assetId
            };
        }

        /// <summary>
        /// アセット実体欠落時の結果を生成する。
        /// </summary>
        /// <typeparam name="TAsset">結果が保持するアセット型。</typeparam>
        /// <param name="assetId">失敗対象の AssetId。</param>
        /// <param name="message">詳細メッセージ。</param>
        /// <returns>データ欠落を表す解決結果。</returns>
        template <typename TAsset>
        AssetResolveResult<TAsset> MakeMissingDataResult(const AssetId& assetId, std::string message)
        {
            return AssetResolveResult<TAsset>{
                nullptr,
                assetId,
                AssetResolveError::AssetDataMissing,
                std::move(message)
            };
        }
    }

    SpriteAsset::SpriteAsset(std::shared_ptr<Graphics::Texture2D> texture)
        : m_texture(std::move(texture))
    {
    }

    std::shared_ptr<Graphics::Texture2D> SpriteAsset::GetTexture() const
    {
        return m_texture;
    }

    bool AssetManager::RegisterSpriteAsset(AssetId assetId, std::shared_ptr<SpriteAsset> spriteAsset)
    {
        if (!IsAssetIdValid(assetId) || !spriteAsset) {
            return false;
        }

        m_spriteAssets[std::move(assetId)] = std::move(spriteAsset);
        return true;
    }

    bool AssetManager::RegisterTexture2D(AssetId assetId, std::shared_ptr<Graphics::Texture2D> texture)
    {
        if (!texture) {
            return false;
        }

        return RegisterSpriteAsset(std::move(assetId), std::make_shared<SpriteAsset>(std::move(texture)));
    }

    SpriteAssetResolveResult AssetManager::ResolveSpriteAsset(const AssetId& assetId) const
    {
        if (!IsAssetIdValid(assetId)) {
            return MakeInvalidAssetIdResult<SpriteAsset>(assetId);
        }

        const auto it = m_spriteAssets.find(assetId);
        if (it == m_spriteAssets.end()) {
            return MakeAssetNotFoundResult<SpriteAsset>(assetId);
        }

        if (!it->second) {
            return MakeMissingDataResult<SpriteAsset>(assetId, "SpriteAsset entry is null: " + assetId);
        }

        return SpriteAssetResolveResult{
            it->second,
            assetId,
            AssetResolveError::None,
            {}
        };
    }

    Texture2DResolveResult AssetManager::ResolveTexture2D(const AssetId& assetId) const
    {
        const SpriteAssetResolveResult spriteAssetResult = ResolveSpriteAsset(assetId);
        if (!spriteAssetResult.Succeeded()) {
            return Texture2DResolveResult{
                nullptr,
                spriteAssetResult.assetId,
                spriteAssetResult.error,
                spriteAssetResult.message
            };
        }

        const std::shared_ptr<Graphics::Texture2D> texture = spriteAssetResult.asset->GetTexture();
        if (!texture) {
            return MakeMissingDataResult<Graphics::Texture2D>(assetId, "SpriteAsset does not have Texture2D: " + assetId);
        }

        return Texture2DResolveResult{
            texture,
            assetId,
            AssetResolveError::None,
            {}
        };
    }

    bool AssetManager::IsAssetIdValid(const AssetId& assetId)
    {
        return !assetId.empty();
    }
}
