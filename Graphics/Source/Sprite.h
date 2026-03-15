#pragma once
#include <memory>


namespace Xelqoria::Graphics
{
	class Texture2D;

	/// <summary>
	/// 2D スプライトの描画データ参照（主にテクスチャ）を保持するモデル。
	/// </summary>
	class Sprite
	{
	public:
		/// <summary>
		/// スプライトに使用するテクスチャを設定する。
		/// </summary>
		/// <param name="texture">設定するテクスチャ。</param>
		void SetTexture(std::shared_ptr <Texture2D> texture);

		/// <summary>
		/// 現在設定されているテクスチャを取得する。
		/// </summary>
		/// <returns>スプライトに紐づくテクスチャ。</returns>
		std::shared_ptr<Texture2D> GetTexture() const;
	private:
		std::shared_ptr<Texture2D> m_textuer;
	};
}
