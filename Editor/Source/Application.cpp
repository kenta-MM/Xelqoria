#include "Application.h"

#include <algorithm>
#include <chrono>
#include <cwchar>
#include <memory>
#include <string>

#include "Assets/SpriteAsset.h"
#include "GraphicsAPI.h"
#include "RenderBackendBootstrap.h"
#include "Texture2D.h"

namespace Xelqoria::Editor
{
    namespace
    {
        std::wstring ToWideString(std::string_view value)
        {
            return std::wstring(value.begin(), value.end());
        }
    }

    Application::Application(HINSTANCE hInstance)
        : m_hInstance(hInstance)
    {
    }

    Application::~Application()
    {
        Shutdown();
    }

    int Application::Run()
    {
        if (!Initialize())
        {
            return -1;
        }

        using clock = std::chrono::steady_clock;
        auto lastTime = clock::now();

        while (m_running)
        {
            if (!m_window.PumpMessages())
            {
                m_running = false;
                continue;
            }

            const auto currentTime = clock::now();
            const std::chrono::duration<float> delta = currentTime - lastTime;
            lastTime = currentTime;

            Update(delta.count());
            Render();
        }

        return 0;
    }

    bool Application::Initialize()
    {
        constexpr auto clientWidth = 1600u;
        constexpr auto clientHeight = 900u;
        constexpr RHI::GraphicsAPI api = RHI::GraphicsAPI::D3D11;

        std::wstring title = L"Xelqoria Editor - ";
        title += RHI::GraphicsAPIToString(api);
        if (!m_window.Create(m_hInstance, title.c_str(), clientWidth, clientHeight))
        {
            return false;
        }

        if (!CreateEditorShell())
        {
            return false;
        }

        UpdateLayout();
        m_window.Show();

        if (!InitializeSceneViewGraphics())
        {
            return false;
        }

        if (!InitializeDocument())
        {
            return false;
        }

        RefreshAssetsPanel();
        RefreshHierarchyPanel();
        return true;
    }

    void Application::Shutdown()
    {
        if (m_graphics)
        {
            m_graphics->Shutdown();
            m_graphics.reset();
        }
    }

    void Application::Update(float deltaTime)
    {
        (void)deltaTime;
        UpdateLayout();
        SyncAssetSelection();
        SyncHierarchySelection();
    }

    void Application::Render()
    {
        if (!m_graphics)
        {
            return;
        }

        m_graphics->BeginFrame();
        m_graphics->EndFrame();
    }

    bool Application::CreateEditorShell()
    {
        m_defaultFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

        constexpr DWORD panelStyle = WS_CHILD | WS_VISIBLE | BS_GROUPBOX;
        m_hierarchyPanel = CreateChildWindow(L"Button", L"Hierarchy", panelStyle);
        m_assetsPanel = CreateChildWindow(L"Button", L"Assets", panelStyle);
        m_inspectorPanel = CreateChildWindow(L"Button", L"Inspector", panelStyle);
        m_sceneViewPanel = CreateChildWindow(L"Button", L"SceneView", panelStyle);
        m_sceneViewPlanLabel = CreateChildWindow(
            L"Static",
            L"Runtime 描画は SceneView 専用 child HWND に埋め込みます。",
            WS_CHILD | WS_VISIBLE);
        m_sceneViewHost = CreateChildWindow(
            L"Static",
            L"",
            WS_CHILD | WS_VISIBLE | SS_NOTIFY,
            WS_EX_CLIENTEDGE);
        m_sceneViewSizeLabel = CreateChildWindow(
            L"Static",
            L"SceneView size: pending",
            WS_CHILD | WS_VISIBLE);
        m_assetsSummaryLabel = CreateChildWindow(
            L"Static",
            L"Sprite assets: pending",
            WS_CHILD | WS_VISIBLE);
        m_assetsListBox = CreateChildWindow(
            L"ListBox",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_BORDER);
        m_hierarchySummaryLabel = CreateChildWindow(
            L"Static",
            L"Entities: pending",
            WS_CHILD | WS_VISIBLE);
        m_hierarchyListBox = CreateChildWindow(
            L"ListBox",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_BORDER);

        return m_hierarchyPanel != nullptr
            && m_assetsPanel != nullptr
            && m_inspectorPanel != nullptr
            && m_sceneViewPanel != nullptr
            && m_sceneViewPlanLabel != nullptr
            && m_sceneViewHost != nullptr
            && m_sceneViewSizeLabel != nullptr
            && m_assetsSummaryLabel != nullptr
            && m_assetsListBox != nullptr
            && m_hierarchySummaryLabel != nullptr
            && m_hierarchyListBox != nullptr;
    }

