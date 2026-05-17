#include "ScriptRuntimeSession.h"

#include <algorithm>
#include <exception>
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

        /// <summary>
        /// Script Runtime 例外を Editor 表示用メッセージへ変換する。
        /// </summary>
        /// <param name="functionName">実行中の Script 関数名。</param>
        /// <param name="message">例外メッセージ。</param>
        /// <returns>表示用メッセージ。</returns>
        [[nodiscard]] std::wstring BuildRuntimeExceptionMessage(
            const wchar_t* functionName,
            const char* message)
        {
            std::wstring result = L"Script ";
            result += functionName;
            result += L" 実行中に例外が発生しました";
            if (nullptr != message && '\0' != message[0])
            {
                result += L": ";
                result += ToWideString(message);
            }

            return result;
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

    bool ScriptRuntimeSession::SetSpritePosition(
        Game::Scene& scene,
        Game::EntityId entityId,
        float x,
        float y,
        float z)
    {
        const auto entity = scene.FindEntity(entityId);
        if (false == entity.has_value() || false == entity->get().HasSpriteComponent())
        {
            return false;
        }

        entity->get().GetTransform().SetPosition(x, y, z);
        return true;
    }

    bool ScriptRuntimeSession::SetSpriteRotation(
        Game::Scene& scene,
        Game::EntityId entityId,
        float x,
        float y,
        float z)
    {
        const auto entity = scene.FindEntity(entityId);
        if (false == entity.has_value() || false == entity->get().HasSpriteComponent())
        {
            return false;
        }

        entity->get().GetTransform().SetRotation(x, y, z);
        return true;
    }

    bool ScriptRuntimeSession::SetSpriteScale(
        Game::Scene& scene,
        Game::EntityId entityId,
        float x,
        float y,
        float z)
    {
        const auto entity = scene.FindEntity(entityId);
        if (false == entity.has_value() || false == entity->get().HasSpriteComponent())
        {
            return false;
        }

        entity->get().GetTransform().SetScale(x, y, z);
        return true;
    }

    bool ScriptRuntimeSession::SetSpriteVisible(
        Game::Scene& scene,
        Game::EntityId entityId,
        bool visible)
    {
        const auto entity = scene.FindEntity(entityId);
        if (false == entity.has_value())
        {
            return false;
        }

        const auto spriteComponent = entity->get().GetSpriteComponent();
        if (false == spriteComponent.has_value())
        {
            return false;
        }

        spriteComponent->get().renderSettings.visible = visible;
        return true;
    }

    bool ScriptRuntimeSession::SetSpriteColor(
        Game::Scene& scene,
        Game::EntityId entityId,
        float red,
        float green,
        float blue,
        float alpha)
    {
        const auto entity = scene.FindEntity(entityId);
        if (false == entity.has_value())
        {
            return false;
        }

        const auto spriteComponent = entity->get().GetSpriteComponent();
        if (false == spriteComponent.has_value())
        {
            return false;
        }

        spriteComponent->get().renderSettings.color = { red, green, blue, alpha };
        return true;
    }

    bool ScriptRuntimeSession::Begin(
        Game::Scene& scene,
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

        m_instances.reserve(plans.size());
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
            auto setSpriteApi =
                reinterpret_cast<SetSpriteApiFunction>(GetProcAddress(moduleHandle, "Xelqoria_SetSpriteApi"));
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
                this,
                &scene,
                plan.entityId,
                plan.scriptAssetId,
                plan.instanceModulePath,
                moduleHandle,
                start,
                update,
                {},
                false,
                false
            });
            ScriptRuntimeInstance& instance = m_instances.back();
            instance.api = ScriptSpriteApi{
                &instance,
                &ScriptRuntimeSession::ReportErrorCallback,
                &ScriptRuntimeSession::SetPositionCallback,
                &ScriptRuntimeSession::SetRotationCallback,
                &ScriptRuntimeSession::SetScaleCallback,
                &ScriptRuntimeSession::SetVisibleCallback,
                &ScriptRuntimeSession::SetColorCallback
            };

            if (nullptr != setSpriteApi)
            {
                setSpriteApi(&instance.api);
            }
        }

        m_playing = true;
        m_paused = false;
        return m_diagnostics.empty();
    }

    void ScriptRuntimeSession::Update(float deltaTime)
    {
        if (false == m_playing || true == m_paused)
        {
            return;
        }

        for (ScriptRuntimeInstance& instance : m_instances)
        {
            if (instance.failed)
            {
                continue;
            }

            if (false == instance.startCalled)
            {
                instance.startCalled = true;
                try
                {
                    instance.start();
                }
                catch (const std::exception& exception)
                {
                    instance.failed = true;
                    AddDiagnostic(
                        instance.entityId,
                        instance.scriptAssetId,
                        BuildRuntimeExceptionMessage(L"Start", exception.what()));
                    continue;
                }
                catch (...)
                {
                    instance.failed = true;
                    AddDiagnostic(
                        instance.entityId,
                        instance.scriptAssetId,
                        BuildRuntimeExceptionMessage(L"Start", nullptr));
                    continue;
                }
            }

            try
            {
                instance.update(deltaTime);
            }
            catch (const std::exception& exception)
            {
                instance.failed = true;
                AddDiagnostic(
                    instance.entityId,
                    instance.scriptAssetId,
                    BuildRuntimeExceptionMessage(L"Update", exception.what()));
            }
            catch (...)
            {
                instance.failed = true;
                AddDiagnostic(
                    instance.entityId,
                    instance.scriptAssetId,
                    BuildRuntimeExceptionMessage(L"Update", nullptr));
            }
        }
    }

    void ScriptRuntimeSession::Pause()
    {
        if (m_playing)
        {
            m_paused = true;
        }
    }

    void ScriptRuntimeSession::Resume()
    {
        if (m_playing)
        {
            m_paused = false;
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
        m_paused = false;
    }

    bool ScriptRuntimeSession::IsPlaying() const
    {
        return m_playing;
    }

    bool ScriptRuntimeSession::IsPaused() const
    {
        return m_paused;
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

    void ScriptRuntimeSession::ReportErrorCallback(void* context, const char* message)
    {
        ScriptRuntimeInstance* instance = static_cast<ScriptRuntimeInstance*>(context);
        if (nullptr == instance || nullptr == instance->owner)
        {
            return;
        }

        std::wstring diagnostic = L"Script からエラーが報告されました";
        if (nullptr != message && '\0' != message[0])
        {
            diagnostic += L": ";
            diagnostic += ToWideString(message);
        }

        instance->failed = true;
        instance->owner->AddDiagnostic(instance->entityId, instance->scriptAssetId, std::move(diagnostic));
    }

    void ScriptRuntimeSession::SetPositionCallback(void* context, float x, float y, float z)
    {
        ScriptRuntimeInstance* instance = static_cast<ScriptRuntimeInstance*>(context);
        if (nullptr == instance || nullptr == instance->owner || nullptr == instance->scene)
        {
            return;
        }

        if (false == SetSpritePosition(*instance->scene, instance->entityId, x, y, z))
        {
            instance->failed = true;
            instance->owner->AddDiagnostic(
                instance->entityId,
                instance->scriptAssetId,
                L"Script Sprite API の位置操作対象が見つかりません。");
        }
    }

    void ScriptRuntimeSession::SetRotationCallback(void* context, float x, float y, float z)
    {
        ScriptRuntimeInstance* instance = static_cast<ScriptRuntimeInstance*>(context);
        if (nullptr == instance || nullptr == instance->owner || nullptr == instance->scene)
        {
            return;
        }

        if (false == SetSpriteRotation(*instance->scene, instance->entityId, x, y, z))
        {
            instance->failed = true;
            instance->owner->AddDiagnostic(
                instance->entityId,
                instance->scriptAssetId,
                L"Script Sprite API の回転操作対象が見つかりません。");
        }
    }

    void ScriptRuntimeSession::SetScaleCallback(void* context, float x, float y, float z)
    {
        ScriptRuntimeInstance* instance = static_cast<ScriptRuntimeInstance*>(context);
        if (nullptr == instance || nullptr == instance->owner || nullptr == instance->scene)
        {
            return;
        }

        if (false == SetSpriteScale(*instance->scene, instance->entityId, x, y, z))
        {
            instance->failed = true;
            instance->owner->AddDiagnostic(
                instance->entityId,
                instance->scriptAssetId,
                L"Script Sprite API のスケール操作対象が見つかりません。");
        }
    }

    void ScriptRuntimeSession::SetVisibleCallback(void* context, int visible)
    {
        ScriptRuntimeInstance* instance = static_cast<ScriptRuntimeInstance*>(context);
        if (nullptr == instance || nullptr == instance->owner || nullptr == instance->scene)
        {
            return;
        }

        if (false == SetSpriteVisible(*instance->scene, instance->entityId, 0 != visible))
        {
            instance->failed = true;
            instance->owner->AddDiagnostic(
                instance->entityId,
                instance->scriptAssetId,
                L"Script Sprite API の表示操作対象が見つかりません。");
        }
    }

    void ScriptRuntimeSession::SetColorCallback(
        void* context,
        float red,
        float green,
        float blue,
        float alpha)
    {
        ScriptRuntimeInstance* instance = static_cast<ScriptRuntimeInstance*>(context);
        if (nullptr == instance || nullptr == instance->owner || nullptr == instance->scene)
        {
            return;
        }

        if (false == SetSpriteColor(*instance->scene, instance->entityId, red, green, blue, alpha))
        {
            instance->failed = true;
            instance->owner->AddDiagnostic(
                instance->entityId,
                instance->scriptAssetId,
                L"Script Sprite API の色操作対象が見つかりません。");
        }
    }
}
