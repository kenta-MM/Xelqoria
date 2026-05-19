#include "Sprite.h"

#include "SpriteMaterial.h"

#include <utility>

namespace Xelqoria::Graphics
{
	Sprite::Sprite()
		: m_material(std::make_shared<SpriteMaterial>())
	{
	}

	void Sprite::SetMaterial(std::shared_ptr<SpriteMaterial> material)
	{
		if (material)
		{
			m_material = std::move(material);
			return;
		}

		m_material = std::make_shared<SpriteMaterial>();
	}

	std::shared_ptr<SpriteMaterial> Sprite::GetMaterial() const
	{
		return m_material;
	}

	void Sprite::SetTexture(std::shared_ptr <Texture2D> texture)
	{
		m_material->SetTexture(std::move(texture));
	}

	std::shared_ptr<Texture2D> Sprite::GetTexture() const
	{
		return m_material->GetTexture();
	}

	void Sprite::SetTextureAssetId(Core::AssetId assetId)
	{
		m_material->SetTextureAssetId(std::move(assetId));
	}

	const Core::AssetId& Sprite::GetTextureAssetId() const
	{
		return m_material->GetTextureAssetId();
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
		m_material->SetColor(red, green, blue, alpha);
	}

	const std::array<float, 4>& Sprite::GetColor() const
	{
		return m_material->GetColor();
	}

	void Sprite::SetOutlineEnabled(bool enabled)
	{
		m_material->SetOutlineEnabled(enabled);
	}

	bool Sprite::IsOutlineEnabled() const
	{
		return m_material->IsOutlineEnabled();
	}

	void Sprite::SetOutlineThickness(float thickness)
	{
		m_material->SetOutlineThickness(thickness);
	}

	float Sprite::GetOutlineThickness() const
	{
		return m_material->GetOutlineThickness();
	}

	void Sprite::SetOutlineColor(float red, float green, float blue, float alpha)
	{
		m_material->SetOutlineColor(red, green, blue, alpha);
	}

	const std::array<float, 4>& Sprite::GetOutlineColor() const
	{
		return m_material->GetOutlineColor();
	}

	SpriteDrawInput Sprite::ToDrawInput() const
	{
		return SpriteDrawInput{
			m_material->GetTexture(),
			m_material->GetTextureAssetId(),
			m_position,
			m_scale,
			m_rotationDegrees,
			m_material->GetColor(),
			m_material->IsOutlineEnabled(),
			m_material->GetOutlineThickness(),
			m_material->GetOutlineColor()
		};
	}
}
