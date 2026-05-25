#include "Collider2DComponent.h"

#include <cmath>

#include "Physics2D.h"
#include "Transform.h"

namespace Xelqoria::Game
{
	AabbCollider2D BuildAabbCollider2D(
		const Transform& transform,
		const Collider2DComponent& collider)
	{
		const float scaleX = transform.scale.x;
		const float scaleY = transform.scale.y;

		AabbCollider2D result{};
		result.center = {
			transform.position.x + (collider.offset.x * scaleX),
			transform.position.y + (collider.offset.y * scaleY)
		};
		result.halfSize = {
			(std::fabs(collider.size.x * scaleX)) * 0.5f,
			(std::fabs(collider.size.y * scaleY)) * 0.5f
		};
		return result;
	}
}
