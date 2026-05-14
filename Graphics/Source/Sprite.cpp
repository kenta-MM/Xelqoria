#include "Sprite.h"

#include <utility>

namespace Xelqoria::Graphics
{
	void Sprite::SetTexture(std::shared_ptr <Texture2D> texture)
	{
		m_texture = std::move(texture);
	}

	std::shared_ptr<Texture2D> Sprite::GetTexture() const
	{
		return m_texture;
	}

	void Sprite::SetTextureAssetId(Core::AssetId assetId)
	{
		m_textureAssetId = std::move(assetId);
	}

	const Core::AssetId& Sprite::GetTextureAssetId() const
	{
		return m_textureAssetId;
	}

	void Sprite::SetPosition(const Xelqoria::Math::Vector2& position)
	{
		m_position = position;
	}

	void Sprite::SetPosition(float x, float y)
	{
		SetPosition(Xelqoria::Math::Vector2{ x, y });
	}

	const Xelqoria::Math::Vector2& Sprite::GetPosition() const
	{
		return m_position;
	}

	void Sprite::SetScale(const Xelqoria::Math::Vector2& scale)
	{
		m_scale = scale;
	}

	void Sprite::SetScale(float x, float y)
	{
		SetScale(Xelqoria::Math::Vector2{ x, y });
	}

	const Xelqoria::Math::Vector2& Sprite::GetScale() const
	{
		return m_scale;
	}

	void Sprite::SetRotationDegrees(float rotationDegrees)
	{
		m_rotationDegrees = rotationDegrees;
	}

	float Sprite::GetRotationDegrees() const
	{
		return m_rotationDegrees;
	}

	void Sprite::SetColor(float red, float green, float blue, float alpha)
	{
		m_color = { red, green, blue, alpha };
	}

	const std::array<float, 4>& Sprite::GetColor() const
	{
		return m_color;
	}

	void Sprite::SetOutlineEnabled(bool enabled)
	{
		m_outlineEnabled = enabled;
	}

	bool Sprite::IsOutlineEnabled() const
	{
		return m_outlineEnabled;
	}

	void Sprite::SetOutlineThickness(float thickness)
	{
		m_outlineThickness = thickness;
	}

	float Sprite::GetOutlineThickness() const
	{
		return m_outlineThickness;
	}

	void Sprite::SetOutlineColor(float red, float green, float blue, float alpha)
	{
		m_outlineColor = { red, green, blue, alpha };
	}

	const std::array<float, 4>& Sprite::GetOutlineColor() const
	{
		return m_outlineColor;
	}
}
