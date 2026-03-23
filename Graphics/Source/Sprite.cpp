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

	void Sprite::SetPosition(const Vector2& position)
	{
		m_position = position;
	}

	void Sprite::SetPosition(float x, float y)
	{
		SetPosition(Vector2{ x, y });
	}

	const Vector2& Sprite::GetPosition() const
	{
		return m_position;
	}

	void Sprite::SetScale(const Vector2& scale)
	{
		m_scale = scale;
	}

	void Sprite::SetScale(float x, float y)
	{
		SetScale(Vector2{ x, y });
	}

	const Vector2& Sprite::GetScale() const
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
}
