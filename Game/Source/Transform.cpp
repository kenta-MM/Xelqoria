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

	void Transform::SetRotation(const Vector3& newRotation)
	{
		rotation = newRotation;
	}

	void Transform::SetRotation(float x, float y, float z)
	{
		rotation = Vector3{ x, y, z };
	}

	void Transform::SetScale(const Vector3& newScale)
	{
		scale = newScale;
	}

	void Transform::SetScale(float x, float y, float z)
	{
		scale = Vector3{ x, y, z };
	}
}
