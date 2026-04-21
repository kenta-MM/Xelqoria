#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "AssetId.h"
#include "Assets/SpriteAssetRegistry.h"
#include "EditorProject.h"
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
        /// 新規プロジェクトを作成し、現在の Scene を初期 Scene として保存する。
        /// </summary>
        /// <param name="projectName">作成するプロジェクト名。</param>
        /// <param name="parentDirectory">保存先親フォルダ。</param>
        /// <returns>作成に成功した場合は true。</returns>
        [[nodiscard]] bool CreateProject(
            const std::wstring& projectName,
            const std::filesystem::path& parentDirectory);

        /// <summary>
        /// 既存プロジェクトを開き、アクティブ Scene を読み込む。
        /// </summary>
        /// <param name="projectFilePath">開く `.proj` ファイル。</param>
        /// <returns>読込に成功した場合は true。</returns>
        [[nodiscard]] bool OpenProject(const std::filesystem::path& projectFilePath);

        /// <summary>
        /// 現在の Scene を別名プロジェクトとして保存する。
        /// </summary>
        /// <param name="projectName">新しいプロジェクト名。</param>
        /// <param name="parentDirectory">保存先親フォルダ。</param>
        /// <returns>保存に成功した場合は true。</returns>
        [[nodiscard]] bool SaveProjectAs(
            const std::wstring& projectName,
            const std::filesystem::path& parentDirectory);

        /// <summary>
        /// 現在のプロジェクト情報を取得する。
        /// </summary>
        /// <returns>プロジェクト情報。未オープン時は空。</returns>
        [[nodiscard]] const std::optional<EditorProjectInfo>& GetProjectInfo() const;

        /// <summary>
        /// 現在のプロジェクトで管理している Scene ファイルを列挙する。
        /// </summary>
        /// <returns>Scene ファイルパス一覧。</returns>
        [[nodiscard]] std::vector<std::filesystem::path> EnumerateProjectSceneFiles() const;

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
        /// 指定パスから Scene を読み込む。
        /// </summary>
        /// <param name="scenePath">読込対象 Scene ファイルパス。</param>
        /// <returns>読込に成功した場合は true。</returns>
        bool LoadSceneFromPath(const std::filesystem::path& scenePath);

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
        EditorProject m_project{};
    };
}
