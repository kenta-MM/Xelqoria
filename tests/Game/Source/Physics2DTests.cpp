#include <cmath>

#include <gtest/gtest.h>

#include "Collider2DComponent.h"
#include "Physics2D.h"
#include "Transform.h"

namespace
{
	bool IsEqual(float lhs, float rhs)
	{
		return std::fabs(lhs - rhs) < 0.0001f;
	}
}

TEST(Physics2DTests, AabbOverlapReportsCollisionNormalAndSeparation)
{
	const Xelqoria::Game::AabbCollider2D movingCollider{
		{ 0.5f, 0.0f },
		{ 1.0f, 1.0f }
	};
	const Xelqoria::Game::AabbCollider2D staticCollider{
		{ 1.25f, 0.0f },
		{ 1.0f, 1.0f }
	};

	const Xelqoria::Game::CollisionResult2D result =
		Xelqoria::Game::Physics2D::TestAabbOverlap(movingCollider, staticCollider);

	EXPECT_TRUE(result.hit);
	EXPECT_TRUE(IsEqual(result.normal.x, -1.0f));
	EXPECT_TRUE(IsEqual(result.normal.y, 0.0f));
	EXPECT_TRUE(IsEqual(result.penetrationDepth, 1.25f));

	const Xelqoria::Math::Vector2 separation =
		Xelqoria::Game::Physics2D::ComputeSeparationVector(result);
	EXPECT_TRUE(IsEqual(separation.x, -1.25f));
	EXPECT_TRUE(IsEqual(separation.y, 0.0f));
}

TEST(Physics2DTests, AabbOverlapReturnsMissWhenSeparated)
{
	const Xelqoria::Game::AabbCollider2D movingCollider{
		{ -3.0f, 0.0f },
		{ 1.0f, 1.0f }
	};
	const Xelqoria::Game::AabbCollider2D staticCollider{
		{ 3.0f, 0.0f },
		{ 1.0f, 1.0f }
	};

	const Xelqoria::Game::CollisionResult2D result =
		Xelqoria::Game::Physics2D::TestAabbOverlap(movingCollider, staticCollider);

	EXPECT_FALSE(result.hit);
	EXPECT_TRUE(IsEqual(result.normal.x, 0.0f));
	EXPECT_TRUE(IsEqual(result.normal.y, 0.0f));
	EXPECT_TRUE(IsEqual(result.penetrationDepth, 0.0f));
}

TEST(Physics2DTests, RestitutionReflectsVelocityAlongCollisionNormal)
{
	const Xelqoria::Game::CollisionResult2D result{
		true,
		{ -1.0f, 0.0f },
		1.0f
	};
	const Xelqoria::Math::Vector2 velocity{ 10.0f, 2.0f };

	const Xelqoria::Math::Vector2 reflectedVelocity =
		Xelqoria::Game::Physics2D::ResolveRestitution(velocity, result, 0.5f);

	EXPECT_TRUE(IsEqual(reflectedVelocity.x, -5.0f));
	EXPECT_TRUE(IsEqual(reflectedVelocity.y, 2.0f));
}

TEST(Physics2DTests, RestitutionDoesNotApplyWhenAlreadyMovingAway)
{
	const Xelqoria::Game::CollisionResult2D result{
		true,
		{ -1.0f, 0.0f },
		1.0f
	};
	const Xelqoria::Math::Vector2 velocity{ -3.0f, 4.0f };

	const Xelqoria::Math::Vector2 resolvedVelocity =
		Xelqoria::Game::Physics2D::ResolveRestitution(velocity, result, 1.0f);

	EXPECT_TRUE(IsEqual(resolvedVelocity.x, -3.0f));
	EXPECT_TRUE(IsEqual(resolvedVelocity.y, 4.0f));
}

TEST(Physics2DTests, SpatialHashReturnsOnlyNearbyPotentialPairs)
{
	Xelqoria::Game::PhysicsSpatialHash2D spatialHash(2.0f);
	spatialHash.Insert(1, Xelqoria::Game::AabbCollider2D{ { 0.0f, 0.0f }, { 0.5f, 0.5f } });
	spatialHash.Insert(2, Xelqoria::Game::AabbCollider2D{ { 0.75f, 0.0f }, { 0.5f, 0.5f } });
	spatialHash.Insert(3, Xelqoria::Game::AabbCollider2D{ { 100.0f, 100.0f }, { 0.5f, 0.5f } });

	const std::vector<Xelqoria::Game::PotentialCollisionPair2D> pairs = spatialHash.QueryPotentialPairs();

	ASSERT_EQ(pairs.size(), 1u);
	EXPECT_EQ(pairs[0].first, 1u);
	EXPECT_EQ(pairs[0].second, 2u);
}

TEST(Physics2DTests, SpatialHashRemovesDuplicatePairsAcrossCells)
{
	Xelqoria::Game::PhysicsSpatialHash2D spatialHash(1.0f);
	spatialHash.Insert(1, Xelqoria::Game::AabbCollider2D{ { 0.5f, 0.5f }, { 1.0f, 1.0f } });
	spatialHash.Insert(2, Xelqoria::Game::AabbCollider2D{ { 0.75f, 0.75f }, { 1.0f, 1.0f } });

	const std::vector<Xelqoria::Game::PotentialCollisionPair2D> pairs = spatialHash.QueryPotentialPairs();

	ASSERT_EQ(pairs.size(), 1u);
	EXPECT_EQ(pairs[0].first, 1u);
	EXPECT_EQ(pairs[0].second, 2u);
}

TEST(Physics2DTests, BuildAabbCollider2DUsesTransformPositionOffsetAndScale)
{
	Xelqoria::Game::Transform transform{};
	transform.position = { 10.0f, -20.0f, 5.0f };
	transform.scale = { 2.0f, 3.0f, 1.0f };
	Xelqoria::Game::Collider2DComponent collider{};
	collider.offset = { 4.0f, -5.0f };
	collider.size = { 6.0f, 8.0f };

	const Xelqoria::Game::AabbCollider2D aabb =
		Xelqoria::Game::BuildAabbCollider2D(transform, collider);

	EXPECT_TRUE(IsEqual(aabb.center.x, 18.0f));
	EXPECT_TRUE(IsEqual(aabb.center.y, -35.0f));
	EXPECT_TRUE(IsEqual(aabb.halfSize.x, 6.0f));
	EXPECT_TRUE(IsEqual(aabb.halfSize.y, 12.0f));
}

TEST(Physics2DTests, BuildAabbCollider2DUsesAbsoluteScaleAndIgnoresRotationZ)
{
	Xelqoria::Game::Transform transform{};
	transform.position = { 1.0f, 2.0f, 0.0f };
	transform.rotation = { 0.0f, 0.0f, 90.0f };
	transform.scale = { -2.0f, -4.0f, 1.0f };
	Xelqoria::Game::Collider2DComponent collider{};
	collider.offset = { 3.0f, 5.0f };
	collider.size = { 7.0f, 11.0f };

	const Xelqoria::Game::AabbCollider2D aabb =
		Xelqoria::Game::BuildAabbCollider2D(transform, collider);

	EXPECT_TRUE(IsEqual(aabb.center.x, -5.0f));
	EXPECT_TRUE(IsEqual(aabb.center.y, -18.0f));
	EXPECT_TRUE(IsEqual(aabb.halfSize.x, 7.0f));
	EXPECT_TRUE(IsEqual(aabb.halfSize.y, 22.0f));
}
