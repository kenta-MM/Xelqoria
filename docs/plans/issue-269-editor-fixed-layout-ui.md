# ExecPlan: issue-269 Xelqoria Editor の固定レイアウト UI を画像ベースで刷新する

## 目的

Xelqoria Editor のメイン画面を、既存レイヤー責務を維持したまま、固定レイアウトのダークテーマ UI として刷新する。

## child issue 一覧

- issue-270: Editor 全体のダークテーマ基盤と共通 UI 描画を実装する
- issue-271: TopBar / StatusBar を画像ベースで実装する
- issue-272: 固定レイアウトで Assets / Hierarchy / Scene / Inspector / Console を配置する
- issue-273: Assets ビューでプロジェクトルートの読込とファイル監視を実装する
- issue-274: Hierarchy ビューで Entity 一覧表示と選択同期を実装する
- issue-275: Scene ビューでグリッド・カメラ枠・選択枠・軸ギズモを表示する
- issue-276: Inspector で Transform / Sprite / Material / Script を編集できるようにする
- issue-277: Console ビューでダミーログ表示とフィルター UI を実装する

## child issue 間の依存関係

- issue-270 は共通テーマ基盤のため先行する。
- issue-271 は issue-270 のテーマ基盤を利用する。
- issue-272 は issue-270 と issue-271 の表示領域を前提に固定レイアウトを整える。
- issue-273 から issue-277 は issue-272 の View / Panel 配置を前提にする。
- issue-274、issue-275、issue-276 は選択同期を共有するため、競合がある場合は直列で扱う。
- issue-277 は Console パネルに閉じるため、issue-272 後は比較的独立して扱える。

## ブランチ戦略

- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-269`
- child issue ブランチ: `issue-270` から `issue-277`
- child issue の PR の向き: `issue-<child>` -> `issue-269`
- parent issue ブランチの最終 PR の向き: `issue-269` -> `main`
- 後続タスクが先行実装を必要とする場合、後続 child ブランチ上で必要な先行 child ブランチをローカルマージしてから実装する。

## parent issue での作業範囲

parent issue では原則として実装作業を行わない。

すべての child issue が parent issue ブランチにマージされた後、統合確認・動作確認・最終的な差分確認のみ行う。

## 最終確認項目

- すべての child issue が parent issue ブランチにマージされている
- child issue 間の変更が競合していない
- parent issue の目的を満たしている
- レイヤー責務に違反していない
- ビルドが通る
- 必要なテストが通る
- 必要なドキュメントが更新されている
- 不要な一時ファイルや作業メモが含まれていない

## 完了条件

- すべての child issue が完了している
- 統合後の確認が完了している
- parent issue ブランチから `main` への PR が作成されている
