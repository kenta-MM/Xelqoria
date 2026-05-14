#include "ScriptRuntimeSession.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>

#include "Assets/SpriteAsset.h"
#include "EditorStringUtils.h"
#include "SpriteComponent.h"

namespace Xelqoria::Editor
{
    namespace
    {
        constexpr const wchar_t* ScriptInstanceDirectory = L".xelqoria/ScriptInstances";

        /// <summary>
        /// Entity ごとの Script モジュールコピー先を構築する。
        /// </summary>
        /// <param name="projectRootDirectory">プロジェクトルートディレクトリ。</param>
        /// <param name="entityId">対象 Entity ID。</param>
        /// <param name="index">同名回避用インデックス。</param>
        /// <param name="sourceModulePath">元のモジュールファイル。</param>
        /// <returns>コピー先モジュールファイル。</returns>
        [[nodiscard]] std::filesystem::path BuildInstanceModulePath(
            const std::filesystem::path& projectRootDirectory,
            Game::EntityId entityId,
            std::size_t index,
            const std::filesystem::path& sourceModulePath)
        {
            const std::wstring extension = sourceModulePath.extension().empty()
                ? std::wstring(L".dll")
                : sourceModulePath.extension().wstring();
            return projectRootDirectory
                / ScriptInstanceDirectory
                / (L"entity_"
                    + std::to_wstring(static_cast<unsigned>(entityId))
                    + L"_"
                    + std::to_wstring(index)
                    + extension);
        }

        /// <summary>
        /// Script AssetId に対応するビルド成果物を取得する。
        /// </summary>
        /// <param name="buildResult">Script ビルド結果。</param>
        /// <param name="scriptAssetId">検索する Script AssetId。</param>
        /// <returns>見つかった成果物。未検出時は nullptr。</returns>
        [[nodiscard]] const ScriptBuildArtifact* FindArtifact(
            const ScriptBuildResult& buildResult,
            const Core::AssetId& scriptAssetId)
        {
            const auto found = std::find_if(
                buildResult.artifacts.begin(),
                buildResult.artifacts.end(),
                [&scriptAssetId](const ScriptBuildArtifact& artifact)
                {
                    return artifact.scriptAssetId == scriptAssetId;
                });
            return found == buildResult.artifacts.end() ? nullptr : &(*found);
        }
    }

    ScriptRuntimeSession::~ScriptRuntimeSession()
    {
        End();
    }

    std::vector<ScriptRuntimeInstancePlan> ScriptRuntimeSession::BuildInstancePlans(
        const Game::Scene& scene,
        const Game::Assets::ISpriteAssetResolver& spriteAssetResolver,
        const ScriptBuildResult& buildResult,
        const std::filesystem::path& projectRootDirectory)
    {
        std::vector<ScriptRuntimeInstancePlan> plans{};
        std::size_t index = 0;
        for (const Game::Entity& entity : scene.GetEntities())
        {
            const auto spriteComponent = entity.GetSpriteComponent();
            if (false == spriteComponent.has_value())
            {
                continue;
            }

            const auto spriteAsset = spriteAssetResolver.ResolveSpriteAsset(spriteComponent->get().spriteAssetRef);
            if (false == spriteAsset.has_value() || spriteAsset->scriptAssetId.IsEmpty())
            {
                continue;
            }

            const ScriptBuildArtifact* artifact = FindArtifact(buildResult, spriteAsset->scriptAssetId);
            if (nullptr == artifact)
            {
                continue;
            }

            plans.push_back(ScriptRuntimeInstancePlan{
                entity.GetId(),
                spriteAsset->scriptAssetId,
                artifact->modulePath,
                BuildInstanceModulePath(projectRootDirectory, entity.GetId(), index, artifact->modulePath)
            });
            ++index;
        }

        return plans;
    }

    bool ScriptRuntimeSession::Begin(
        const Game::Scene& scene,
        const Game::Assets::ISpriteAssetResolver& spriteAssetResolver,
        const ScriptBuildResult& buildResult,
        const std::filesystem::path& projectRootDirectory)
    {
        End();
        const std::vector<ScriptRuntimeInstancePlan> plans =
            BuildInstancePlans(scene, spriteAssetResolver, buildResult, projectRootDirectory);

        std::error_code errorCode;
        std::filesystem::create_directories(projectRootDirectory / ScriptInstanceDirectory, errorCode);
        if (errorCode)
        {
            AddDiagnostic(0, {}, L"Script インスタンス用フォルダを作成できません。");
            return false;
        }

        for (const ScriptRuntimeInstancePlan& plan : plans)
        {
            if (false == std::filesystem::exists(plan.sourceModulePath, errorCode) || errorCode)
            {
                AddDiagnostic(
                    plan.entityId,
                    plan.scriptAssetId,
                    L"Script 実行モジュールが見つかりません: " + plan.sourceModulePath.wstring());
                continue;
            }

            errorCode.clear();
            std::filesystem::copy_file(
                plan.sourceModulePath,
                plan.instanceModulePath,
                std::filesystem::copy_options::overwrite_existing,
                errorCode);
            if (errorCode)
            {
                AddDiagnostic(
                    plan.entityId,
                    plan.scriptAssetId,
                    L"Script 実行モジュールをインスタンス用にコピーできません。");
                continue;
            }

            HMODULE moduleHandle = LoadLibraryW(plan.instanceModulePath.c_str());
            if (nullptr == moduleHandle)
            {
                AddDiagnostic(
                    plan.entityId,
                    plan.scriptAssetId,
                    L"Script 実行モジュールをロードできません: " + plan.instanceModulePath.wstring());
                continue;
            }

            auto start = reinterpret_cast<StartFunction>(GetProcAddress(moduleHandle, "Start"));
            auto update = reinterpret_cast<UpdateFunction>(GetProcAddress(moduleHandle, "Update"));
            if (nullptr == start || nullptr == update)
            {
                FreeLibrary(moduleHandle);
                AddDiagnostic(
                    plan.entityId,
                    plan.scriptAssetId,
                    L"Script 実行モジュールに Start / Update が見つかりません。");
                continue;
            }

            m_instances.push_back(ScriptRuntimeInstance{
                plan.entityId,
                plan.scriptAssetId,
                plan.instanceModulePath,
                moduleHandle,
                start,
                update,
                false
            });
        }

        m_playing = true;
        return m_diagnostics.empty();
    }

    void ScriptRuntimeSession::Update(float deltaTime)
    {
        if (false == m_playing)
        {
            return;
        }

        for (ScriptRuntimeInstance& instance : m_instances)
        {
            if (false == instance.startCalled)
            {
                instance.start();
                instance.startCalled = true;
            }

            instance.update(deltaTime);
        }
    }

    void ScriptRuntimeSession::End()
    {
        for (ScriptRuntimeInstance& instance : m_instances)
        {
            if (nullptr != instance.moduleHandle)
            {
                FreeLibrary(instance.moduleHandle);
                instance.moduleHandle = nullptr;
            }
        }

        m_instances.clear();
        m_diagnostics.clear();
        m_playing = false;
    }

    bool ScriptRuntimeSession::IsPlaying() const
    {
        return m_playing;
    }

    std::span<const ScriptRuntimeDiagnostic> ScriptRuntimeSession::GetDiagnostics() const
    {
        return m_diagnostics;
    }

    void ScriptRuntimeSession::AddDiagnostic(
        Game::EntityId entityId,
        Core::AssetId scriptAssetId,
        std::wstring message)
    {
        m_diagnostics.push_back(ScriptRuntimeDiagnostic{
            entityId,
            std::move(scriptAssetId),
            std::move(message)
        });
    }
}
