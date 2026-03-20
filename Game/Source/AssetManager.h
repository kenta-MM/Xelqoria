#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace Xelqoria::Graphics
{
    class Texture2D;
}

namespace Xelqoria::Game
{
    /// <summary>
    /// アセットを識別するための文字列 ID。
    /// </summary>
    using AssetId = std::string;

    /// <summary>
    /// アセット解決失敗の種別を表す。
    /// </summary>
    enum class AssetResolveError
    {
        None = 0,
        InvalidAssetId,
        AssetNotFound,
        AssetDataMissing
    };

    /// <summary>
    /// Sprite 用アセットとして扱うデータを保持する。
    /// </summary>
    class SpriteAsset
    {
    public:
        /// <summary>
        /// SpriteAsset を生成する。
        /// </summary>
        /// <param name="texture">描画に使用する Texture2D。</param>
        explicit SpriteAsset(std::shared_ptr<Graphics::Texture2D> texture);

        /// <summary>
        /// 紐づく Texture2D を取得する。
        /// </summary>
        /// <returns>保持している Texture2D。</returns>
        std::shared_ptr<Graphics::Texture2D> GetTexture() const;

    private:
        std::shared_ptr<Graphics::Texture2D> m_texture;
    };

    /// <summary>
    /// AssetManager の解決結果を呼び出し側へ返す。
    /// </summary>
    template <typename TAsset>
    struct AssetResolveResult
    {
        /// <summary>
        /// 解決に成功したアセット本体を表す。
        /// </summary>
        std::shared_ptr<TAsset> asset{};

        /// <summary>
        /// 解決を試みた AssetId を表す。
        /// </summary>
        AssetId assetId{};

        /// <summary>
        /// 解決失敗時の種別を表す。
        /// </summary>
        AssetResolveError error = AssetResolveError::None;

        /// <summary>
        /// 呼び出し側でログに使える説明文を表す。
        /// </summary>
        std::string message{};

        /// <summary>
        /// 解決成功可否を取得する。
        /// </summary>
        /// <returns>成功時は true。</returns>
        bool Succeeded() const
        {
            return asset != nullptr && error == AssetResolveError::None;
        }
    };

    /// <summary>
    /// SpriteAsset 解決結果を表す。
    /// </summary>
    using SpriteAssetResolveResult = AssetResolveResult<SpriteAsset>;

    /// <summary>
    /// Texture2D 解決結果を表す。
    /// </summary>
    using Texture2DResolveResult = AssetResolveResult<Graphics::Texture2D>;

    /// <summary>
    /// AssetId から SpriteAsset と Texture2D を解決する最小マネージャ。
    /// </summary>
    class AssetManager
    {
    public:
        /// <summary>
        /// SpriteAsset を登録する。
        /// </summary>
        /// <param name="assetId">登録先の AssetId。</param>
        /// <param name="spriteAsset">登録する SpriteAsset。</param>
        /// <returns>登録に成功した場合は true。</returns>
        bool RegisterSpriteAsset(AssetId assetId, std::shared_ptr<SpriteAsset> spriteAsset);

        /// <summary>
        /// Texture2D から SpriteAsset を生成して登録する。
        /// </summary>
        /// <param name="assetId">登録先の AssetId。</param>
        /// <param name="texture">SpriteAsset に紐づける Texture2D。</param>
        /// <returns>登録に成功した場合は true。</returns>
        bool RegisterTexture2D(AssetId assetId, std::shared_ptr<Graphics::Texture2D> texture);

        /// <summary>
        /// AssetId から SpriteAsset を解決する。
        /// </summary>
        /// <param name="assetId">解決対象の AssetId。</param>
        /// <returns>解決結果。</returns>
        SpriteAssetResolveResult ResolveSpriteAsset(const AssetId& assetId) const;

        /// <summary>
        /// AssetId から Texture2D を解決する。
        /// </summary>
        /// <param name="assetId">解決対象の AssetId。</param>
        /// <returns>解決結果。</returns>
        Texture2DResolveResult ResolveTexture2D(const AssetId& assetId) const;

    private:
        static bool IsAssetIdValid(const AssetId& assetId);

        std::unordered_map<AssetId, std::shared_ptr<SpriteAsset>> m_spriteAssets;
    };
}
