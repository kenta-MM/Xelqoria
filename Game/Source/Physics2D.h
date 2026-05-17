#pragma once
#include "Vector2.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Xelqoria::Game
{
	/// <summary>
	/// Physics2D のコライダー識別子を表す。
	/// </summary>
	using PhysicsColliderId2D = std::uint32_t;

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
	/// 空間分割で抽出された当たり判定候補ペアを表す。
	/// </summary>
	struct PotentialCollisionPair2D
	{
		/// <summary>
		/// 小さい方のコライダー識別子。
		/// </summary>
		PhysicsColliderId2D first = 0;

		/// <summary>
		/// 大きい方のコライダー識別子。
		/// </summary>
		PhysicsColliderId2D second = 0;
	};

	/// <summary>
	/// 2D 当たり判定向けの spatial hash 空間分割を提供する。
	/// </summary>
	class PhysicsSpatialHash2D
	{
	public:
		/// <summary>
		/// spatial hash を生成する。
		/// </summary>
		/// <param name="cellSize">1セルの幅と高さ。0 以下の場合は 1 として扱う。</param>
		explicit PhysicsSpatialHash2D(float cellSize);

		/// <summary>
		/// 登録済みコライダーをすべて削除する。
		/// </summary>
		void Clear();

		/// <summary>
		/// コライダーを重なるセルへ登録する。
		/// </summary>
		/// <param name="id">登録するコライダー識別子。</param>
		/// <param name="collider">登録する AABB コライダー。</param>
		void Insert(PhysicsColliderId2D id, const AabbCollider2D& collider);

		/// <summary>
		/// 同じセルを共有する当たり判定候補ペアを取得する。
		/// </summary>
		/// <returns>候補ペア一覧。同じペアは1回だけ返る。</returns>
		[[nodiscard]] std::vector<PotentialCollisionPair2D> QueryPotentialPairs() const;

	private:
		struct CellKey
		{
			int x = 0;
			int y = 0;

			[[nodiscard]] bool operator==(const CellKey& other) const
			{
				return x == other.x && y == other.y;
			}
		};

		struct CellKeyHasher
		{
			[[nodiscard]] std::size_t operator()(const CellKey& key) const;
		};

		struct CellEntry
		{
			PhysicsColliderId2D id = 0;
			AabbCollider2D collider{};
		};

		float m_cellSize = 1.0f;
		std::unordered_map<CellKey, std::vector<CellEntry>, CellKeyHasher> m_cells{};
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
