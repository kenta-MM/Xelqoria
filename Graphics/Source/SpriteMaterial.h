#pragma once

#include "AssetId.h"

#include <array>
#include <memory>

namespace Xelqoria::Graphics
{
	class Texture2D;

	/// <summary>
	/// 2D スプライトの描画方法に関する情報を保持する Material。
	/// </summary>
	class SpriteMaterial
	{
	public:
		/// <summary>
		/// 描画に使用するテクスチャを設定する。
		/// </summary>
		/// <param name="texture">設定するテクスチャ。</param>
		void SetTexture(std::shared_ptr<Texture2D> texture);

		/// <summary>
		/// 描画に使用するテクスチャを取得する。
		/// </summary>
		/// <returns>設定されているテクスチャ。</returns>
		std::shared_ptr<Texture2D> GetTexture() const;

		/// <summary>
		/// 描画に使用するテクスチャアセット識別子を設定する。
		/// </summary>
		/// <param name="assetId">設定するアセット識別子。</param>
		void SetTextureAssetId(Core::AssetId assetId);

		/// <summary>
		/// 描画に使用するテクスチャアセット識別子を取得する。
		/// </summary>
		/// <returns>保持しているアセット識別子。</returns>
		const Core::AssetId& GetTextureAssetId() const;

		/// <summary>
		/// RGBA 順の色乗算値を設定する。
		/// </summary>
		/// <param name="red">赤成分。</param>
		/// <param name="green">緑成分。</param>
		/// <param name="blue">青成分。</param>
		/// <param name="alpha">アルファ成分。</param>
		void SetColor(float red, float green, float blue, float alpha);

		/// <summary>
		/// RGBA 順の色乗算値を取得する。
		/// </summary>
		/// <returns>保持している色乗算値。</returns>
		const std::array<float, 4>& GetColor() const;

		/// <summary>
		/// 外枠描画の有効状態を設定する。
		/// </summary>
		/// <param name="enabled">外枠描画を有効にする場合は true。</param>
		void SetOutlineEnabled(bool enabled);

		/// <summary>
		/// 外枠描画が有効かを取得する。
		/// </summary>
		/// <returns>外枠描画が有効な場合は true。</returns>
		bool IsOutlineEnabled() const;

		/// <summary>
		/// 外枠の太さを画面ピクセル単位で設定する。
		/// </summary>
		/// <param name="thickness">設定する外枠太さ。</param>
		void SetOutlineThickness(float thickness);

		/// <summary>
		/// 外枠の太さを取得する。
		/// </summary>
		/// <returns>画面ピクセル単位の外枠太さ。</returns>
		float GetOutlineThickness() const;

		/// <summary>
		/// RGBA 順の外枠色を設定する。
		/// </summary>
		/// <param name="red">赤成分。</param>
		/// <param name="green">緑成分。</param>
		/// <param name="blue">青成分。</param>
		/// <param name="alpha">アルファ成分。</param>
		void SetOutlineColor(float red, float green, float blue, float alpha);

		/// <summary>
		/// RGBA 順の外枠色を取得する。
		/// </summary>
		/// <returns>保持している外枠色。</returns>
		const std::array<float, 4>& GetOutlineColor() const;

	private:
		std::shared_ptr<Texture2D> m_texture;
		Core::AssetId m_textureAssetId{};
		std::array<float, 4> m_color{ 1.0f, 1.0f, 1.0f, 1.0f };
		bool m_outlineEnabled = false;
		float m_outlineThickness = 1.0f;
		std::array<float, 4> m_outlineColor{ 1.0f, 1.0f, 0.0f, 1.0f };
	};
}
