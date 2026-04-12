#include "Transform.h"
#include <Vector3.h>

namespace Xelqoria::Game
{
	void Transform::SetPosition(const Xelqoria::Math::Vector3& newPosition)
	{
		position = newPosition;
	}

	void Transform::SetPosition(float x, float y, float z)
	{
		position = Xelqoria::Math::Vector3{ x, y, z };
	}

	void Transform::SetRotation(const Xelqoria::Math::Vector3& newRotation)
	{
		rotation = newRotation;
	}

	void Transform::SetRotation(float x, float y, float z)
	{
		rotation = Xelqoria::Math::Vector3{ x, y, z };
	}

	void Transform::SetScale(const Xelqoria::Math::Vector3& newScale)
	{
		scale = newScale;
	}

	void Transform::SetScale(float x, float y, float z)
	{
		scale = Xelqoria::Math::Vector3{ x, y, z };
	}
}
