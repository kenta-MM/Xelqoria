# ExecPlan: issue-273 Assets ビューでプロジェクトルートの読込とファイル監視を実装する

## 目的

Assets ビューでプロジェクトルート全体を読み取り、ファイル変更を監視して UI に反映する。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/AssetsPanelController.h`, `Editor/Source/AssetsPanelController.cpp`, `Editor/Source/Application.cpp`
- 変更対象クラス: `AssetsPanelController`, `Application`
- 影響する機能: Assets ビューのプロジェクトルート表示、変更監視、選択維持

## 前提

- parent issue: #269
- 先行 issue: #270, #271, #272
- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-269`
- この child issue のブランチ: `issue-273`
- PR の向き: `issue-273` -> `issue-269`

## 実装方針

`issue-272` の固定レイアウトを前提に、既存の詳細 ListView 表示を活かす。プロジェクトルート全体は軽量なシグネチャポーリングで監視し、変更があった場合のみ現在表示中フォルダを再構築する。大量変更時の固まりを避けるため、走査数に上限を設ける。

## 作業手順

1. `issue-272` をこのブランチへローカルマージする
2. プロジェクトルート全体の変更シグネチャを計算する
3. 一定間隔でシグネチャを比較し、変更時のみ ListView を更新する
4. 選択中ファイルが削除された場合は選択解除する
5. 現在フォルダが削除された場合はプロジェクトルートへ戻す
6. ビルドまたはレイヤー依存チェックを実行する
7. PR を作成する

## 検証方法

- 実行するビルド: `dotnet build tools/LayerDependencyChecker/LayerDependencyChecker.csproj -c Debug`
- 実行するテスト: LayerDependencyChecker
- 手動確認項目: プロジェクトルート配下の追加 / 削除 / 更新 / 名前変更が Assets 表示へ反映される

## 完了条件

- プロジェクトルート全体のファイル変更を監視する
- 名前、更新日時、種類、サイズが表示される
- ファイル追加、削除、更新、名前変更が Assets ビューに反映される
- フォルダ追加、削除、名前変更が Assets ビューに反映される
- 大量変更時に UI が固まりにくいよう監視走査を制限する
- 選択中ファイル削除時に選択が解除される
- 既存のレイヤー責務を破壊していない
- `issue-273` から `issue-269` への PR が作成されている
