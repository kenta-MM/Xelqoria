#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include "AssetId.h"
#include "Assets/SpriteAssetRegistry.h"
#include "IGraphicsContext.h"
#include "Scene.h"
#include "TextureAssetRegistry.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Editor が編集中の Scene と関連アセットを管理する。
    /// </summary>
    class EditorSceneDocument
    {
    public:
        /// <summary>
        /// 既定のアセット登録と Scene 読込を初期化する。
        /// </summary>
        /// <param name="graphicsContext">Texture 読込に使用する描画コンテキスト。</param>
        /// <returns>初期化に成功した場合は true。</returns>
        bool Initialize(RHI::IGraphicsContext& graphicsContext);

        /// <summary>
        /// 現在の Scene を取得する。
        /// </summary>
        /// <returns>編集中 Scene。未読込時は nullptr。</returns>
        [[nodiscard]] Game::Scene* GetScene();

        /// <summary>
        /// 現在の Scene を読み取り専用で取得する。
        /// </summary>
        /// <returns>編集中 Scene。未読込時は nullptr。</returns>
        [[nodiscard]] const Game::Scene* GetScene() const;

        /// <summary>
        /// 現在の Scene を別インスタンスへ置き換える。
        /// </summary>
        /// <param name="scene">置き換え後の Scene。</param>
        void ReplaceScene(std::unique_ptr<Game::Scene> scene);

        /// <summary>
        /// 現在の Scene を保存ファイルへ書き出す。
        /// </summary>
        /// <returns>保存に成功した場合は true。</returns>
        [[nodiscard]] bool Save() const;

        /// <summary>
        /// SpriteAsset レジストリを取得する。
        /// </summary>
        /// <returns>SpriteAsset レジストリ。</returns>
        [[nodiscard]] Game::Assets::SpriteAssetRegistry& GetSpriteAssetRegistry();

        /// <summary>
        /// SpriteAsset レジストリを読み取り専用で取得する。
        /// </summary>
        /// <returns>SpriteAsset レジストリ。</returns>
        [[nodiscard]] const Game::Assets::SpriteAssetRegistry& GetSpriteAssetRegistry() const;

        /// <summary>
        /// Texture レジストリを取得する。
        /// </summary>
        /// <returns>Texture レジストリ。</returns>
        [[nodiscard]] Graphics::TextureAssetRegistry& GetTextureAssetRegistry();

        /// <summary>
        /// Texture レジストリを読み取り専用で取得する。
        /// </summary>
        /// <returns>Texture レジストリ。</returns>
        [[nodiscard]] const Graphics::TextureAssetRegistry& GetTextureAssetRegistry() const;

        /// <summary>
        /// 登録済み SpriteAssetId 一覧を取得する。
        /// </summary>
        /// <returns>SpriteAssetId 一覧。</returns>
        [[nodiscard]] const std::vector<Core::AssetId>& GetRegisteredSpriteAssetIds() const;

    private:
        /// <summary>
        /// 保存先パスから Scene を読み込む。
        /// </summary>
        /// <returns>読込に成功した場合は true。</returns>
        bool LoadSceneDocument();

        /// <summary>
        /// 保存先 Scene ファイルパスを取得する。
        /// </summary>
        /// <returns>Scene ファイルパス。</returns>
        [[nodiscard]] std::filesystem::path GetSceneDocumentPath() const;

    private:
        std::unique_ptr<Game::Scene> m_scene;
        Game::Assets::SpriteAssetRegistry m_spriteAssetRegistry{};
        Graphics::TextureAssetRegistry m_textureAssetRegistry{};
        std::vector<Core::AssetId> m_registeredSpriteAssetIds{};
    };
}
