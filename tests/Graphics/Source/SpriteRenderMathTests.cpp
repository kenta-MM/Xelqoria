#include <cmath>
#include <memory>

#include <gtest/gtest.h>

#include "AssetId.h"
#include "ITexture.h"
#include "QuadTransformFactory.h"
#include "Sprite.h"
#include "SpriteRenderMath.h"
#include "TextureAssetRegistry.h"
#include "Texture2D.h"

namespace
{
    bool IsEqual(float lhs, float rhs)
    {
        return std::fabs(lhs - rhs) < 0.0001f;
    }

    class FakeTexture final : public Xelqoria::RHI::ITexture
    {
    public:
        FakeTexture(std::uint32_t width, std::uint32_t height)
            : m_width(width)
            , m_height(height)
        {
        }

        std::uint32_t GetWidth() const override
        {
            return m_width;
        }

        std::uint32_t GetHeight() const override
        {
            return m_height;
        }

    private:
        std::uint32_t m_width = 0;
        std::uint32_t m_height = 0;
    };
}

TEST(SpriteRenderMathTests, SpriteStoresRenderStateAndComputesQuadTransform)
{
    Xelqoria::Graphics::Sprite sprite;
    sprite.SetTextureAssetId("textures/player-idle");
    EXPECT_EQ(sprite.GetTextureAssetId(), Xelqoria::Core::AssetId("textures/player-idle"));

    auto renderTexture = std::make_shared<Xelqoria::Graphics::Texture2D>();
    renderTexture->SetRHITexture(std::make_shared<FakeTexture>(64, 32));

    sprite.SetTexture(renderTexture);
    sprite.SetPosition(160.0f, -90.0f);
    sprite.SetScale(2.0f, 0.5f);
    sprite.SetRotationDegrees(90.0f);
    sprite.SetColor(0.25f, 0.5f, 0.75f, 0.8f);

    EXPECT_TRUE(IsEqual(sprite.GetPosition().x, 160.0f));
    EXPECT_TRUE(IsEqual(sprite.GetPosition().y, -90.0f));
    EXPECT_TRUE(IsEqual(sprite.GetScale().x, 2.0f));
    EXPECT_TRUE(IsEqual(sprite.GetScale().y, 0.5f));
    EXPECT_TRUE(IsEqual(sprite.GetRotationDegrees(), 90.0f));
    EXPECT_TRUE(IsEqual(sprite.GetColor()[0], 0.25f));
    EXPECT_TRUE(IsEqual(sprite.GetColor()[3], 0.8f));

    const auto quadTransform = Xelqoria::Graphics::ComputeSpriteQuadTransform(sprite, 1280, 720);
    EXPECT_TRUE(IsEqual(quadTransform.scaleX, 0.2f));
    EXPECT_TRUE(IsEqual(quadTransform.scaleY, 0.044444446f));
    EXPECT_TRUE(IsEqual(quadTransform.translateX, 0.25f));
    EXPECT_TRUE(IsEqual(quadTransform.translateY, 0.25f));
    EXPECT_TRUE(IsEqual(quadTransform.rotationCos, 0.0f));
    EXPECT_TRUE(IsEqual(quadTransform.rotationSin, 1.0f));

    const auto emptyTransform = Xelqoria::Graphics::ComputeSpriteQuadTransform(sprite, 0, 720);
    EXPECT_TRUE(IsEqual(emptyTransform.scaleX, 1.0f));
    EXPECT_TRUE(IsEqual(emptyTransform.scaleY, 1.0f));
    EXPECT_TRUE(IsEqual(emptyTransform.translateX, 0.0f));
    EXPECT_TRUE(IsEqual(emptyTransform.translateY, 0.0f));
    EXPECT_TRUE(IsEqual(emptyTransform.rotationCos, 1.0f));
    EXPECT_TRUE(IsEqual(emptyTransform.rotationSin, 0.0f));
}

TEST(SpriteRenderMathTests, TextureRegistryAndTexture2DHandleResolvedAndMissingTextures)
{
    auto renderTexture = std::make_shared<Xelqoria::Graphics::Texture2D>();
    renderTexture->SetRHITexture(std::make_shared<FakeTexture>(64, 32));

    Xelqoria::Graphics::TextureAssetRegistry registry;
    registry.RegisterTexture("textures/player-idle", renderTexture);

    EXPECT_EQ(registry.ResolveTexture("textures/player-idle"), renderTexture);
    EXPECT_EQ(registry.ResolveTexture("textures/missing"), nullptr);

    Xelqoria::Graphics::Texture2D emptyTexture;
    emptyTexture.SetRHITexture(std::shared_ptr<Xelqoria::RHI::ITexture>{});

    EXPECT_EQ(emptyTexture.GetRHITexture(), nullptr);
    EXPECT_EQ(emptyTexture.GetWidth(), 0u);
    EXPECT_EQ(emptyTexture.GetHeight(), 0u);
}

TEST(SpriteRenderMathTests, QuadRenderConstantsPackGraphicsStateForRhi)
{
    Xelqoria::Graphics::QuadRenderConstants constants{};
    constants.scaleX = 2.0f;
    constants.scaleY = 3.0f;
    constants.rotationCos = 0.5f;
    constants.rotationSin = 0.25f;
    constants.translateX = 0.125f;
    constants.translateY = -0.5f;
    constants.outlineEnabled = 1.0f;
    constants.outlineThickness = 4.0f;
    constants.outlineColor = { 0.1f, 0.2f, 0.3f, 0.4f };
    constants.fillColor = { 0.5f, 0.6f, 0.7f, 0.8f };
    constants.textureEnabled = 0.0f;

    const auto packedConstants = Xelqoria::Graphics::PackQuadRenderConstants(constants);
    EXPECT_EQ(Xelqoria::Graphics::QuadRenderConstantFloatCount, packedConstants.size());
    EXPECT_TRUE(IsEqual(2.0f, packedConstants[0]));
    EXPECT_TRUE(IsEqual(3.0f, packedConstants[1]));
    EXPECT_TRUE(IsEqual(0.5f, packedConstants[2]));
    EXPECT_TRUE(IsEqual(0.25f, packedConstants[3]));
    EXPECT_TRUE(IsEqual(0.125f, packedConstants[4]));
    EXPECT_TRUE(IsEqual(-0.5f, packedConstants[5]));
    EXPECT_TRUE(IsEqual(1.0f, packedConstants[6]));
    EXPECT_TRUE(IsEqual(4.0f, packedConstants[7]));
    EXPECT_TRUE(IsEqual(0.1f, packedConstants[8]));
    EXPECT_TRUE(IsEqual(0.4f, packedConstants[11]));
    EXPECT_TRUE(IsEqual(0.5f, packedConstants[12]));
    EXPECT_TRUE(IsEqual(0.8f, packedConstants[15]));
    EXPECT_TRUE(IsEqual(0.0f, packedConstants[16]));
}
