#include "SceneSerializer.h"

#include <iomanip>
#include <sstream>

#include "SceneSaveFormat.h"

namespace
{
	void AppendVector3(std::ostringstream& stream, const Xelqoria::Game::Vector3& value)
	{
		stream << value.x << "," << value.y << "," << value.z;
	}
}

namespace Xelqoria::Game
{
	std::string SceneSerializer::SaveToText(const Scene& scene)
	{
		std::ostringstream stream;
		stream << std::fixed << std::setprecision(6);
		stream << "magic=" << SceneSaveFormatMagic << "\n";
		stream << "version=" << SceneSaveFormatVersion << "\n";

		const auto entities = scene.GetEntities();
		for (std::size_t entityIndex = 0; entityIndex < entities.size(); ++entityIndex)
		{
			const auto& entity = entities[entityIndex];
			const auto& transform = entity.GetTransform();
			const auto prefix = "entity." + std::to_string(entityIndex);

			stream << prefix << ".id=" << entity.GetId() << "\n";
			stream << prefix << ".transform.position=";
			AppendVector3(stream, transform.position);
			stream << "\n";
			stream << prefix << ".transform.rotation=";
			AppendVector3(stream, transform.rotation);
			stream << "\n";
			stream << prefix << ".transform.scale=";
			AppendVector3(stream, transform.scale);
			stream << "\n";

			const auto spriteComponent = entity.GetSpriteComponent();
			if (spriteComponent.has_value() && !spriteComponent->get().spriteAssetRef.IsEmpty()) {
				stream << prefix << ".spriteRef=" << spriteComponent->get().spriteAssetRef.GetValue() << "\n";
			}
		}

		return stream.str();
	}
}
