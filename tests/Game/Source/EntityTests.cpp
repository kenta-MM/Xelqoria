#include <gtest/gtest.h>

#include "Collider2DComponent.h"
#include "Entity.h"

TEST(EntityTests, Collider2DComponentCanBeSetReadAndRemoved)
{
	Xelqoria::Game::Entity entity(1);
	Xelqoria::Game::Collider2DComponent collider{};
	collider.enabled = false;
	collider.isTrigger = true;
	collider.offset = { 2.0f, -3.0f };
	collider.size = { 4.0f, 5.0f };

	entity.SetCollider2DComponent(collider);

	ASSERT_TRUE(entity.HasCollider2DComponent());
	const auto storedCollider = entity.GetCollider2DComponent();
	ASSERT_TRUE(storedCollider.has_value());
	EXPECT_FALSE(storedCollider->get().enabled);
	EXPECT_TRUE(storedCollider->get().isTrigger);
	EXPECT_FLOAT_EQ(2.0f, storedCollider->get().offset.x);
	EXPECT_FLOAT_EQ(-3.0f, storedCollider->get().offset.y);
	EXPECT_FLOAT_EQ(4.0f, storedCollider->get().size.x);
	EXPECT_FLOAT_EQ(5.0f, storedCollider->get().size.y);

	entity.RemoveCollider2DComponent();

	EXPECT_FALSE(entity.HasCollider2DComponent());
	EXPECT_FALSE(entity.GetCollider2DComponent().has_value());
}

TEST(EntityTests, DefaultCollider2DComponentUsesBoxDefaults)
{
	const Xelqoria::Game::Collider2DComponent collider{};

	EXPECT_TRUE(collider.enabled);
	EXPECT_FALSE(collider.isTrigger);
	EXPECT_EQ(Xelqoria::Game::Collider2DShapeType::Box, collider.shapeType);
	EXPECT_FLOAT_EQ(0.0f, collider.offset.x);
	EXPECT_FLOAT_EQ(0.0f, collider.offset.y);
	EXPECT_FLOAT_EQ(1.0f, collider.size.x);
	EXPECT_FLOAT_EQ(1.0f, collider.size.y);
}
