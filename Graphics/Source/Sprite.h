#pragma once

#include "AssetId.h"

#include <memory>

namespace Xelqoria::Graphics
{
	class Texture2D;

	/// <summary>
	/// 2 次元座標や拡大率に使う最小ベクトル値を表す。
	/// </summary>
	struct Vector2
	{
		/// <summary>
		/// X 成分を表す。
		/// </summary>
		float x = 0.0f;

		/// <summary>
		/// Y 成分を表す。
		/// </summary>
		float y = 0.0f;
	};

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
		void SetPosition(const Vector2& position);

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
		const Vector2& GetPosition() const;

		/// <summary>
		/// スプライトの拡大率を設定する。
		/// </summary>
		/// <param name="scale">設定する拡大率。</param>
		void SetScale(const Vector2& scale);

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
		const Vector2& GetScale() const;

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

	private:
		std::shared_ptr<Texture2D> m_texture;
		Core::AssetId m_textureAssetId{};
		Vector2 m_position{};
		Vector2 m_scale{ 1.0f, 1.0f };
		float m_rotationDegrees = 0.0f;
	};
}
