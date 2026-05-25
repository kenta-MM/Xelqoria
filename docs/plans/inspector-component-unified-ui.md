# ExecPlan: Inspector Component unified UI

## 目的

Entity に付く Component と、SpriteComponent が参照する Material の主要項目を Inspector 内で一覧・編集できるようにする。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/EditorShell.*`, `Editor/Source/InspectorPanelController.*`, `Editor/Source/Application.*`
- 変更対象クラス: `EditorShell`, `InspectorPanelController`, `Application`
- 影響する機能: Inspector 表示、Material 編集、Collider2D 編集、右ペインの通常表示タブ

## 前提

- ブランチ運用ルール: `docs/agents/branch-rules.md`
- 今回は Editor 層の Win32 UI と編集フローに閉じる。
- Game / Graphics / RHI / Backends の依存方向は変更しない。

## 実装方針

Inspector 内に `Transform`, `SpriteComponent`, `Material Details`, `Collider2DComponent`, `Add Component` の順で UI を配置する。

Material は SpriteComponent の Material 参照先を編集対象とし、既存の Material Asset 保存経路を利用する。Collider2D は専用ビューの通常利用をやめ、Inspector 内の Component セクションで編集する。Material / Collider2D 専用パネルは通常 Dock から外し、既存コードが残る場合も通常編集導線では開かない。

## 作業手順

1. 現在の Inspector / Material / Collider2D UI 生成と入力反映を確認する
2. EditorShell に Inspector 内 Material Details と Add Component 用の control を追加・配置する
3. Collider2D controls を Inspector レイアウトへ戻し、専用 Collider2D タブを通常 Dock から外す
4. InspectorPanelController で Material Details の表示・編集結果を扱う
5. Application で Material Details 変更を Material Asset 保存へ接続する
6. ビルドで検証する

## 検証方法

- 実行するビルド: `MSBuild.exe Editor\Xelqoria.Editor.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`
- 手動確認項目:
  - Inspector 内に指定順のセクションが表示される
  - Material 専用ビューを開かずに Material Details を編集できる
  - Collider2D 専用ビューを開かずに Collider2DComponent を編集できる
  - Material / Collider2D の専用タブが通常 Dock に出ない

## 完了条件

- Inspector 内に指定セクションが表示される
- Material Details の編集が Material Asset 保存に反映される
- Collider2DComponent の編集が既存 Scene 変更経路に反映される
- Editor ビルドが通る
- レイヤー責務に違反していない
