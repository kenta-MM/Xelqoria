#include "Transform.h"

namespace Xelqoria::Game
{
	void Transform::SetPosition(const Vector3& newPosition)
	{
		position = newPosition;
	}

	void Transform::SetPosition(float x, float y, float z)
	{
		position = Vector3{ x, y, z };
	}
}
