#pragma once

#include "AssetId.h"
#include "SpriteDrawInput.h"
#include "Vector2.h"

#include <array>
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

		/// <summary>
		/// スプライトに対応するテクスチャアセット識別子を設定する。
		/// </summary>
		/// <param name="assetId">設定するアセット識別子。</param>
		void SetTextureAssetId(Core::AssetId assetId);

		/// <summary>
		/// スプライトに対応するテクスチャアセット識別子を取得する。
		/// </summary>
		/// <returns>保持しているアセット識別子。</returns>
		const Core::AssetId& GetTextureAssetId() const;

		/// <summary>
		/// スプライトの描画位置を設定する。
		/// </summary>
		/// <param name="position">設定する描画位置。</param>
		void SetPosition(const Xelqoria::Math::Vector2& position);

		/// <summary>
		/// スプライトの描画位置を設定する。
		/// </summary>
		/// <param name="x">設定する X 位置。</param>
		/// <param name="y">設定する Y 位置。</param>
		void SetPosition(float x, float y);

		/// <summary>
		/// スプライトの描画位置を取得する。
		/// </summary>
		/// <returns>保持している描画位置。</returns>
		const Xelqoria::Math::Vector2& GetPosition() const;

		/// <summary>
		/// スプライトの拡大率を設定する。
		/// </summary>
		/// <param name="scale">設定する拡大率。</param>
		void SetScale(const Xelqoria::Math::Vector2& scale);

		/// <summary>
		/// スプライトの拡大率を設定する。
		/// </summary>
		/// <param name="x">設定する X 拡大率。</param>
		/// <param name="y">設定する Y 拡大率。</param>
		void SetScale(float x, float y);

		/// <summary>
		/// スプライトの拡大率を取得する。
		/// </summary>
		/// <returns>保持している拡大率。</returns>
		const Xelqoria::Math::Vector2& GetScale() const;

		/// <summary>
		/// スプライトの回転角度を設定する。
		/// </summary>
		/// <param name="rotationDegrees">度数法で表した回転角度。</param>
		void SetRotationDegrees(float rotationDegrees);

		/// <summary>
		/// スプライトの回転角度を取得する。
		/// </summary>
		/// <returns>度数法で表した回転角度。</returns>
		float GetRotationDegrees() const;

		/// <summary>
		/// スプライトの色乗算値を設定する。
		/// </summary>
		/// <param name="red">赤成分。</param>
		/// <param name="green">緑成分。</param>
		/// <param name="blue">青成分。</param>
		/// <param name="alpha">アルファ成分。</param>
		void SetColor(float red, float green, float blue, float alpha);

		/// <summary>
		/// スプライトの色乗算値を取得する。
		/// </summary>
		/// <returns>RGBA 順の色乗算値。</returns>
		const std::array<float, 4>& GetColor() const;

		/// <summary>
		/// スプライト外枠描画の有効状態を設定する。
		/// </summary>
		/// <param name="enabled">外枠描画を有効にする場合は true。</param>
		void SetOutlineEnabled(bool enabled);

		/// <summary>
		/// スプライト外枠描画が有効かを取得する。
		/// </summary>
		/// <returns>外枠描画が有効な場合は true。</returns>
		bool IsOutlineEnabled() const;

		/// <summary>
		/// スプライト外枠の太さを画面ピクセル単位で設定する。
		/// </summary>
		/// <param name="thickness">設定する外枠太さ。</param>
		void SetOutlineThickness(float thickness);

		/// <summary>
		/// スプライト外枠の太さを取得する。
		/// </summary>
		/// <returns>画面ピクセル単位の外枠太さ。</returns>
		float GetOutlineThickness() const;

		/// <summary>
		/// スプライト外枠色を設定する。
		/// </summary>
		/// <param name="red">赤成分。</param>
		/// <param name="green">緑成分。</param>
		/// <param name="blue">青成分。</param>
		/// <param name="alpha">アルファ成分。</param>
		void SetOutlineColor(float red, float green, float blue, float alpha);

		/// <summary>
		/// スプライト外枠色を取得する。
		/// </summary>
		/// <returns>RGBA 順の外枠色。</returns>
		const std::array<float, 4>& GetOutlineColor() const;

		/// <summary>
		/// SpriteRenderer が受け取る共通 Sprite 入力データへ変換する。
		/// </summary>
		/// <returns>現在の Sprite 状態から作成した共通描画入力。</returns>
		[[nodiscard]] SpriteDrawInput ToDrawInput() const;

	private:
		std::shared_ptr<Texture2D> m_texture;
		Core::AssetId m_textureAssetId{};
		Xelqoria::Math::Vector2 m_position{};
		Xelqoria::Math::Vector2 m_scale{ 1.0f, 1.0f };
		float m_rotationDegrees = 0.0f;
		std::array<float, 4> m_color{ 1.0f, 1.0f, 1.0f, 1.0f };
		bool m_outlineEnabled = false;
		float m_outlineThickness = 1.0f;
		std::array<float, 4> m_outlineColor{ 1.0f, 1.0f, 0.0f, 1.0f };
	};
}
