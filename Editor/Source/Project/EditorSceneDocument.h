#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "AssetId.h"
#include "Assets/Collider2DAssetRegistry.h"
#include "Assets/SpriteAssetRegistry.h"
#include "Assets/SpriteMaterialAssetRegistry.h"
#include "Collider2DComponent.h"
#include "Entity.h"
#include "Project/EditorProject.h"
#include "IGraphicsContext.h"
#include "Scene.h"
#include "Assets/ScriptAssetService.h"
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
        /// <param name="projectFilePath">開くプロジェクトファイル。</param>
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
        /// 現在のプロジェクト内 Scene を開く。
        /// </summary>
        /// <param name="scenePath">開く Scene ファイルパス。</param>
        /// <returns>読込に成功した場合は true。</returns>
        [[nodiscard]] bool OpenProjectScene(const std::filesystem::path& scenePath);

        /// <summary>
        /// プロジェクト配下の画像ファイルを Texture/SpriteAsset として登録する。
        /// </summary>
        void RefreshProjectAssetRegistries();

        /// <summary>
        /// 指定画像ファイルを Texture/SpriteAsset として登録する。
        /// </summary>
        /// <param name="imagePath">登録対象の画像ファイルパス。</param>
        /// <returns>登録に成功した場合は true。</returns>
        [[nodiscard]] bool RegisterImageAsset(const std::filesystem::path& imagePath);

        /// <summary>
        /// 指定 Sprite Asset ファイルを SpriteAsset として登録する。
        /// </summary>
        /// <param name="spriteAssetPath">登録対象の Sprite Asset ファイルパス。</param>
        /// <returns>登録に成功した場合は true。</returns>
        [[nodiscard]] bool RegisterSpriteAssetFile(const std::filesystem::path& spriteAssetPath);

        /// <summary>
        /// 指定 Material Asset ファイルを SpriteMaterialAsset として登録する。
        /// </summary>
        /// <param name="materialAssetPath">登録対象の Material Asset ファイルパス。</param>
        /// <returns>登録に成功した場合は true。</returns>
        [[nodiscard]] bool RegisterMaterialAssetFile(const std::filesystem::path& materialAssetPath);

        /// <summary>
        /// 指定 Collider2D Asset ファイルを Collider2DAsset として登録する。
        /// </summary>
        /// <param name="collider2DAssetPath">登録対象の Collider2D Asset ファイルパス。</param>
        /// <returns>登録に成功した場合は true。</returns>
        [[nodiscard]] bool RegisterCollider2DAssetFile(const std::filesystem::path& collider2DAssetPath);

        /// <summary>
        /// 現在のプロジェクト配下へ Material Asset ファイルを作成する。
        /// </summary>
        /// <param name="targetDirectory">作成先フォルダ。空またはプロジェクト外の場合は Assets 直下。</param>
        /// <param name="textureAssetId">初期 TextureAssetId。</param>
        /// <returns>作成した MaterialAssetId。作成できない場合は空。</returns>
        [[nodiscard]] std::optional<Core::AssetId> CreateMaterialAssetFile(
            const std::filesystem::path& targetDirectory,
            const Core::AssetId& textureAssetId = {});

        /// <summary>
        /// 現在のプロジェクト配下へ Collider2D Asset ファイルを作成する。
        /// </summary>
        /// <param name="targetDirectory">作成先フォルダ。空またはプロジェクト外の場合は Assets 直下。</param>
        /// <param name="collider">初期 Collider2D 値。</param>
        /// <returns>作成した Collider2DAssetId。作成できない場合は空。</returns>
        [[nodiscard]] std::optional<Core::AssetId> CreateCollider2DAssetFile(
            const std::filesystem::path& targetDirectory,
            const Game::Collider2DComponent& collider = {});

        /// <summary>
        /// MaterialAssetId から対応する Material Asset ファイルパスを解決する。
        /// </summary>
        /// <param name="materialAssetId">解決対象 MaterialAssetId。</param>
        /// <returns>Material Asset ファイルパス。解決できない場合は空。</returns>
        [[nodiscard]] std::optional<std::filesystem::path> ResolveMaterialAssetPath(
            const Core::AssetId& materialAssetId) const;

        /// <summary>
        /// Collider2DAssetId から対応する Collider2D Asset ファイルパスを解決する。
        /// </summary>
        /// <param name="collider2DAssetId">解決対象 Collider2DAssetId。</param>
        /// <returns>Collider2D Asset ファイルパス。解決できない場合は空。</returns>
        [[nodiscard]] std::optional<std::filesystem::path> ResolveCollider2DAssetPath(
            const Core::AssetId& collider2DAssetId) const;

        /// <summary>
        /// Material Asset ファイルの内容を更新する。
        /// </summary>
        /// <param name="materialAssetId">更新対象 MaterialAssetId。</param>
        /// <param name="materialAsset">保存する Material 内容。</param>
        /// <returns>更新に成功した場合は true。</returns>
        [[nodiscard]] bool SaveMaterialAsset(
            const Core::AssetId& materialAssetId,
            const Game::Assets::SpriteMaterialAsset& materialAsset);

        /// <summary>
        /// Material Asset の Texture が未設定の場合のみ指定 Texture を保存する。
        /// </summary>
        /// <param name="materialAssetId">更新対象 MaterialAssetId。</param>
        /// <param name="textureAssetId">補完する TextureAssetId。</param>
        /// <returns>既に設定済み、または補完に成功した場合は true。</returns>
        [[nodiscard]] bool EnsureMaterialAssetTexture(
            const Core::AssetId& materialAssetId,
            const Core::AssetId& textureAssetId);

        /// <summary>
        /// Sprite Asset ファイルへ Collider2D Asset の割り当てを保存する。
        /// </summary>
        /// <param name="spriteAssetId">割り当て先 SpriteAssetId。</param>
        /// <param name="collider2DAssetId">割り当てる Collider2DAssetId。</param>
        /// <returns>保存に成功した場合は true。</returns>
        [[nodiscard]] bool AssignCollider2DAssetToSpriteAsset(
            const Core::AssetId& spriteAssetId,
            const Core::AssetId& collider2DAssetId);

        /// <summary>
        /// SpriteAssetId と同じフォルダへ Collider2D Asset を作成し割り当てる。
        /// </summary>
        /// <param name="spriteAssetId">割り当て先 SpriteAssetId。</param>
        /// <param name="collider">初期 Collider2D 値。</param>
        /// <returns>作成し割り当てた Collider2DAssetId。失敗時は空。</returns>
        [[nodiscard]] std::optional<Core::AssetId> CreateAndAssignCollider2DAssetToSpriteAsset(
            const Core::AssetId& spriteAssetId,
            const Game::Collider2DComponent& collider);

        /// <summary>
        /// Scene 内の旧 SpriteAsset 参照から Material 参照を自動作成する。
        /// </summary>
        /// <returns>Scene が変更された場合は true。</returns>
        [[nodiscard]] bool MigrateSpriteComponentsToMaterialAssets();

        /// <summary>
        /// SpriteAssetId から対応する Sprite Asset ファイルパスを解決する。
        /// </summary>
        /// <param name="spriteAssetId">解決対象 SpriteAssetId。</param>
        /// <returns>Sprite Asset ファイルパス。解決できない場合は空。</returns>
        [[nodiscard]] std::optional<std::filesystem::path> ResolveSpriteAssetPath(
            const Core::AssetId& spriteAssetId) const;

        /// <summary>
        /// Entity の Sprite 情報に対応する Sprite アセットファイルをプロジェクト配下へ作成する。
        /// </summary>
        /// <param name="entity">保存元の Sprite Entity。</param>
        /// <param name="targetDirectory">作成先フォルダ。空またはプロジェクト外の場合は Assets 直下。</param>
        /// <returns>作成または既存ファイル確認に成功した場合は true。</returns>
        [[nodiscard]] bool CreateSpriteAssetFile(
            const Game::Entity& entity,
            const std::filesystem::path& targetDirectory);

        /// <summary>
        /// Entity の SpriteComponent が実体 `.sprite` を参照するようにし、必要なら Sprite アセットファイルを作成する。
        /// </summary>
        /// <param name="entity">更新対象の Sprite Entity。</param>
        /// <param name="targetDirectory">作成先フォルダ。空またはプロジェクト外の場合は Assets 直下。</param>
        /// <returns>参照先 SpriteAssetId。作成または更新できない場合は空。</returns>
        [[nodiscard]] std::optional<Core::AssetId> EnsureSpriteAssetFileForEntity(
            Game::Entity& entity,
            const std::filesystem::path& targetDirectory);

        /// <summary>
        /// Sprite Asset ファイルへ Script Asset の割り当てを保存する。
        /// </summary>
        /// <param name="spriteAssetPath">割り当て先 Sprite Asset ファイルパス。</param>
        /// <param name="scriptAssetPath">割り当てる Script Asset ファイルパス。</param>
        /// <returns>保存に成功した場合は true。</returns>
        [[nodiscard]] bool AssignScriptAssetToSpriteAssetFile(
            const std::filesystem::path& spriteAssetPath,
            const std::filesystem::path& scriptAssetPath);

        /// <summary>
        /// SpriteAssetId で指定した Sprite Asset へ Script Asset を割り当てる。
        /// </summary>
        /// <param name="spriteAssetId">割り当て先 SpriteAssetId。</param>
        /// <param name="scriptAssetPath">割り当てる Script Asset ファイルパス。</param>
        /// <returns>保存に成功した場合は true。</returns>
        [[nodiscard]] bool AssignScriptAssetToSpriteAsset(
            const Core::AssetId& spriteAssetId,
            const std::filesystem::path& scriptAssetPath);

        /// <summary>
        /// SpriteAssetId で指定した Sprite Asset の Script Asset 割り当てを解除する。
        /// </summary>
        /// <param name="spriteAssetId">解除対象 SpriteAssetId。</param>
        /// <returns>保存に成功した場合は true。</returns>
        [[nodiscard]] bool ClearScriptAssetFromSpriteAsset(const Core::AssetId& spriteAssetId);

        /// <summary>
        /// SpriteAssetId で指定した Sprite Asset と同じフォルダへ Script Asset を作成し割り当てる。
        /// </summary>
        /// <param name="spriteAssetId">割り当て先 SpriteAssetId。</param>
        /// <returns>Script Asset 作成結果。割り当て失敗時は succeeded=false。</returns>
        [[nodiscard]] ScriptAssetCreationResult CreateAndAssignScriptAssetToSpriteAsset(
            const Core::AssetId& spriteAssetId);

        /// <summary>
        /// 現在のプロジェクト配下へ Script Asset と初期 C++ コードを作成する。
        /// </summary>
        /// <param name="targetDirectory">Script Asset 作成先フォルダ。空またはプロジェクト外の場合は作成しない。</param>
        /// <returns>Script Asset 作成結果。</returns>
        [[nodiscard]] ScriptAssetCreationResult CreateScriptAssetFile(
            const std::filesystem::path& targetDirectory);

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
        /// MaterialAsset レジストリを取得する。
        /// </summary>
        /// <returns>MaterialAsset レジストリ。</returns>
        [[nodiscard]] Game::Assets::SpriteMaterialAssetRegistry& GetMaterialAssetRegistry();

        /// <summary>
        /// MaterialAsset レジストリを読み取り専用で取得する。
        /// </summary>
        /// <returns>MaterialAsset レジストリ。</returns>
        [[nodiscard]] const Game::Assets::SpriteMaterialAssetRegistry& GetMaterialAssetRegistry() const;

        /// <summary>
        /// Collider2DAsset レジストリを取得する。
        /// </summary>
        /// <returns>Collider2DAsset レジストリ。</returns>
        [[nodiscard]] Game::Assets::Collider2DAssetRegistry& GetCollider2DAssetRegistry();

        /// <summary>
        /// Collider2DAsset レジストリを読み取り専用で取得する。
        /// </summary>
        /// <returns>Collider2DAsset レジストリ。</returns>
        [[nodiscard]] const Game::Assets::Collider2DAssetRegistry& GetCollider2DAssetRegistry() const;

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
        RHI::IGraphicsContext* m_graphicsContext = nullptr;
        Game::Assets::SpriteAssetRegistry m_spriteAssetRegistry{};
        Game::Assets::SpriteMaterialAssetRegistry m_materialAssetRegistry{};
        Game::Assets::Collider2DAssetRegistry m_collider2DAssetRegistry{};
        Graphics::TextureAssetRegistry m_textureAssetRegistry{};
        std::vector<Core::AssetId> m_registeredSpriteAssetIds{};
        std::vector<Core::AssetId> m_registeredMaterialAssetIds{};
        std::vector<Core::AssetId> m_registeredCollider2DAssetIds{};
        EditorProject m_project{};
    };
}
