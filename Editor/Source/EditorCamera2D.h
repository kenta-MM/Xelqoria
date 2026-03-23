#pragma once

#include <algorithm>
#include <cstdint>

namespace Xelqoria::Editor
{
    /// <summary>
    /// SceneView 内のスクリーン座標を表す。
    /// </summary>
    struct EditorScreenPoint
    {
        /// <summary>
        /// SceneView 左上基準の X 座標を表す。
        /// </summary>
        float x = 0.0f;

        /// <summary>
        /// SceneView 左上基準の Y 座標を表す。
        /// </summary>
        float y = 0.0f;
    };

    /// <summary>
    /// Editor 内で扱う 2D ワールド座標を表す。
    /// </summary>
    struct EditorWorldPoint
    {
        /// <summary>
        /// ワールド X 座標を表す。
        /// </summary>
        float x = 0.0f;

        /// <summary>
        /// ワールド Y 座標を表す。
        /// </summary>
        float y = 0.0f;
    };

    /// <summary>
    /// 2D SceneView で使用するビューポート設定を表す。
    /// </summary>
    struct EditorViewport
    {
        /// <summary>
        /// ビューポート幅を表す。
        /// </summary>
        std::uint32_t width = 0;

        /// <summary>
        /// ビューポート高さを表す。
        /// </summary>
        std::uint32_t height = 0;
    };

    /// <summary>
    /// SceneView の pan/zoom 状態と表示設定を保持する 2D カメラを表す。
    /// </summary>
    class EditorCamera2D
    {
    public:
        /// <summary>
        /// カメラ中心座標を設定する。
        /// </summary>
        /// <param name="centerX">設定する中心 X 座標。</param>
        /// <param name="centerY">設定する中心 Y 座標。</param>
        void SetCenter(float centerX, float centerY)
        {
            m_centerX = centerX;
            m_centerY = centerY;
        }

        /// <summary>
        /// カメラ中心 X 座標を取得する。
        /// </summary>
        /// <returns>現在の中心 X 座標。</returns>
        float GetCenterX() const
        {
            return m_centerX;
        }

        /// <summary>
        /// カメラ中心 Y 座標を取得する。
        /// </summary>
        /// <returns>現在の中心 Y 座標。</returns>
        float GetCenterY() const
        {
            return m_centerY;
        }

        /// <summary>
        /// 表示倍率を設定する。
        /// </summary>
        /// <param name="zoom">設定する倍率。</param>
        void SetZoom(float zoom)
        {
            m_zoom = (std::max)(zoom, 0.0001f);
        }

        /// <summary>
        /// 表示倍率を取得する。
        /// </summary>
        /// <returns>現在の表示倍率。</returns>
        float GetZoom() const
        {
            return m_zoom;
        }

        /// <summary>
        /// SceneView のビューポートサイズを設定する。
        /// </summary>
        /// <param name="width">ビューポート幅。</param>
        /// <param name="height">ビューポート高さ。</param>
        void SetViewport(std::uint32_t width, std::uint32_t height)
        {
            m_viewport.width = width;
            m_viewport.height = height;
        }

        /// <summary>
        /// 現在のビューポート設定を取得する。
        /// </summary>
        /// <returns>現在保持しているビューポート設定。</returns>
        EditorViewport GetViewport() const
        {
            return m_viewport;
        }

        /// <summary>
        /// ワールド X 座標を SceneView 描画座標へ変換する。
        /// </summary>
        /// <param name="worldX">変換対象のワールド X 座標。</param>
        /// <returns>描画に使用する SceneView 座標。</returns>
        float TransformWorldToViewX(float worldX) const
        {
            return (worldX - m_centerX) * m_zoom;
        }

        /// <summary>
        /// ワールド Y 座標を SceneView 描画座標へ変換する。
        /// </summary>
        /// <param name="worldY">変換対象のワールド Y 座標。</param>
        /// <returns>描画に使用する SceneView 座標。</returns>
        float TransformWorldToViewY(float worldY) const
        {
            return (worldY - m_centerY) * m_zoom;
        }

        /// <summary>
        /// スプライト拡大率へカメラ倍率を適用する。
        /// </summary>
        /// <param name="worldScale">ワールド上の拡大率。</param>
        /// <returns>描画に使用する拡大率。</returns>
        float TransformWorldScale(float worldScale) const
        {
            return worldScale * m_zoom;
        }

        /// <summary>
        /// SceneView のスクリーン座標をワールド座標へ変換する。
        /// </summary>
        /// <param name="screenPoint">SceneView 左上基準のスクリーン座標。</param>
        /// <returns>pan/zoom を加味したワールド座標。</returns>
        EditorWorldPoint TransformScreenToWorld(const EditorScreenPoint& screenPoint) const
        {
            const float halfWidth = static_cast<float>(m_viewport.width) * 0.5f;
            const float halfHeight = static_cast<float>(m_viewport.height) * 0.5f;

            return EditorWorldPoint{
                (screenPoint.x - halfWidth) / m_zoom + m_centerX,
                -(screenPoint.y - halfHeight) / m_zoom + m_centerY
            };
        }

    private:
        /// <summary>
        /// カメラ中心 X 座標を保持する。
        /// </summary>
        float m_centerX = 0.0f;

        /// <summary>
        /// カメラ中心 Y 座標を保持する。
        /// </summary>
        float m_centerY = 0.0f;

        /// <summary>
        /// 表示倍率を保持する。
        /// </summary>
        float m_zoom = 1.0f;

        /// <summary>
        /// SceneView のビューポート設定を保持する。
        /// </summary>
        EditorViewport m_viewport{};
    };
}
