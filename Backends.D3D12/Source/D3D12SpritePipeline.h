#pragma once

#include <cstdint>
#include <memory>

#include <d3d12.h>
#include <wrl/client.h>

#include "IGraphicsContext.h"
#include "ITexture.h"
#include "IVertexBuffer.h"

namespace Xelqoria::Backends::D3D12
{
    /// <summary>
    /// Direct3D 12 のスプライト描画用パイプライン資産を管理する。
    /// </summary>
    class D3D12SpritePipeline
    {
    public:
        /// <summary>
        /// スプライト描画用資産を初期化する。
        /// </summary>
        /// <param name="device">初期化に使用する D3D12 デバイス。</param>
        /// <returns>初期化に成功した場合は true。</returns>
        bool Initialize(ID3D12Device* device);

        /// <summary>
        /// 保持している描画資産を解放する。
        /// </summary>
        void Shutdown();

        /// <summary>
        /// 指定スロットへスプライト描画用テクスチャをバインドする。
        /// </summary>
        /// <param name="device">SRV 更新に使用する D3D12 デバイス。</param>
        /// <param name="slot">バインド先スロット番号。</param>
        /// <param name="texture">バインドするテクスチャ。</param>
        void BindTexture(ID3D12Device* device, std::uint32_t slot, RHI::ITexture* texture);

        /// <summary>
        /// スプライト描画に使用する 2D 変換値を設定する。
        /// </summary>
        /// <param name="transform">設定する変換値。</param>
        void SetQuadTransform(const RHI::QuadTransform2D& transform);

        /// <summary>
        /// スプライト描画を実行する。
        /// </summary>
        /// <param name="commandList">描画に使用するコマンドリスト。</param>
        /// <param name="vertexCount">描画頂点数。</param>
        /// <param name="startVertexLocation">開始頂点オフセット。</param>
        void Draw(ID3D12GraphicsCommandList* commandList, std::uint32_t vertexCount, std::uint32_t startVertexLocation);

    private:
        /// <summary>
        /// テクスチャ SRV 用ディスクリプタヒープを生成する。
        /// </summary>
        bool CreateDescriptorHeap(ID3D12Device* device);

        /// <summary>
        /// スプライト描画用シェーダーと固定機能状態を生成する。
        /// </summary>
        bool CreateShaderState(ID3D12Device* device);

        /// <summary>
        /// スプライト描画用ジオメトリを生成する。
        /// </summary>
        bool CreateGeometry(ID3D12Device* device);

        /// <summary>
        /// 現在の SRV スロットへ null テクスチャ記述子を書き込む。
        /// </summary>
        void WriteNullShaderResourceView(ID3D12Device* device);

    private:
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
        std::shared_ptr<RHI::IVertexBuffer> m_vertexBuffer;
        D3D12_GPU_DESCRIPTOR_HANDLE m_boundTextureSrvGpu{};
        bool m_hasBoundTexture = false;
        RHI::QuadTransform2D m_quadTransform{};
    };
}