    void Application::UpdateLayout()
    {
        RECT clientRect{};
        GetClientRect(m_window.GetHwnd(), &clientRect);

        const int clientWidth = clientRect.right - clientRect.left;
        const int clientHeight = clientRect.bottom - clientRect.top;
        if (clientWidth <= 0 || clientHeight <= 0)
        {
            return;
        }

        constexpr int outerPadding = 12;
        constexpr int panelSpacing = 12;
        constexpr int leftPaneWidth = 260;
        constexpr int rightPaneWidth = 300;
        constexpr int groupHeaderHeight = 26;
        constexpr int hierarchyHeight = 280;
        constexpr int labelHeight = 24;

        const int centerX = outerPadding + leftPaneWidth + panelSpacing;
        const int centerWidth = (std::max)(320, clientWidth - leftPaneWidth - rightPaneWidth - (outerPadding * 2) - (panelSpacing * 2));
        const int rightX = centerX + centerWidth + panelSpacing;
        const int rightWidth = (std::max)(220, clientWidth - rightX - outerPadding);
        const int hierarchyPanelHeight = (std::max)(180, (std::min)(hierarchyHeight, clientHeight - (outerPadding * 2) - panelSpacing - 160));
        const int assetsPanelY = outerPadding + hierarchyPanelHeight + panelSpacing;
        const int assetsPanelHeight = (std::max)(140, clientHeight - assetsPanelY - outerPadding);
        const int scenePanelHeight = clientHeight - (outerPadding * 2);

        MoveWindow(m_hierarchyPanel, outerPadding, outerPadding, leftPaneWidth, hierarchyPanelHeight, TRUE);
        MoveWindow(m_assetsPanel, outerPadding, assetsPanelY, leftPaneWidth, assetsPanelHeight, TRUE);
        MoveWindow(m_sceneViewPanel, centerX, outerPadding, centerWidth, scenePanelHeight, TRUE);
        MoveWindow(m_inspectorPanel, rightX, outerPadding, rightWidth, scenePanelHeight, TRUE);

        const int sideInnerWidth = leftPaneWidth - (outerPadding * 2);
        MoveWindow(
            m_assetsSummaryLabel,
            outerPadding + outerPadding,
            assetsPanelY + groupHeaderHeight,
            sideInnerWidth,
            labelHeight,
            TRUE);
        MoveWindow(
            m_assetsListBox,
            outerPadding + outerPadding,
            assetsPanelY + groupHeaderHeight + labelHeight + 6,
            sideInnerWidth,
            (std::max)(100, assetsPanelHeight - groupHeaderHeight - labelHeight - outerPadding - 12),
            TRUE);
        MoveWindow(
            m_hierarchySummaryLabel,
            outerPadding + outerPadding,
            outerPadding + groupHeaderHeight,
            sideInnerWidth,
            labelHeight,
            TRUE);
        MoveWindow(
            m_hierarchyListBox,
            outerPadding + outerPadding,
            outerPadding + groupHeaderHeight + labelHeight + 6,
            sideInnerWidth,
            (std::max)(100, hierarchyPanelHeight - groupHeaderHeight - labelHeight - outerPadding - 12),
            TRUE);

        const int sceneInnerWidth = (std::max)(120, centerWidth - (outerPadding * 2));
        const int sceneHostHeight = (std::max)(160, scenePanelHeight - groupHeaderHeight - labelHeight - (outerPadding * 3));

        MoveWindow(
            m_sceneViewPlanLabel,
            centerX + outerPadding,
            outerPadding + groupHeaderHeight,
            sceneInnerWidth,
            labelHeight,
            TRUE);
        MoveWindow(
            m_sceneViewHost,
            centerX + outerPadding,
            outerPadding + groupHeaderHeight + labelHeight + panelSpacing,
            sceneInnerWidth,
            sceneHostHeight,
            TRUE);
        MoveWindow(
            m_sceneViewSizeLabel,
            centerX + outerPadding,
            outerPadding + groupHeaderHeight + labelHeight + panelSpacing + sceneHostHeight + 6,
            sceneInnerWidth,
            labelHeight,
            TRUE);

        RECT sceneHostRect{};
        GetClientRect(m_sceneViewHost, &sceneHostRect);
        const auto newWidth = static_cast<std::uint32_t>(sceneHostRect.right - sceneHostRect.left);
        const auto newHeight = static_cast<std::uint32_t>(sceneHostRect.bottom - sceneHostRect.top);

        if (newWidth != m_sceneViewWidth || newHeight != m_sceneViewHeight)
        {
            m_sceneViewWidth = newWidth;
            m_sceneViewHeight = newHeight;

            wchar_t sizeText[128]{};
            std::swprintf(
                sizeText,
                std::size(sizeText),
                L"SceneView size: %u x %u / host: child HWND",
                m_sceneViewWidth,
                m_sceneViewHeight);
            SetWindowTextW(m_sceneViewSizeLabel, sizeText);

            if (m_graphics && m_sceneViewWidth > 0 && m_sceneViewHeight > 0)
            {
                m_graphics->Resize(m_sceneViewWidth, m_sceneViewHeight);
            }
        }
    }

