#include "Sprite.h"


namespace Xelqoria::Graphics
{
	void Sprite::SetTexture(std::shared_ptr <Texture2D> texture)
	{
		m_textuer = texture;
	}

	std::shared_ptr<Texture2D> Sprite::GetTexture() const
	{
		return m_textuer;
	}
}
