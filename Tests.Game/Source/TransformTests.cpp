#include "EntityManager.h"
#include "Transform.h"

namespace
{
	bool IsEqual(float lhs, float rhs)
	{
		return lhs == rhs;
	}
}

/// <summary>
/// Transform の最小生成と位置変更 API を検証する。
/// </summary>
/// <returns>成功時は 0、失敗時は 1 を返す。</returns>
int main()
{
	Xelqoria::Game::Transform transform;

	if (!IsEqual(transform.position.x, 0.0f) ||
		!IsEqual(transform.position.y, 0.0f) ||
		!IsEqual(transform.position.z, 0.0f)) {
		return 1;
	}

	if (!IsEqual(transform.rotation.x, 0.0f) ||
		!IsEqual(transform.rotation.y, 0.0f) ||
		!IsEqual(transform.rotation.z, 0.0f)) {
		return 1;
	}

	if (!IsEqual(transform.scale.x, 1.0f) ||
		!IsEqual(transform.scale.y, 1.0f) ||
		!IsEqual(transform.scale.z, 1.0f)) {
		return 1;
	}

	transform.SetPosition(10.0f, 20.0f, 30.0f);
	if (!IsEqual(transform.position.x, 10.0f) ||
		!IsEqual(transform.position.y, 20.0f) ||
		!IsEqual(transform.position.z, 30.0f)) {
		return 1;
	}

	transform.SetPosition(Xelqoria::Game::Vector3{ -1.0f, 2.5f, 0.5f });
	if (!IsEqual(transform.position.x, -1.0f) ||
		!IsEqual(transform.position.y, 2.5f) ||
		!IsEqual(transform.position.z, 0.5f)) {
		return 1;
	}

	Xelqoria::Game::EntityManager entities;
	const Xelqoria::Game::EntityId firstEntity = entities.CreateEntity();
	const Xelqoria::Game::EntityId secondEntity = entities.CreateEntity();

	if (firstEntity == 0 || secondEntity == 0 || firstEntity == secondEntity) {
		return 1;
	}

	if (!entities.HasEntity(firstEntity) || !entities.HasEntity(secondEntity)) {
		return 1;
	}

	if (entities.GetEntityCount() != 2) {
		return 1;
	}

	Xelqoria::Game::Transform firstTransform;
	firstTransform.SetPosition(3.0f, 4.0f, 5.0f);
	if (!entities.SetTransform(firstEntity, firstTransform)) {
		return 1;
	}

	const Xelqoria::Game::Transform* storedTransform = entities.FindTransform(firstEntity);
	if (storedTransform == nullptr ||
		!IsEqual(storedTransform->position.x, 3.0f) ||
		!IsEqual(storedTransform->position.y, 4.0f) ||
		!IsEqual(storedTransform->position.z, 5.0f)) {
		return 1;
	}

	if (entities.FindTransform(secondEntity) != nullptr) {
		return 1;
	}

	if (entities.SetTransform(0, firstTransform)) {
		return 1;
	}

	if (!entities.DestroyEntity(firstEntity)) {
		return 1;
	}

	if (entities.HasEntity(firstEntity) ||
		entities.FindTransform(firstEntity) != nullptr ||
		entities.GetEntityCount() != 1) {
		return 1;
	}

	if (entities.DestroyEntity(firstEntity)) {
		return 1;
	}

	return 0;
}
