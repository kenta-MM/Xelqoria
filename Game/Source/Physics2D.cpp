#include "Physics2D.h"

#include <algorithm>
#include <cmath>

namespace
{
	float Dot(const Xelqoria::Math::Vector2& lhs, const Xelqoria::Math::Vector2& rhs)
	{
		return (lhs.x * rhs.x) + (lhs.y * rhs.y);
	}
}

namespace Xelqoria::Game
{
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
