# ExecPlan: issue-338 Project Settings をメニューから開く導線を追加する

## 目的

`ProjectSettings/project_settings.json` を Assets ビューではなく、ヘッダーの `Project > Settings...` から編集できるようにする。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Application.h`, `Application.cpp`
- 変更対象クラス: `Xelqoria::Editor::Application`
- 影響する機能: Project メニュー、Project Settings 編集導線

## 前提

- parent issue: issue-334
- 先行 child issue: issue-335
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-334`
- この child issue のブランチ: `issue-338`
- PR の向き: `issue-338` -> `issue-334`

## 実装方針

既存の Project メニューに用意されている Settings コマンドを実装し、現在開いているプロジェクトの `EditorProjectInfo::projectSettingsFilePath` を開く。

編集画面は Editor 内のモーダルウィンドウとして実装し、`project_settings.json` の UTF-8 テキストをそのまま編集・保存できるようにする。設定項目の追加や Assets ビューの表示ルート変更はこの issue では扱わない。

## 作業手順

1. ヘッダーの Project メニュー表記を `Project` にする
2. Settings メニュー項目を `Settings...` として表示する
3. Settings コマンドから Project Settings 編集画面を開く
4. `project_settings.json` を UTF-8 テキストとして読み込む
5. Save 操作で `project_settings.json` に書き戻す
6. 保存結果またはエラーを LogOutput と SceneView ステータスに表示する
7. Editor ビルドと差分チェックを実行する

## 検証方法

- 実行するビルド: `MSBuild Editor/Xelqoria.Editor.vcxproj /p:Configuration=Debug /p:Platform=x64 /m`
- 実行する静的確認: `git diff --check`
- 手動確認項目: メニュー表記が `Project > Settings...` であること、Project Settings 編集画面が開き保存できること

## 完了条件

- ヘッダーに `Project > Settings...` が表示される
- `Project > Settings...` から `ProjectSettings/project_settings.json` の編集画面を開ける
- 編集画面から `project_settings.json` を保存できる
- `ProjectSettings/` を Assets ビュー上の編集導線として扱わない
- 関連ビルドが通る
- `issue-338` から `issue-334` への PR が作成されている