    bool Application::InitializeSceneViewGraphics()
    {
        constexpr RHI::GraphicsAPI api = RHI::GraphicsAPI::D3D11;

        m_graphics = BootstrapRenderBackend(api);
        if (!m_graphics)
        {
            return false;
        }

        if (m_sceneViewEmbeddingMode != SceneViewEmbeddingMode::ChildWindowSwapChainHost)
        {
            return false;
        }

        if (m_sceneViewHost == nullptr || m_sceneViewWidth == 0 || m_sceneViewHeight == 0)
        {
            return false;
        }

        return m_graphics->Initialize(
            m_sceneViewHost,
            m_hInstance,
            m_sceneViewWidth,
            m_sceneViewHeight);
    }

    bool Application::InitializeDocument()
    {
        m_scene = std::make_unique<Game::Scene>();

        auto spriteTexture = std::make_shared<Graphics::Texture2D>();
        if (!spriteTexture->LoadFromFile(L"../Resource\\mapchip.png", *m_graphics))
        {
            return false;
        }

        m_textureAssetRegistry.RegisterTexture("textures/mapchip", spriteTexture);

        m_registeredSpriteAssetIds.clear();
        m_registeredSpriteAssetIds.emplace_back("sprites/mapchip-left");
        m_registeredSpriteAssetIds.emplace_back("sprites/mapchip-right");
        m_registeredSpriteAssetIds.emplace_back("sprites/invalid-missing-texture");

        m_spriteAssetRegistry.RegisterSpriteAsset(
            m_registeredSpriteAssetIds[0],
            Game::Assets::SpriteAsset{ "textures/mapchip" });
        m_spriteAssetRegistry.RegisterSpriteAsset(
            m_registeredSpriteAssetIds[1],
            Game::Assets::SpriteAsset{ "textures/mapchip" });
        m_spriteAssetRegistry.RegisterSpriteAsset(
            m_registeredSpriteAssetIds[2],
            Game::Assets::SpriteAsset{ "textures/missing" });

        auto& firstEntity = m_scene->CreateEntity();
        firstEntity.GetTransform().SetPosition(-160.0f, 0.0f, 0.0f);
        firstEntity.SetSpriteComponent(Game::SpriteComponent{
            m_registeredSpriteAssetIds[0],
            {
                true,
                0,
                1.0f
            }
        });

        auto& secondEntity = m_scene->CreateEntity();
        secondEntity.GetTransform().SetPosition(160.0f, 90.0f, 0.0f);
        secondEntity.GetTransform().scale = { 0.75f, 0.75f, 1.0f };
        secondEntity.SetSpriteComponent(Game::SpriteComponent{
            m_registeredSpriteAssetIds[1],
            {
                true,
                1,
                1.0f
            }
        });

        return true;
    }

