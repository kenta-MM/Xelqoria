#pragma once

namespace Xelqoria::RHI
{
    /// <summary>
	/// 使用するグラフィックス API の種類。
	/// </summary>
	enum class GraphicsAPI
	{
		None = 0,
		D3D11,
		D3D12,
	};

	/// <summary>
	/// GraphicsAPI を表示用文字列へ変換する。
	/// </summary>
	/// <param name="api">変換対象の GraphicsAPI。</param>
	/// <returns>API 名を表す文字列。</returns>
	constexpr const wchar_t* GraphicsAPIToString(GraphicsAPI api)
	{
		switch (api)
		{
			case GraphicsAPI::D3D11: return L"DirectX11";
			case GraphicsAPI::D3D12: return L"DirectX12";
			default: return L"None";
		}
	}
}
