#pragma once
#include "Vector3.h"

namespace Xelqoria::Game
{
	/// <summary>
	/// Scene 上のオブジェクト配置に必要な最小 Transform 情報を保持する。
	/// </summary>
	struct Transform
	{
		/// <summary>
		/// 既定の Transform を生成する。
		/// </summary>
		Transform()
			: position{}
			, rotation{}
			, scale{ 1.0f, 1.0f, 1.0f }
		{
		}

		/// <summary>
		/// 配置位置を表す。
		/// </summary>
		Xelqoria::Math::Vector3 position{};

		/// <summary>
		/// 回転量を表す。
		/// </summary>
		Xelqoria::Math::Vector3 rotation{};

		/// <summary>
		/// 拡大率を表す。
		/// 初期値は等倍とする。
		/// </summary>
		Xelqoria::Math::Vector3 scale{ 1.0f, 1.0f, 1.0f };

		/// <summary>
		/// 配置位置を一括で更新する。
		/// </summary>
		/// <param name="newPosition">設定する新しい位置。</param>
		void SetPosition(const Xelqoria::Math::Vector3& newPosition);

		/// <summary>
		/// 配置位置を各成分で更新する。
		/// </summary>
		/// <param name="x">X 座標。</param>
		/// <param name="y">Y 座標。</param>
		/// <param name="z">Z 座標。</param>
		void SetPosition(float x, float y, float z);

		/// <summary>
		/// 回転量を一括で更新する。
		/// </summary>
		/// <param name="newRotation">設定する新しい回転量。</param>
		void SetRotation(const Xelqoria::Math::Vector3& newRotation);

		/// <summary>
		/// 回転量を各成分で更新する。
		/// </summary>
		/// <param name="x">X 軸回転量。</param>
		/// <param name="y">Y 軸回転量。</param>
		/// <param name="z">Z 軸回転量。</param>
		void SetRotation(float x, float y, float z);

		/// <summary>
		/// 拡大率を一括で更新する。
		/// </summary>
		/// <param name="newScale">設定する新しい拡大率。</param>
		void SetScale(const Xelqoria::Math::Vector3& newScale);

		/// <summary>
		/// 拡大率を各成分で更新する。
		/// </summary>
		/// <param name="x">X 軸拡大率。</param>
		/// <param name="y">Y 軸拡大率。</param>
		/// <param name="z">Z 軸拡大率。</param>
		void SetScale(float x, float y, float z);
	};
}
