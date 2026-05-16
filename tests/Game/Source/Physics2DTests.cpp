#include <cmath>

#include <gtest/gtest.h>

#include "Physics2D.h"

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
