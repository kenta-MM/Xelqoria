#include "SceneTextWriter.h"

#include <iomanip>
#include <sstream>
#include <string_view>

#include "SceneSaveFormat.h"
#include <string>
#include <ios>
#include "Scene.h"
#include <Vector3.h>

namespace
{
	/// <summary>
	/// Vector3 をシーン保存形式の `x,y,z` 文字列として追記する。
	/// </summary>
	/// <param name="stream">出力先ストリーム。</param>
	/// <param name="value">追記するベクトル値。</param>
	void AppendVector3(std::ostringstream& stream, const Xelqoria::Math::Vector3& value)
	{
		stream << value.x << "," << value.y << "," << value.z;
	}

	/// <summary>
	/// 文字列を引用符付きの保存形式として追記する。
	/// </summary>
	/// <param name="stream">出力先ストリーム。</param>
	/// <param name="value">追記する文字列。</param>
	void AppendQuotedString(std::ostringstream& stream, std::string_view value)
	{
		stream << std::quoted(std::string(value));
	}
}

namespace Xelqoria::Game
{
	std::string SceneTextWriter::Write(const Scene& scene)
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
			stream << prefix << ".name=";
			AppendQuotedString(stream, entity.GetName());
			stream << "\n";
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
			stream << prefix << ".hasSpriteComponent=" << (spriteComponent.has_value() ? "true" : "false") << "\n";
			if (spriteComponent.has_value())
			{
				stream << prefix << ".spriteRef=" << spriteComponent->get().spriteAssetRef.GetValue() << "\n";
				if (false == spriteComponent->get().materialAssetRef.IsEmpty())
				{
					stream << prefix << ".materialRef=" << spriteComponent->get().materialAssetRef.GetValue() << "\n";
				}
			}
		}

		return stream.str();
	}
}
