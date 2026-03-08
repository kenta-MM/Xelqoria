#pragma once

namespace Xelqoria::RHI
{
    /**
	 * @brief 使用するグラフィックス API の種類
	 *
	 * RHI（Render Hardware Interface）層で利用する
	 * グラフィックス API を指定するための列挙型。
	 *
	 * エンジン初期化時などに、この値を指定することで
	 * 利用する描画バックエンドを選択する。
	 *
	 * 例：
	 * - Direct3D11
	 * - Direct3D12
	 * - Vulkan
	 *
	 * 将来的に複数のグラフィックス API をサポートする場合、
	 * この列挙型に項目を追加することで対応する。
	 */
	enum class GraphicsAPI
	{
		None = 0,
		D3D11,
		D3D12,
	};

	/**
	 * @brief GraphicsAPI を文字列へ変換する
	 *
	 * 指定された GraphicsAPI の名前を
	 * wchar_t 文字列として返す。
	 *
	 * 主に以下の用途で使用する。
	 *
	 * - ウィンドウタイトル表示
	 * - ログ出力
	 * - デバッグ表示
	 *
	 * 例:
	 * @code
	 * auto api = GraphicsAPI::D3D12;
	 * const wchar_t* name = ToString(api);
	 * // name = L"D3D12"
	 * @endcode
	 *
	 * @param api 変換する GraphicsAPI
	 * @return API 名を表す文字列
	 */
	constexpr const wchar_t* GraphicsAPIToString(GraphicsAPI api)
	{
		switch (api)
		{
			case GraphicsAPI::D3D11: return L"D3D11";
			case GraphicsAPI::D3D12: return L"D3D12";
			default: return L"None";
		}
	}
}