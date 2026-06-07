#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <Windows.h>

#include "AssetId.h"
#include "Assets/ISpriteAssetResolver.h"
#include "InputSystem.h"
#include "Entity.h"
#include "Scene.h"
#include "Assets/ScriptAssetService.h"

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
            Game::Scene& scene,
            const Game::Assets::ISpriteAssetResolver& spriteAssetResolver,
            const ScriptBuildResult& buildResult,
            const std::filesystem::path& projectRootDirectory);

        /// <summary>
        /// Script Sprite API と同じ規則で対象 Sprite の位置を更新する。
        /// </summary>
        /// <param name="scene">対象 Scene。</param>
        /// <param name="entityId">対象 Entity ID。</param>
        /// <param name="x">X 座標。</param>
        /// <param name="y">Y 座標。</param>
        /// <param name="z">Z 座標。</param>
        /// <returns>更新できた場合は true。</returns>
        [[nodiscard]] static bool SetSpritePosition(Game::Scene& scene, Game::EntityId entityId, float x, float y, float z);

        /// <summary>
        /// Script Sprite API と同じ規則で対象 Sprite の回転を更新する。
        /// </summary>
        /// <param name="scene">対象 Scene。</param>
        /// <param name="entityId">対象 Entity ID。</param>
        /// <param name="x">X 軸回転量。</param>
        /// <param name="y">Y 軸回転量。</param>
        /// <param name="z">Z 軸回転量。</param>
        /// <returns>更新できた場合は true。</returns>
        [[nodiscard]] static bool SetSpriteRotation(Game::Scene& scene, Game::EntityId entityId, float x, float y, float z);

        /// <summary>
        /// Script Sprite API と同じ規則で対象 Sprite の拡大率を更新する。
        /// </summary>
        /// <param name="scene">対象 Scene。</param>
        /// <param name="entityId">対象 Entity ID。</param>
        /// <param name="x">X 軸拡大率。</param>
        /// <param name="y">Y 軸拡大率。</param>
        /// <param name="z">Z 軸拡大率。</param>
        /// <returns>更新できた場合は true。</returns>
        [[nodiscard]] static bool SetSpriteScale(Game::Scene& scene, Game::EntityId entityId, float x, float y, float z);

        /// <summary>
        /// Script Sprite API と同じ規則で対象 Sprite の表示状態を更新する。
        /// </summary>
        /// <param name="scene">対象 Scene。</param>
        /// <param name="entityId">対象 Entity ID。</param>
        /// <param name="visible">表示する場合は true。</param>
        /// <returns>更新できた場合は true。</returns>
        [[nodiscard]] static bool SetSpriteVisible(Game::Scene& scene, Game::EntityId entityId, bool visible);

        /// <summary>
        /// Script Sprite API と同じ規則で対象 Sprite の色を更新する。
        /// </summary>
        /// <param name="scene">対象 Scene。</param>
        /// <param name="entityId">対象 Entity ID。</param>
        /// <param name="red">赤成分。</param>
        /// <param name="green">緑成分。</param>
        /// <param name="blue">青成分。</param>
        /// <param name="alpha">アルファ成分。</param>
        /// <returns>更新できた場合は true。</returns>
        [[nodiscard]] static bool SetSpriteColor(
            Game::Scene& scene,
            Game::EntityId entityId,
            float red,
            float green,
            float blue,
            float alpha);

        /// <summary>
        /// Script の Start と Update を実行する。
        /// </summary>
        /// <param name="deltaTime">前フレームからの経過時間（秒）。</param>
        /// <param name="inputSnapshot">現在フレームの入力状態。</param>
        void Update(float deltaTime, const Core::InputSnapshot& inputSnapshot);

        /// <summary>
        /// Script 実行セッションを一時停止する。
        /// </summary>
        void Pause();

        /// <summary>
        /// 一時停止中の Script 実行セッションを再開する。
        /// </summary>
        void Resume();

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
        /// Script 実行セッションが一時停止中かを取得する。
        /// </summary>
        /// <returns>一時停止中の場合は true。</returns>
        [[nodiscard]] bool IsPaused() const;

        /// <summary>
        /// Script Runtime 診断メッセージを取得する。
        /// </summary>
        /// <returns>診断メッセージ一覧。</returns>
        [[nodiscard]] std::span<const ScriptRuntimeDiagnostic> GetDiagnostics() const;

    private:
        using StartFunction = void (*)();
        using UpdateFunction = void (*)(float);
        struct ScriptSpriteApi;
        using SetSpriteApiFunction = void (*)(ScriptSpriteApi*);

        struct ScriptSpriteApi
        {
            void* context = nullptr;
            void (*reportError)(void* context, const char* message) = nullptr;
            void (*setPosition)(void* context, float x, float y, float z) = nullptr;
            void (*setRotation)(void* context, float x, float y, float z) = nullptr;
            void (*setScale)(void* context, float x, float y, float z) = nullptr;
            void (*setVisible)(void* context, int visible) = nullptr;
            void (*setColor)(void* context, float red, float green, float blue, float alpha) = nullptr;
            int (*isKeyDown)(void* context, int keyCode) = nullptr;
        };

        struct ScriptRuntimeInstance
        {
            ScriptRuntimeSession* owner = nullptr;
            Game::Scene* scene = nullptr;
            Game::EntityId entityId = 0;
            Core::AssetId scriptAssetId{};
            std::filesystem::path instanceModulePath{};
            HMODULE moduleHandle = nullptr;
            StartFunction start = nullptr;
            UpdateFunction update = nullptr;
            ScriptSpriteApi api{};
            bool startCalled = false;
            bool failed = false;
        };

        static void ReportErrorCallback(void* context, const char* message);
        static void SetPositionCallback(void* context, float x, float y, float z);
        static void SetRotationCallback(void* context, float x, float y, float z);
        static void SetScaleCallback(void* context, float x, float y, float z);
        static void SetVisibleCallback(void* context, int visible);
        static void SetColorCallback(void* context, float red, float green, float blue, float alpha);
        static int IsKeyDownCallback(void* context, int keyCode);

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
        const Core::InputSnapshot* m_currentInputSnapshot = nullptr;
        bool m_playing = false;
        bool m_paused = false;
    };
}
