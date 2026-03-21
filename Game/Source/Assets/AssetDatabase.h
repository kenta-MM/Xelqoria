#pragma once

#include <filesystem>
#include <vector>

#include "AssetId.h"
#include "SpriteAsset.h"

namespace Xelqoria::Game::Assets
{
	/// <summary>
	/// Sprite 一覧表示に利用する走査結果 1 件分を表す。
	/// </summary>
	struct SpriteAssetEntry
	{
		/// <summary>
		/// Editor や Runtime から参照する Sprite アセット識別子を表す。
		/// </summary>
		Core::AssetId assetId{};

		/// <summary>
		/// 読み込んだ元ファイルのパスを表す。
		/// </summary>
		std::filesystem::path filePath{};

		/// <summary>
		/// 解析済みの SpriteAsset 本体を表す。
		/// </summary>
		SpriteAsset asset{};
	};

	/// <summary>
	/// assets フォルダを走査して Sprite アセット一覧を構築する簡易データベースを表す。
	/// </summary>
	class AssetDatabase
	{
	public:
		/// <summary>
		/// 走査対象の assets ルートを指定して AssetDatabase を生成する。
		/// </summary>
		/// <param name="assetsRootPath">走査対象の assets フォルダパス。</param>
		explicit AssetDatabase(std::filesystem::path assetsRootPath);

		/// <summary>
		/// assets フォルダを再走査して Sprite 一覧を更新する。
		/// </summary>
		void RescanSprites();

		/// <summary>
		/// 現在保持している Sprite 一覧を取得する。
		/// </summary>
		/// <returns>Sprite 一覧の読み取り専用参照。</returns>
		const std::vector<SpriteAssetEntry>& GetSpriteEntries() const;

	private:
		std::filesystem::path m_assetsRootPath;
		std::vector<SpriteAssetEntry> m_spriteEntries{};
	};
}
