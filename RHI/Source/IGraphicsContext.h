#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace Xelqoria::RHI
{
    class ITexture;

    /// <summary>
    /// プラットフォーム固有の描画先ウィンドウハンドルを表す。
    /// </summary>
    using NativeWindowHandle = void*;

    /// <summary>
    /// プラットフォーム固有のアプリケーションインスタンスハンドルを表す。
    /// </summary>
    using NativeInstanceHandle = void*;

    /// <summary>
    /// レンダリング基盤の初期化・フレーム制御・描画を抽象化する RHI インターフェース。
    /// </summary>
    class IGraphicsContext
    {
    public:
        virtual ~IGraphicsContext() = default;

        /// <summary>
        /// グラフィックスコンテキストを初期化する。
        /// </summary>
        /// <param name="windowHandle">描画対象のネイティブウィンドウハンドル。</param>
        /// <param name="instanceHandle">ネイティブアプリケーションインスタンスハンドル。</param>
        /// <param name="width">初期描画幅（ピクセル）。</param>
        /// <param name="height">初期描画高さ（ピクセル）。</param>
        /// <returns>初期化成功時は true。</returns>
        virtual bool Initialize(
            NativeWindowHandle windowHandle,
            NativeInstanceHandle instanceHandle,
            std::uint32_t width,
            std::uint32_t height) = 0;

        /// <summary>
        /// グラフィックスコンテキストを終了し、内部リソースを解放する。
        /// </summary>
        virtual void Shutdown() = 0;

        /// <summary>
        /// フレーム描画を開始する。
        /// </summary>
        virtual void BeginFrame() = 0;

        /// <summary>
        /// フレーム描画を終了する。
        /// </summary>
        virtual void EndFrame() = 0;

        /// <summary>
        /// ファイルからテクスチャを生成する。
        /// </summary>
        /// <param name="filePath">テクスチャファイルパス。</param>
        /// <returns>生成されたテクスチャ。失敗時は nullptr。</returns>
        virtual std::shared_ptr<ITexture> CreateTextureFromFile(const std::wstring& filePath) = 0;

        /// <summary>
        /// 指定スロットにテクスチャをバインドする。
        /// </summary>
        /// <param name="slot">バインド先スロット番号。</param>
        /// <param name="texture">バインドするテクスチャ。</param>
        virtual void BindTexture(std::uint32_t slot, ITexture* texture) = 0;

        /// <summary>
        /// 現在の描画パイプラインに渡す 32bit 浮動小数点定数を設定する。
        /// </summary>
        /// <param name="constants">設定する定数列。</param>
        virtual void SetShaderConstants(std::span<const float> constants) = 0;

        /// <summary>
        /// 非インデックス描画を実行する。
        /// </summary>
        /// <param name="vertexCount">描画頂点数。</param>
        /// <param name="startVertexLocation">開始頂点オフセット。</param>
        virtual void Draw(std::uint32_t vertexCount, std::uint32_t startVertexLocation = 0) = 0;

        /// <summary>
        /// インデックス描画を実行する。
        /// </summary>
        /// <param name="indexCount">描画インデックス数。</param>
        /// <param name="startIndexLocation">開始インデックスオフセット。</param>
        /// <param name="baseVertexLocation">ベース頂点オフセット。</param>
        virtual void DrawIndexed(std::uint32_t indexCount, std::uint32_t startIndexLocation = 0, std::int32_t baseVertexLocation = 0) = 0;

        /// <summary>
        /// 描画ターゲットサイズ変更時のリソースを再構築する。
        /// </summary>
        /// <param name="width">変更後の幅（ピクセル）。</param>
        /// <param name="height">変更後の高さ（ピクセル）。</param>
        virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;

        /// <summary>
        /// 現在の描画ターゲット幅を取得する。
        /// </summary>
        /// <returns>描画ターゲット幅。</returns>
        virtual std::uint32_t GetViewportWidth() const = 0;

        /// <summary>
        /// 現在の描画ターゲット高さを取得する。
        /// </summary>
        /// <returns>描画ターゲット高さ。</returns>
        virtual std::uint32_t GetViewportHeight() const = 0;
    };
}
