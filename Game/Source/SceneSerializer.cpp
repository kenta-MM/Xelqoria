#include "SceneSerializer.h"

#include "SceneTextReader.h"
#include "SceneTextWriter.h"
#include <string>
#include <string_view>
#include "Scene.h"

namespace Xelqoria::Game
{
	std::string SceneSerializer::SaveToText(const Scene& scene)
	{
		return SceneTextWriter::Write(scene);
	}

	SceneLoadResult SceneSerializer::LoadFromText(std::string_view source)
	{
		return SceneTextReader::Read(source);
	}
}
