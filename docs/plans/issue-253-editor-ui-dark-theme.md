# ExecPlan: issue-253 Editor UI の見た目統一と Xelqoria Dark テーマ導入

## 目的

Editor 全体に Xelqoria Dark テーマを導入し、主要 Editor View の背景色、パネル色、境界線色、テキスト色、選択状態、hover 状態、ログ色、Scene View 表示を統一する。

## child issue 一覧

- issue-254: EditorTheme と Xelqoria Dark テーマを追加する
- issue-255: Editor View のパネル背景・ヘッダー・境界線を共通テーマに統一する
- issue-256: Editor タブの active / inactive / hover 表示をテーマ対応する
- issue-257: Hierarchy / Assets の選択状態・hover 表示をテーマ対応する
- issue-258: Inspector のセクション表示と入力欄の見た目をテーマ対応する
- issue-259: LogOutput / Console のログ表示色をテーマ対応する
- issue-260: Scene View の背景色・境界線・既存選択枠をテーマ対応する

## child issue 間の依存関係

- issue-254 は共通テーマ定義を追加するため、issue-255 以降の前提になる。
- issue-255 は主要 View のパネル背景・ヘッダー・境界線を扱うため、issue-257、issue-258、issue-259、issue-260 と一部ファイルが重なる可能性がある。
- issue-256 は Docking / タブ表示に限定できる場合、issue-254 完了後に並列対応可能。
- issue-257、issue-258、issue-259、issue-260 は対象 View が分かれているが、issue-255 の変更範囲確定後に並列可否を判断する。

## ブランチ戦略

- ブランチ運用ルール: `docs/agents/branch-rules.md`
- parent issue ブランチ: `issue-253`
- child issue ブランチ: `issue-254` から `issue-260`
- child issue の PR の向き: `issue-254` から `issue-260` -> `issue-253`
- parent issue ブランチの最終 PR の向き: `issue-253` -> `main`

## parent issue での作業範囲

parent issue では実装作業を行わない。

parent issue ブランチでは child issue の依存関係、ブランチ戦略、統合後の確認項目を管理する。すべての child issue が parent issue ブランチにマージされた後、統合確認・動作確認・最終的な差分確認のみ行う。

## 最終確認項目

- すべての child issue が parent issue ブランチにマージされている
- child issue 間の変更が競合していない
- Editor.UI に EditorTheme 相当のテーマ定義が存在する
- 主要 Editor View が共通テーマを参照している
- 背景色、パネル色、境界線色、テキスト色が統一されている
- タブ、選択状態、hover 状態、ログ色、Scene View 表示がテーマに沿っている
- 既存の編集操作、Scene 操作、Asset 操作、Docking 操作の挙動が変わっていない
- Editor / Graphics / RHI / Backends の依存方向を崩していない
- Direct3D 型が Editor.UI / Graphics 側に漏れていない
- Scene View のグリッド描画、アイコン描画、テーマ切り替え機能など対象外スコープが混入していない
- ビルドが通る
- 不要な一時ファイルや作業メモが含まれていない

## 完了条件

- すべての child issue が完了している
- 統合後の確認が完了している
- parent issue ブランチから `main` への PR が作成されている