    void Application::RefreshAssetsPanel()
    {
        m_visibleSpriteAssetIds.clear();

        for (const auto& assetId : m_registeredSpriteAssetIds)
        {
            if (assetId.IsEmpty())
            {
                continue;
            }

            const auto spriteAsset = m_spriteAssetRegistry.ResolveSpriteAsset(assetId);
            if (!spriteAsset.has_value())
            {
                continue;
            }

            if (!m_textureAssetRegistry.ResolveTexture(spriteAsset->textureAssetId))
            {
                continue;
            }

            m_visibleSpriteAssetIds.push_back(assetId);
        }

        SendMessageW(m_assetsListBox, LB_RESETCONTENT, 0, 0);
        for (const auto& assetId : m_visibleSpriteAssetIds)
        {
            const auto text = ToWideString(assetId.GetValue());
            SendMessageW(m_assetsListBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
        }

        if (m_selectedSpriteAssetId.IsEmpty() && !m_visibleSpriteAssetIds.empty())
        {
            m_selectedSpriteAssetId = m_visibleSpriteAssetIds.front();
        }

        int selectedIndex = LB_ERR;
        for (std::size_t index = 0; index < m_visibleSpriteAssetIds.size(); ++index)
        {
            if (m_visibleSpriteAssetIds[index] == m_selectedSpriteAssetId)
            {
                selectedIndex = static_cast<int>(index);
                break;
            }
        }

        if (selectedIndex != LB_ERR)
        {
            SendMessageW(m_assetsListBox, LB_SETCURSEL, static_cast<WPARAM>(selectedIndex), 0);
        }

        wchar_t summaryText[128]{};
        std::swprintf(
            summaryText,
            std::size(summaryText),
            L"Sprite assets: %u visible / %u registered",
            static_cast<unsigned>(m_visibleSpriteAssetIds.size()),
            static_cast<unsigned>(m_registeredSpriteAssetIds.size()));
        SetWindowTextW(m_assetsSummaryLabel, summaryText);
    }

    void Application::SyncAssetSelection()
    {
        if (m_assetsListBox == nullptr)
        {
            return;
        }

        const LRESULT selectedIndex = SendMessageW(m_assetsListBox, LB_GETCURSEL, 0, 0);
        if (selectedIndex == LB_ERR)
        {
            return;
        }

        const auto index = static_cast<std::size_t>(selectedIndex);
        if (index >= m_visibleSpriteAssetIds.size())
        {
            return;
        }

        m_selectedSpriteAssetId = m_visibleSpriteAssetIds[index];
    }

    void Application::RefreshHierarchyPanel()
    {
        m_visibleEntityIds.clear();

        SendMessageW(m_hierarchyListBox, LB_RESETCONTENT, 0, 0);
        if (m_scene)
        {
            for (const auto& entity : m_scene->GetEntities())
            {
                m_visibleEntityIds.push_back(entity.GetId());

                std::wstring label = L"Entity ";
                label += std::to_wstring(entity.GetId());

                if (const auto spriteComponent = entity.GetSpriteComponent(); spriteComponent.has_value())
                {
                    label += L" (";
                    label += ToWideString(spriteComponent->get().spriteAssetRef.GetValue());
                    label += L")";
                }

                SendMessageW(m_hierarchyListBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
            }
        }

        if (!m_selectedEntityId.has_value() && !m_visibleEntityIds.empty())
        {
            m_selectedEntityId = m_visibleEntityIds.front();
        }

        int selectedIndex = LB_ERR;
        for (std::size_t index = 0; index < m_visibleEntityIds.size(); ++index)
        {
            if (m_selectedEntityId.has_value() && m_visibleEntityIds[index] == *m_selectedEntityId)
            {
                selectedIndex = static_cast<int>(index);
                break;
            }
        }

        if (selectedIndex != LB_ERR)
        {
            SendMessageW(m_hierarchyListBox, LB_SETCURSEL, static_cast<WPARAM>(selectedIndex), 0);
        }

        wchar_t summaryText[128]{};
        std::swprintf(
            summaryText,
            std::size(summaryText),
            L"Entities: %u / selected: %u",
            static_cast<unsigned>(m_visibleEntityIds.size()),
            m_selectedEntityId.has_value() ? static_cast<unsigned>(*m_selectedEntityId) : 0u);
        SetWindowTextW(m_hierarchySummaryLabel, summaryText);
    }

    void Application::SyncHierarchySelection()
    {
        if (m_hierarchyListBox == nullptr)
        {
            return;
        }

        const LRESULT selectedIndex = SendMessageW(m_hierarchyListBox, LB_GETCURSEL, 0, 0);
        if (selectedIndex == LB_ERR)
        {
            return;
        }

        const auto index = static_cast<std::size_t>(selectedIndex);
        if (index >= m_visibleEntityIds.size())
        {
            return;
        }

        const auto entityId = m_visibleEntityIds[index];
        if (!m_selectedEntityId.has_value() || *m_selectedEntityId != entityId)
        {
            m_selectedEntityId = entityId;
            RefreshHierarchyPanel();
        }
    }

    HWND Application::CreateChildWindow(const wchar_t* className, const wchar_t* text, DWORD style, DWORD exStyle) const
    {
        HWND handle = CreateWindowExW(
            exStyle,
            className,
            text,
            style,
            0,
            0,
            0,
            0,
            m_window.GetHwnd(),
            nullptr,
            m_hInstance,
            nullptr);

        if (handle != nullptr && m_defaultFont != nullptr)
        {
            SendMessageW(handle, WM_SETFONT, reinterpret_cast<WPARAM>(m_defaultFont), TRUE);
        }

        return handle;
    }
}
