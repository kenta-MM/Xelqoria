#include <string>

#include <gtest/gtest.h>

#include "AssetId.h"

TEST(AssetIdTests, DefaultConstructedAssetIdIsEmpty)
{
    const Xelqoria::Core::AssetId emptyAssetId;

    EXPECT_TRUE(emptyAssetId.IsEmpty());
    EXPECT_TRUE(emptyAssetId.GetValue().empty());
}

TEST(AssetIdTests, NullCStringConstructedAssetIdIsEmpty)
{
    const Xelqoria::Core::AssetId nullAssetId(static_cast<const char*>(nullptr));

    EXPECT_TRUE(nullAssetId.IsEmpty());
}

TEST(AssetIdTests, CStringConstructedAssetIdStoresOwnedValue)
{
    const Xelqoria::Core::AssetId cStringAssetId("textures/player");

    EXPECT_FALSE(cStringAssetId.IsEmpty());
    EXPECT_EQ(std::string("textures/player"), cStringAssetId.GetValue());
}

TEST(AssetIdTests, StringAndStringViewConstructedAssetIdsCompareEqual)
{
    const std::string ownedString = "sprites/player";
    const Xelqoria::Core::AssetId stringAssetId(ownedString);
    const Xelqoria::Core::AssetId viewAssetId(std::string_view("sprites/player"));

    EXPECT_EQ(stringAssetId, viewAssetId);
}

TEST(AssetIdTests, DifferentAssetIdsDoNotCompareEqual)
{
    const Xelqoria::Core::AssetId stringAssetId("sprites/player");
    const Xelqoria::Core::AssetId otherAssetId("textures/player");

    EXPECT_NE(stringAssetId, otherAssetId);
}
