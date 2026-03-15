#include "Scene.h"
#include "Transform.h"

namespace
{
	bool IsEqual(float lhs, float rhs)
	{
		return lhs == rhs;
	}
}

/// <summary>
/// Transform と Scene の最小ランタイム API を検証する。
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

	Xelqoria::Game::Scene scene;
	const Xelqoria::Game::EntityId firstEntityId = scene.CreateEntity().GetId();
	const Xelqoria::Game::EntityId secondEntityId = scene.CreateEntity().GetId();
	auto secondEntity = scene.FindEntity(secondEntityId);
	const std::span<const Xelqoria::Game::Entity> entities = scene.GetEntities();

	if (scene.GetEntityCount() != 2 || entities.size() != 2) {
		return 1;
	}

	if (firstEntityId != 1 || secondEntityId != 2) {
		return 1;
	}

	if (entities[0].GetId() != 1 || entities[1].GetId() != 2) {
		return 1;
	}

	if (!secondEntity.has_value()) {
		return 1;
	}

	secondEntity->get().GetTransform().SetPosition(3.0f, 4.0f, 5.0f);
	if (!IsEqual(secondEntity->get().GetTransform().position.x, 3.0f) ||
		!IsEqual(secondEntity->get().GetTransform().position.y, 4.0f) ||
		!IsEqual(secondEntity->get().GetTransform().position.z, 5.0f)) {
		return 1;
	}

	if (!scene.DestroyEntity(firstEntityId)) {
		return 1;
	}

	if (scene.DestroyEntity(firstEntityId)) {
		return 1;
	}

	if (scene.GetEntityCount() != 1) {
		return 1;
	}

	const std::span<const Xelqoria::Game::Entity> remainingEntities = scene.GetEntities();
	if (remainingEntities.size() != 1 || remainingEntities[0].GetId() != secondEntityId) {
		return 1;
	}

	return 0;
}
