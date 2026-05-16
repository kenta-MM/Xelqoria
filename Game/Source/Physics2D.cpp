#include "Physics2D.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
	float Dot(const Xelqoria::Math::Vector2& lhs, const Xelqoria::Math::Vector2& rhs)
	{
		return (lhs.x * rhs.x) + (lhs.y * rhs.y);
	}

	int ToCellIndex(float value, float cellSize)
	{
		return static_cast<int>(std::floor(value / cellSize));
	}

	Xelqoria::Game::PotentialCollisionPair2D MakePair(
		Xelqoria::Game::PhysicsColliderId2D lhs,
		Xelqoria::Game::PhysicsColliderId2D rhs)
	{
		if (lhs < rhs) {
			return { lhs, rhs };
		}

		return { rhs, lhs };
	}
}

namespace Xelqoria::Game
{
	std::size_t PhysicsSpatialHash2D::CellKeyHasher::operator()(const CellKey& key) const
	{
		const std::size_t xHash = static_cast<std::size_t>(static_cast<std::uint32_t>(key.x));
		const std::size_t yHash = static_cast<std::size_t>(static_cast<std::uint32_t>(key.y));
		return xHash ^ (yHash + 0x9e3779b9u + (xHash << 6) + (xHash >> 2));
	}

	PhysicsSpatialHash2D::PhysicsSpatialHash2D(float cellSize)
		: m_cellSize((std::max)(cellSize, std::numeric_limits<float>::epsilon()))
	{
	}

	void PhysicsSpatialHash2D::Clear()
	{
		m_cells.clear();
	}

	void PhysicsSpatialHash2D::Insert(PhysicsColliderId2D id, const AabbCollider2D& collider)
	{
		const float minX = collider.center.x - collider.halfSize.x;
		const float maxX = collider.center.x + collider.halfSize.x;
		const float minY = collider.center.y - collider.halfSize.y;
		const float maxY = collider.center.y + collider.halfSize.y;
		const int startX = ToCellIndex(minX, m_cellSize);
		const int endX = ToCellIndex(maxX, m_cellSize);
		const int startY = ToCellIndex(minY, m_cellSize);
		const int endY = ToCellIndex(maxY, m_cellSize);

		for (int y = startY; y <= endY; ++y)
		{
			for (int x = startX; x <= endX; ++x)
			{
				m_cells[CellKey{ x, y }].push_back(CellEntry{ id, collider });
			}
		}
	}

	std::vector<PotentialCollisionPair2D> PhysicsSpatialHash2D::QueryPotentialPairs() const
	{
		std::vector<PotentialCollisionPair2D> pairs{};
		for (const auto& cell : m_cells)
		{
			const std::vector<CellEntry>& entries = cell.second;
			for (std::size_t lhsIndex = 0; lhsIndex < entries.size(); ++lhsIndex)
			{
				for (std::size_t rhsIndex = lhsIndex + 1; rhsIndex < entries.size(); ++rhsIndex)
				{
					const PhysicsColliderId2D lhs = entries[lhsIndex].id;
					const PhysicsColliderId2D rhs = entries[rhsIndex].id;
					if (lhs == rhs) {
						continue;
					}

					pairs.push_back(MakePair(lhs, rhs));
				}
			}
		}

		std::sort(
			pairs.begin(),
			pairs.end(),
			[](const PotentialCollisionPair2D& lhs, const PotentialCollisionPair2D& rhs)
			{
				if (lhs.first != rhs.first) {
					return lhs.first < rhs.first;
				}

				return lhs.second < rhs.second;
			});
		const auto uniqueEnd = std::unique(
			pairs.begin(),
			pairs.end(),
			[](const PotentialCollisionPair2D& lhs, const PotentialCollisionPair2D& rhs)
			{
				return lhs.first == rhs.first && lhs.second == rhs.second;
			});
		pairs.erase(uniqueEnd, pairs.end());
		return pairs;
	}

	CollisionResult2D Physics2D::TestAabbOverlap(
		const AabbCollider2D& movingCollider,
		const AabbCollider2D& staticCollider)
	{
		const Xelqoria::Math::Vector2 delta = movingCollider.center - staticCollider.center;
		const float overlapX = (movingCollider.halfSize.x + staticCollider.halfSize.x) - std::fabs(delta.x);
		if (overlapX <= 0.0f) {
			return {};
		}

		const float overlapY = (movingCollider.halfSize.y + staticCollider.halfSize.y) - std::fabs(delta.y);
		if (overlapY <= 0.0f) {
			return {};
		}

		CollisionResult2D result{};
		result.hit = true;
		if (overlapX < overlapY) {
			result.normal = { delta.x < 0.0f ? -1.0f : 1.0f, 0.0f };
			result.penetrationDepth = overlapX;
			return result;
		}

		result.normal = { 0.0f, delta.y < 0.0f ? -1.0f : 1.0f };
		result.penetrationDepth = overlapY;
		return result;
	}

	Xelqoria::Math::Vector2 Physics2D::ComputeSeparationVector(const CollisionResult2D& collisionResult)
	{
		if (false == collisionResult.hit) {
			return {};
		}

		return collisionResult.normal * collisionResult.penetrationDepth;
	}

	Xelqoria::Math::Vector2 Physics2D::ResolveRestitution(
		const Xelqoria::Math::Vector2& velocity,
		const CollisionResult2D& collisionResult,
		float restitution)
	{
		if (false == collisionResult.hit) {
			return velocity;
		}

		const float velocityAlongNormal = Dot(velocity, collisionResult.normal);
		if (velocityAlongNormal >= 0.0f) {
			return velocity;
		}

		const float clampedRestitution = std::clamp(restitution, 0.0f, 1.0f);
		return velocity - (collisionResult.normal * ((1.0f + clampedRestitution) * velocityAlongNormal));
	}
}
