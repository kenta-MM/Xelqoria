#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <span>

#include <d3d11.h>
#include <wrl/client.h>

#include "IGraphicsContext.h"
#include "ITexture.h"
#include "IVertexBuffer.h"

namespace Xelqoria::Backends::D3D11
{
    /// <summary>
    /// Direct3D 11 のスプライト描画用パイプライン資産を管理する。
    /// </summary>
    class D3D11SpritePipeline
    {
    public:
        /// <summary>
        /// スプライト描画用資産を初期化する。
        /// </summary>
        /// <param name="device">初期化に使用する D3D11 デバイス。</param>
        /// <returns>初期化に成功した場合は true。</returns>
        bool Initialize(ID3D11Device* device);

        /// <summary>
        /// 保持している描画資産を解放する。
        /// </summary>
        void Shutdown();

        /// <summary>
        /// 指定スロットへスプライト描画用テクスチャをバインドする。
        /// </summary>
        /// <param name="deviceContext">描画に使用するデバイスコンテキスト。</param>
        /// <param name="slot">バインド先スロット番号。</param>
        /// <param name="texture">バインドするテクスチャ。</param>
        void BindTexture(ID3D11DeviceContext* deviceContext, std::uint32_t slot, RHI::ITexture* texture);

        /// <summary>
        /// スプライト描画に使用するシェーダー定数を設定する。
        /// </summary>
        /// <param name="constants">設定する定数列。</param>
        void SetShaderConstants(std::span<const float> constants);

        /// <summary>
        /// スプライト描画を実行する。
        /// </summary>
        /// <param name="deviceContext">描画に使用するデバイスコンテキスト。</param>
        /// <param name="vertexCount">描画頂点数。</param>
        /// <param name="startVertexLocation">開始頂点オフセット。</param>
        void Draw(ID3D11DeviceContext* deviceContext, std::uint32_t vertexCount, std::uint32_t startVertexLocation);

    private:
        /// <summary>
        /// スプライト描画用シェーダーと固定機能状態を生成する。
        /// </summary>
        bool CreateShaderState(ID3D11Device* device);

        /// <summary>
        /// スプライト描画用ジオメトリを生成する。
        /// </summary>
        bool CreateGeometry(ID3D11Device* device);

    private:
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_transformBuffer;
        std::shared_ptr<RHI::IVertexBuffer> m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;
        bool m_hasBoundTexture = false;
        std::array<float, 20> m_shaderConstants{};
    };
}
