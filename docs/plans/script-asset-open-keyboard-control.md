# ExecPlan: Script Asset open and keyboard sprite control

## 目的

Assets 内の Script Asset をダブルクリックしたとき、対応する Editor 管理 C++ ソースを開けるようにする。
また、Sprite に割り当てた Script からキーボード入力を読み取り、Sprite の座標を更新できるようにする。

## 対象範囲

- 変更対象プロジェクト: Editor
- 変更対象ファイル:
  - `Editor/Source/Panels/Assets/AssetsPanelController.*`
  - `Editor/Source/Assets/ScriptAssetService.*`
  - `Editor/Source/Script/ScriptRuntimeSession.*`
  - `Editor/Source/App/Application.cpp`
  - `tests/Editor/Source/ScriptAssetServiceTests.cpp`
- 影響する機能:
  - Assets パネルの Script Asset ダブルクリック
  - Script Asset のドラッグ / Sprite 割り当て
  - Editor 再生中 Script Runtime の入力 API

## 前提

- parent issue: なし
- ブランチ運用ルール: `branch-rules.md`
- 現在ブランチ: `issue-344`
- PR の向き: ブランチ運用ルールに従う

## 実装方針

Script Asset のマニフェストと AssetId はプロジェクトルート基準で扱う。
Assets パネルは表示ルートとして Assets ルートを持ちつつ、Script 関連処理では EditorProjectInfo の project root を使う。

Script Runtime は Editor 専用機能として Editor に閉じる。
`InputSnapshot` を `ScriptRuntimeSession::Update` へ渡し、Script API ヘッダに `IsKeyDown` と矢印キー定数を追加する。

## 作業手順

1. Assets パネルにプロジェクトルート保持を追加する
2. Script Asset の source 解決と AssetId 生成をプロジェクトルート基準へ修正する
3. Script Runtime にキーボード入力 API を追加する
4. 初期 Script テンプレートを矢印キーで座標更新する内容へ更新する
5. 関連テストを更新する
6. Editor テストを実行する

## 検証方法

- 実行したビルド:
  - `MSBuild tests/Editor/Xelqoria.Tests.Editor.vcxproj /p:Configuration=Debug /p:Platform=x64 /m`
  - `MSBuild Editor/Xelqoria.Editor.vcxproj /p:Configuration=Debug /p:Platform=x64 /m`
- 実行したテスト:
  - `artifacts/x64/Debug/Xelqoria.Tests.Editor.exe`
- 手動確認項目:
  - Assets 内の `.script` ダブルクリックで `.xelqoria/Scripts/*.cpp` が開く
  - Sprite Asset に Script Asset を割り当て、再生中に矢印キーで Sprite 座標が変わる

## 完了条件

- [x] Script Asset の C++ ソース解決がプロジェクトルート基準になる
- [x] Script AssetId が Sprite 割り当て時と Script ビルド時で一致する
- [x] Script からキーボード入力を読んで Sprite 座標を更新できる
- [x] 関連テストが通る
- [x] レイヤー責務に違反していない
