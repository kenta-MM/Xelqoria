#include "SpriteMaterial.h"

#include <utility>

namespace Xelqoria::Graphics
{
	void SpriteMaterial::SetTexture(std::shared_ptr<Texture2D> texture)
	{
		m_texture = std::move(texture);
	}

	std::shared_ptr<Texture2D> SpriteMaterial::GetTexture() const
	{
		return m_texture;
	}

	void SpriteMaterial::SetTextureAssetId(Core::AssetId assetId)
	{
		m_textureAssetId = std::move(assetId);
	}

	const Core::AssetId& SpriteMaterial::GetTextureAssetId() const
	{
		return m_textureAssetId;
	}

	void SpriteMaterial::SetColor(float red, float green, float blue, float alpha)
	{
		m_color = { red, green, blue, alpha };
	}

	const std::array<float, 4>& SpriteMaterial::GetColor() const
	{
		return m_color;
	}

	void SpriteMaterial::SetOutlineEnabled(bool enabled)
	{
		m_outlineEnabled = enabled;
	}

	bool SpriteMaterial::IsOutlineEnabled() const
	{
		return m_outlineEnabled;
	}

	void SpriteMaterial::SetOutlineThickness(float thickness)
	{
		m_outlineThickness = thickness;
	}

	float SpriteMaterial::GetOutlineThickness() const
	{
		return m_outlineThickness;
	}

	void SpriteMaterial::SetOutlineColor(float red, float green, float blue, float alpha)
	{
		m_outlineColor = { red, green, blue, alpha };
	}

	const std::array<float, 4>& SpriteMaterial::GetOutlineColor() const
	{
		return m_outlineColor;
	}
}
