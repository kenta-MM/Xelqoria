# ExecPlan: issue-299 Xelqoria Editor UI改善

## 目的

Inspector、Hierarchy、Log、共通パネルの見た目と情報構造を整理し、既存機能を維持したまま視認性と操作性を改善する。

## child issue 一覧

- issue-300: 共通UI定数とUI状態永続化の基盤追加
- issue-301: Inspector の Component カードUI改善
- issue-302: Hierarchy の GameObject / Component ツリー表示改善
- issue-303: Log のフィルター、種別表示、Clear 表示制御改善
- issue-304: 全体の見た目統一と受け入れ確認

## child issue 間の依存関係

- issue-300 は Inspector 折りたたみ状態と Hierarchy 展開状態の土台として先行する。
- issue-301 と issue-302 は Component 選択と Inspector 強調表示で連動するため、同じ変更単位で確認する。
- issue-303 は Log パネル内で独立しているが、共通見た目統一の対象に含める。
- issue-304 は issue-300 から issue-303 の統合確認として最後に確認する。

## ブランチ戦略

- ブランチ運用ルール: `branch-rules.md`
- parent issue ブランチ: `issue-299`
- child issue ブランチ: 本対応では完成図へ一括で寄せる依頼のため、`issue-299` 上で統合実装する。
- PR の向き: `issue-299` から `main`

## parent issue での作業範囲

既存の Editor Win32 UI コントロールを大きく作り替えず、既存コントローラに状態管理、Component 行、カード見出し、Log フィルターを追加する。

## 最終確認項目

- Inspector に Component カード見出し、折りたたみ、選択 Component 強調がある
- Hierarchy に GameObject 親行と Component 子行がある
- Hierarchy 展開状態と Inspector 折りたたみ状態がローカル状態ファイルへ保存される
- Log に All / Info / Editor / Warn / Error フィルターがある
- Clear は画面表示のみを消し、内部ログデータは保持する
- 削除系ボタンは通常操作と区別できる
- Editor ビルドが通る

## 完了条件

- issue-300 から issue-304 の受け入れ条件に対応している
- レイヤー責務に違反していない
- `Editor/Xelqoria.Editor.vcxproj` の Debug x64 ビルドが成功している
