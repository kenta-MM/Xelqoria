#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <Windows.h>

#include "AssetId.h"
#include "Assets/ISpriteAssetResolver.h"
#include "Entity.h"
#include "Scene.h"
#include "ScriptAssetService.h"

namespace Xelqoria::Editor
{
    /// <summary>
    /// Script Runtime の診断メッセージを表す。
    /// </summary>
    struct ScriptRuntimeDiagnostic
    {
        /// <summary>
        /// 対象 Entity ID を表す。
        /// </summary>
        Game::EntityId entityId = 0;

        /// <summary>
        /// 対象 Script Asset 識別子を表す。
        /// </summary>
        Core::AssetId scriptAssetId{};

        /// <summary>
        /// Editor に表示する診断メッセージを表す。
        /// </summary>
        std::wstring message{};
    };

    /// <summary>
    /// Script 実行インスタンスの作成計画を表す。
    /// </summary>
    struct ScriptRuntimeInstancePlan
    {
        /// <summary>
        /// 対象 Entity ID を表す。
        /// </summary>
        Game::EntityId entityId = 0;

        /// <summary>
        /// 対象 Script Asset 識別子を表す。
        /// </summary>
        Core::AssetId scriptAssetId{};

        /// <summary>
        /// ビルド済み Script モジュールを表す。
        /// </summary>
        std::filesystem::path sourceModulePath{};

        /// <summary>
        /// Entity インスタンスごとのロード用コピーを表す。
        /// </summary>
        std::filesystem::path instanceModulePath{};
    };

    /// <summary>
    /// Editor 再生中の Script Start / Update 実行状態を管理する。
    /// </summary>
    class ScriptRuntimeSession
    {
    public:
        /// <summary>
        /// Script Runtime Session を破棄する。
        /// </summary>
        ~ScriptRuntimeSession();

        /// <summary>
        /// Scene 内の Script 付き Sprite から実行計画を構築する。
        /// </summary>
        /// <param name="scene">対象 Scene。</param>
        /// <param name="spriteAssetResolver">SpriteAsset 解決器。</param>
        /// <param name="buildResult">Script ビルド結果。</param>
        /// <param name="projectRootDirectory">プロジェクトルートディレクトリ。</param>
        /// <returns>Script 実行計画。</returns>
        [[nodiscard]] static std::vector<ScriptRuntimeInstancePlan> BuildInstancePlans(
            const Game::Scene& scene,
            const Game::Assets::ISpriteAssetResolver& spriteAssetResolver,
            const ScriptBuildResult& buildResult,
            const std::filesystem::path& projectRootDirectory);

        /// <summary>
        /// Editor 再生用 Script 実行セッションを開始する。
        /// </summary>
        /// <param name="scene">対象 Scene。</param>
        /// <param name="spriteAssetResolver">SpriteAsset 解決器。</param>
        /// <param name="buildResult">Script ビルド結果。</param>
        /// <param name="projectRootDirectory">プロジェクトルートディレクトリ。</param>
        /// <returns>開始に成功した場合は true。</returns>
        [[nodiscard]] bool Begin(
            const Game::Scene& scene,
            const Game::Assets::ISpriteAssetResolver& spriteAssetResolver,
            const ScriptBuildResult& buildResult,
            const std::filesystem::path& projectRootDirectory);

        /// <summary>
        /// Script の Start と Update を実行する。
        /// </summary>
        /// <param name="deltaTime">前フレームからの経過時間（秒）。</param>
        void Update(float deltaTime);

        /// <summary>
        /// Script 実行セッションを終了する。
        /// </summary>
        void End();

        /// <summary>
        /// 再生中かを取得する。
        /// </summary>
        /// <returns>再生中の場合は true。</returns>
        [[nodiscard]] bool IsPlaying() const;

        /// <summary>
        /// Script Runtime 診断メッセージを取得する。
        /// </summary>
        /// <returns>診断メッセージ一覧。</returns>
        [[nodiscard]] std::span<const ScriptRuntimeDiagnostic> GetDiagnostics() const;

    private:
        using StartFunction = void (*)();
        using UpdateFunction = void (*)(float);

        struct ScriptRuntimeInstance
        {
            Game::EntityId entityId = 0;
            Core::AssetId scriptAssetId{};
            std::filesystem::path instanceModulePath{};
            HMODULE moduleHandle = nullptr;
            StartFunction start = nullptr;
            UpdateFunction update = nullptr;
            bool startCalled = false;
        };

        /// <summary>
        /// Script Runtime 診断メッセージを追加する。
        /// </summary>
        /// <param name="entityId">対象 Entity ID。</param>
        /// <param name="scriptAssetId">対象 Script Asset 識別子。</param>
        /// <param name="message">診断メッセージ。</param>
        void AddDiagnostic(
            Game::EntityId entityId,
            Core::AssetId scriptAssetId,
            std::wstring message);

        std::vector<ScriptRuntimeInstance> m_instances{};
        std::vector<ScriptRuntimeDiagnostic> m_diagnostics{};
        bool m_playing = false;
    };
}
