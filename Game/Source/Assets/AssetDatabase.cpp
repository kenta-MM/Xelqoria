#include "Assets/AssetDatabase.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>

#include "Assets/SpriteAssetLoader.h"
#include <AssetId.h>
#include <vector>

namespace
{
	std::string ReadTextFile(const std::filesystem::path& filePath)
	{
		std::ifstream stream(filePath, std::ios::binary);
		if (!stream) {
			return {};
		}

		return std::string(
			std::istreambuf_iterator<char>(stream),
			std::istreambuf_iterator<char>());
	}

	Xelqoria::Core::AssetId MakeAssetId(
		const std::filesystem::path& assetsRootPath,
		const std::filesystem::path& filePath)
	{
		std::error_code errorCode;
		std::filesystem::path relativePath = std::filesystem::relative(filePath, assetsRootPath, errorCode);
		if (errorCode) {
			relativePath = filePath.lexically_relative(assetsRootPath);
		}

		relativePath.replace_extension();
		return Xelqoria::Core::AssetId(relativePath.generic_string());
	}
}

namespace Xelqoria::Game::Assets
{
	AssetDatabase::AssetDatabase(std::filesystem::path assetsRootPath)
		: m_assetsRootPath(std::move(assetsRootPath))
	{
		RescanSprites();
	}

	void AssetDatabase::RescanSprites()
	{
		m_spriteEntries.clear();

		std::error_code errorCode;
		if (!std::filesystem::exists(m_assetsRootPath, errorCode) ||
			!std::filesystem::is_directory(m_assetsRootPath, errorCode)) {
			return;
		}

		for (std::filesystem::recursive_directory_iterator iterator(m_assetsRootPath, errorCode), end;
			iterator != end;
			iterator.increment(errorCode))
		{
			if (errorCode) {
				break;
			}

			const std::filesystem::directory_entry& entry = *iterator;
			if (!entry.is_regular_file(errorCode) || errorCode) {
				errorCode.clear();
				continue;
			}

			if (entry.path().extension() != ".sprite") {
				continue;
			}

			const std::string source = ReadTextFile(entry.path());
			if (source.empty()) {
				continue;
			}

			const SpriteAssetLoadResult loadResult = SpriteAssetLoader::LoadFromText(source);
			if (!loadResult.IsSuccess() || !loadResult.asset.has_value()) {
				continue;
			}

			m_spriteEntries.push_back(SpriteAssetEntry{
				MakeAssetId(m_assetsRootPath, entry.path()),
				entry.path(),
				*loadResult.asset
			});
		}

		std::sort(
			m_spriteEntries.begin(),
			m_spriteEntries.end(),
			[](const SpriteAssetEntry& lhs, const SpriteAssetEntry& rhs)
			{
				return lhs.assetId.GetValue() < rhs.assetId.GetValue();
			});
	}

	const std::vector<SpriteAssetEntry>& AssetDatabase::GetSpriteEntries() const
	{
		return m_spriteEntries;
	}
}
