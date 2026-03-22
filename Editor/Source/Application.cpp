#include "Application.h"

#include <algorithm>
#include <chrono>
#include <cwchar>
#include <memory>
#include <string>

#include "GraphicsAPI.h"
#include "RenderBackendBootstrap.h"

namespace Xelqoria::Editor
{
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

        return InitializeSceneViewGraphics();
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

        return m_hierarchyPanel != nullptr
            && m_assetsPanel != nullptr
            && m_inspectorPanel != nullptr
            && m_sceneViewPanel != nullptr
            && m_sceneViewPlanLabel != nullptr
            && m_sceneViewHost != nullptr
            && m_sceneViewSizeLabel != nullptr;
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
