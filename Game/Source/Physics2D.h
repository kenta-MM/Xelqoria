#pragma once
#include "Vector2.h"

namespace Xelqoria::Game
{
	/// <summary>
	/// 2D の軸平行矩形コライダーを表す。
	/// </summary>
	struct AabbCollider2D
	{
		/// <summary>
		/// コライダー中心座標。
		/// </summary>
		Xelqoria::Math::Vector2 center{};

		/// <summary>
		/// コライダーの半分の幅と高さ。
		/// </summary>
		Xelqoria::Math::Vector2 halfSize{};
	};

	/// <summary>
	/// 2D 当たり判定の結果を表す。
	/// </summary>
	struct CollisionResult2D
	{
		/// <summary>
		/// 当たり判定が成立したかどうか。
		/// </summary>
		bool hit = false;

		/// <summary>
		/// 対象を押し戻す方向を表す単位法線。
		/// </summary>
		Xelqoria::Math::Vector2 normal{};

		/// <summary>
		/// 法線方向へ押し戻す必要がある距離。
		/// </summary>
		float penetrationDepth = 0.0f;
	};

	/// <summary>
	/// 2D 当たり判定と反発に必要な最小 API を提供する。
	/// </summary>
	class Physics2D
	{
	public:
		/// <summary>
		/// 2つの AABB コライダーの重なりを判定する。
		/// </summary>
		/// <param name="movingCollider">反発対象となるコライダー。</param>
		/// <param name="staticCollider">衝突相手となるコライダー。</param>
		/// <returns>当たり判定結果。hit が true の場合は法線とめり込み量を含む。</returns>
		[[nodiscard]] static CollisionResult2D TestAabbOverlap(
			const AabbCollider2D& movingCollider,
			const AabbCollider2D& staticCollider);

		/// <summary>
		/// 衝突結果から、めり込みを解消する移動量を計算する。
		/// </summary>
		/// <param name="collisionResult">当たり判定結果。</param>
		/// <returns>hit が true の場合は押し戻しベクトル、それ以外はゼロベクトル。</returns>
		[[nodiscard]] static Xelqoria::Math::Vector2 ComputeSeparationVector(
			const CollisionResult2D& collisionResult);

		/// <summary>
		/// 衝突法線に基づいて速度を反発方向へ変換する。
		/// </summary>
		/// <param name="velocity">現在の速度。</param>
		/// <param name="collisionResult">当たり判定結果。</param>
		/// <param name="restitution">反発係数。0 から 1 の範囲に補正して扱う。</param>
		/// <returns>反発後の速度。衝突していない場合や離れる向きの場合は元の速度。</returns>
		[[nodiscard]] static Xelqoria::Math::Vector2 ResolveRestitution(
			const Xelqoria::Math::Vector2& velocity,
			const CollisionResult2D& collisionResult,
			float restitution);
	};
}
