#include <cmath>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "AssetId.h"
#include "IGraphicsContext.h"
#include "ITexture.h"
#include "QuadTransformFactory.h"
#include "Sprite.h"
#include "SpriteCulling.h"
#include "SpriteDrawInput.h"
#include "SpriteMaterial.h"
#include "SpriteRenderMath.h"
#include "SpriteRenderer.h"
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

    class FakeGraphicsContext final : public Xelqoria::RHI::IGraphicsContext
    {
    public:
        bool Initialize(
            Xelqoria::RHI::NativeWindowHandle,
            Xelqoria::RHI::NativeInstanceHandle,
            std::uint32_t,
            std::uint32_t) override
        {
            return true;
        }

        void Shutdown() override
        {
        }

        void BeginFrame() override
        {
        }

        void EndFrame() override
        {
        }

        std::shared_ptr<Xelqoria::RHI::ITexture> CreateTextureFromFile(const std::wstring&) override
        {
            return nullptr;
        }

        void BindTexture(std::uint32_t slot, Xelqoria::RHI::ITexture* texture) override
        {
            lastBoundSlot = slot;
            lastBoundTexture = texture;
        }

        void SetShaderConstants(std::span<const float> constants) override
        {
            lastConstants.assign(constants.begin(), constants.end());
        }

        void Draw(std::uint32_t vertexCount, std::uint32_t startVertexLocation) override
        {
            ++drawCount;
            lastVertexCount = vertexCount;
            lastStartVertexLocation = startVertexLocation;
        }

        void DrawIndexed(std::uint32_t, std::uint32_t, std::int32_t) override
        {
        }

        void Resize(std::uint32_t width, std::uint32_t height) override
        {
            viewportWidth = width;
            viewportHeight = height;
        }

        std::uint32_t GetViewportWidth() const override
        {
            return viewportWidth;
        }

        std::uint32_t GetViewportHeight() const override
        {
            return viewportHeight;
        }

        std::uint32_t viewportWidth = 1280;
        std::uint32_t viewportHeight = 720;
        std::uint32_t drawCount = 0;
        std::uint32_t lastBoundSlot = 0;
        Xelqoria::RHI::ITexture* lastBoundTexture = nullptr;
        std::uint32_t lastVertexCount = 0;
        std::uint32_t lastStartVertexLocation = 0;
        std::vector<float> lastConstants{};
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

TEST(SpriteRenderMathTests, SpriteCreatesCommonDrawInput)
{
    Xelqoria::Graphics::Sprite sprite;
    sprite.SetTextureAssetId("textures/player-idle");
    sprite.SetPosition(32.0f, 48.0f);
    sprite.SetScale(1.5f, 2.0f);
    sprite.SetRotationDegrees(45.0f);
    sprite.SetColor(0.2f, 0.4f, 0.6f, 0.8f);
    sprite.SetOutlineEnabled(true);
    sprite.SetOutlineThickness(3.0f);
    sprite.SetOutlineColor(0.9f, 0.7f, 0.5f, 0.3f);

    auto renderTexture = std::make_shared<Xelqoria::Graphics::Texture2D>();
    renderTexture->SetRHITexture(std::make_shared<FakeTexture>(16, 24));
    sprite.SetTexture(renderTexture);

    const Xelqoria::Graphics::SpriteDrawInput input = sprite.ToDrawInput();

    EXPECT_EQ(input.texture, renderTexture);
    EXPECT_EQ(input.textureAssetId, Xelqoria::Core::AssetId("textures/player-idle"));
    EXPECT_TRUE(IsEqual(input.position.x, 32.0f));
    EXPECT_TRUE(IsEqual(input.scale.y, 2.0f));
    EXPECT_TRUE(IsEqual(input.rotationDegrees, 45.0f));
    EXPECT_TRUE(IsEqual(input.color[2], 0.6f));
    EXPECT_TRUE(input.outlineEnabled);
    EXPECT_TRUE(IsEqual(input.outlineThickness, 3.0f));
    EXPECT_TRUE(IsEqual(input.outlineColor[3], 0.3f));
}

TEST(SpriteRenderMathTests, SpriteRendererDrawsCommonDrawInput)
{
    auto rhiTexture = std::make_shared<FakeTexture>(64, 32);
    auto renderTexture = std::make_shared<Xelqoria::Graphics::Texture2D>();
    renderTexture->SetRHITexture(rhiTexture);

    Xelqoria::Graphics::SpriteDrawInput input{};
    input.texture = renderTexture;
    input.position = { 160.0f, -90.0f };
    input.scale = { 2.0f, 0.5f };
    input.rotationDegrees = 90.0f;
    input.color = { 0.25f, 0.5f, 0.75f, 0.8f };
    input.outlineEnabled = true;
    input.outlineThickness = 4.0f;
    input.outlineColor = { 0.1f, 0.2f, 0.3f, 0.4f };

    FakeGraphicsContext context{};
    Xelqoria::Graphics::SpriteBatch spriteBatch(context);
    Xelqoria::Graphics::InstancedSpriteRenderer& instancedSpriteRenderer = spriteBatch;

    spriteBatch.Begin();
    instancedSpriteRenderer.Draw(input);
    spriteBatch.End();

    EXPECT_EQ(context.drawCount, 1u);
    EXPECT_EQ(context.lastVertexCount, 6u);
    EXPECT_EQ(context.lastStartVertexLocation, 0u);
    EXPECT_EQ(context.lastBoundSlot, 0u);
    EXPECT_EQ(context.lastBoundTexture, nullptr);
    EXPECT_EQ(context.lastConstants.size(), Xelqoria::Graphics::QuadRenderConstantFloatCount);
    EXPECT_TRUE(IsEqual(context.lastConstants[0], 0.2f));
    EXPECT_TRUE(IsEqual(context.lastConstants[6], 1.0f));
    EXPECT_TRUE(IsEqual(context.lastConstants[7], 4.0f));
    EXPECT_TRUE(IsEqual(context.lastConstants[12], 0.25f));
    EXPECT_TRUE(IsEqual(context.lastConstants[15], 0.8f));
}

TEST(SpriteRenderMathTests, SpriteRendererDrawsSpriteUsingMaterialState)
{
    auto rhiTexture = std::make_shared<FakeTexture>(64, 32);
    auto renderTexture = std::make_shared<Xelqoria::Graphics::Texture2D>();
    renderTexture->SetRHITexture(rhiTexture);

    Xelqoria::Graphics::Sprite sprite;
    sprite.SetTexture(renderTexture);
    sprite.SetPosition(160.0f, -90.0f);
    sprite.SetScale(2.0f, 0.5f);
    sprite.SetRotationDegrees(90.0f);
    sprite.SetColor(0.25f, 0.5f, 0.75f, 0.8f);
    sprite.SetOutlineEnabled(true);
    sprite.SetOutlineThickness(4.0f);
    sprite.SetOutlineColor(0.1f, 0.2f, 0.3f, 0.4f);

    FakeGraphicsContext context{};
    Xelqoria::Graphics::SpriteRenderer spriteRenderer(context);

    spriteRenderer.Begin();
    spriteRenderer.Draw(sprite);
    spriteRenderer.End();

    EXPECT_EQ(context.drawCount, 1u);
    EXPECT_EQ(context.lastVertexCount, 6u);
    EXPECT_EQ(context.lastStartVertexLocation, 0u);
    EXPECT_EQ(context.lastBoundTexture, nullptr);
    EXPECT_EQ(context.lastConstants.size(), Xelqoria::Graphics::QuadRenderConstantFloatCount);
    EXPECT_TRUE(IsEqual(context.lastConstants[0], 0.2f));
    EXPECT_TRUE(IsEqual(context.lastConstants[6], 1.0f));
    EXPECT_TRUE(IsEqual(context.lastConstants[7], 4.0f));
    EXPECT_TRUE(IsEqual(context.lastConstants[8], 0.1f));
    EXPECT_TRUE(IsEqual(context.lastConstants[11], 0.4f));
    EXPECT_TRUE(IsEqual(context.lastConstants[12], 0.25f));
    EXPECT_TRUE(IsEqual(context.lastConstants[15], 0.8f));
}

TEST(SpriteRenderMathTests, SpritesShareMaterialStateWithoutImplicitCopy)
{
    auto sharedMaterial = std::make_shared<Xelqoria::Graphics::SpriteMaterial>();

    Xelqoria::Graphics::Sprite firstSprite;
    Xelqoria::Graphics::Sprite secondSprite;
    firstSprite.SetMaterial(sharedMaterial);
    secondSprite.SetMaterial(sharedMaterial);

    sharedMaterial->SetColor(0.2f, 0.4f, 0.6f, 0.8f);
    sharedMaterial->SetOutlineEnabled(true);
    sharedMaterial->SetOutlineThickness(5.0f);

    const Xelqoria::Graphics::SpriteDrawInput firstInput = firstSprite.ToDrawInput();
    const Xelqoria::Graphics::SpriteDrawInput secondInput = secondSprite.ToDrawInput();

    EXPECT_EQ(firstSprite.GetMaterial(), sharedMaterial);
    EXPECT_EQ(secondSprite.GetMaterial(), sharedMaterial);
    EXPECT_TRUE(IsEqual(firstInput.color[0], 0.2f));
    EXPECT_TRUE(IsEqual(secondInput.color[2], 0.6f));
    EXPECT_TRUE(firstInput.outlineEnabled);
    EXPECT_TRUE(secondInput.outlineEnabled);
    EXPECT_TRUE(IsEqual(firstInput.outlineThickness, 5.0f));
    EXPECT_TRUE(IsEqual(secondInput.outlineThickness, 5.0f));
}

TEST(SpriteRenderMathTests, SpriteCanUseIndividualMaterialState)
{
    auto sharedMaterial = std::make_shared<Xelqoria::Graphics::SpriteMaterial>();
    sharedMaterial->SetColor(0.2f, 0.4f, 0.6f, 0.8f);

    auto individualMaterial = std::make_shared<Xelqoria::Graphics::SpriteMaterial>();
    individualMaterial->SetColor(0.9f, 0.7f, 0.5f, 0.3f);

    Xelqoria::Graphics::Sprite sharedSprite;
    Xelqoria::Graphics::Sprite individualSprite;
    sharedSprite.SetMaterial(sharedMaterial);
    individualSprite.SetMaterial(individualMaterial);

    sharedMaterial->SetColor(0.1f, 0.2f, 0.3f, 0.4f);

    const Xelqoria::Graphics::SpriteDrawInput sharedInput = sharedSprite.ToDrawInput();
    const Xelqoria::Graphics::SpriteDrawInput individualInput = individualSprite.ToDrawInput();

    EXPECT_EQ(sharedSprite.GetMaterial(), sharedMaterial);
    EXPECT_EQ(individualSprite.GetMaterial(), individualMaterial);
    EXPECT_TRUE(IsEqual(sharedInput.color[0], 0.1f));
    EXPECT_TRUE(IsEqual(sharedInput.color[3], 0.4f));
    EXPECT_TRUE(IsEqual(individualInput.color[0], 0.9f));
    EXPECT_TRUE(IsEqual(individualInput.color[3], 0.3f));
}

TEST(SpriteRenderMathTests, SpriteCullingRejectsSpritesOutsideViewport)
{
    auto renderTexture = std::make_shared<Xelqoria::Graphics::Texture2D>();
    renderTexture->SetRHITexture(std::make_shared<FakeTexture>(64, 32));

    Xelqoria::Graphics::SpriteDrawInput visibleInput{};
    visibleInput.texture = renderTexture;
    visibleInput.position = { 300.0f, 0.0f };

    Xelqoria::Graphics::SpriteDrawInput hiddenInput = visibleInput;
    hiddenInput.position = { 400.0f, 0.0f };

    const Xelqoria::Graphics::SpriteCullRect viewportRect = Xelqoria::Graphics::MakeViewportCullRect(640, 480);
    EXPECT_TRUE(Xelqoria::Graphics::IsSpriteVisible(visibleInput, viewportRect));
    EXPECT_FALSE(Xelqoria::Graphics::IsSpriteVisible(hiddenInput, viewportRect));
}

TEST(SpriteRenderMathTests, SpriteBatchAndInstancedRendererUseCommonCulling)
{
    auto renderTexture = std::make_shared<Xelqoria::Graphics::Texture2D>();
    renderTexture->SetRHITexture(std::make_shared<FakeTexture>(64, 32));

    Xelqoria::Graphics::SpriteDrawInput input{};
    input.texture = renderTexture;
    input.position = { 250.0f, 0.0f };

    FakeGraphicsContext context{};
    context.Resize(640, 480);

    Xelqoria::Graphics::SpriteBatch spriteBatch(context);
    Xelqoria::Graphics::InstancedSpriteRenderer& instancedSpriteRenderer = spriteBatch;
    const Xelqoria::Graphics::SpriteCullRect narrowRect{
        -100.0f,
        -100.0f,
        100.0f,
        100.0f
    };

    spriteBatch.SetCullingRect(narrowRect);
    spriteBatch.Begin();
    instancedSpriteRenderer.Draw(input);
    spriteBatch.End();

    EXPECT_EQ(context.drawCount, 0u);

    spriteBatch.ClearCullingRect();
    spriteBatch.Begin();
    instancedSpriteRenderer.Draw(input);
    spriteBatch.End();

    EXPECT_EQ(context.drawCount, 1u);
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
