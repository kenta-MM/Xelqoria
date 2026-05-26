# ExecPlan: issue-302 Hierarchy の GameObject / Component ツリー表示改善

## 目的

Hierarchy で GameObject と Component の親子関係を明確にし、Component 行を選択できるようにする。

## 対象範囲

- 変更対象プロジェクト: `Editor`
- 変更対象ファイル: `Editor/Source/HierarchyPanelController.*`
- 変更対象クラス: `HierarchyPanelController`
- 影響する機能: Hierarchy 表示、Hierarchy 選択状態

## 前提

- parent issue: issue-299
- 先行 child issue: issue-300
- parent issue ブランチ: `issue-299`
- この child issue のブランチ: `issue-302`
- PR の向き: `issue-302` -> `issue-299`

## 実装方針

GameObject を親行として表示し、SpriteComponent、Material、Collider2D を Component 子行として表示する。Material は SpriteComponent の Material 参照を独立 Component として扱う。

## 作業手順

1. Hierarchy の表示行種別へ Component 種別を追加する
2. GameObject 行の展開状態に応じて Component 子行を表示する
3. 選択中行種別を外部から取得できるようにする
4. Editor ビルドと Editor テストを実行する

## 検証方法

- `Editor/Xelqoria.Editor.vcxproj` Debug x64 ビルド
- `tests/Editor/Xelqoria.Tests.Editor.vcxproj` Debug x64 ビルド
- `artifacts/x64/Debug/Xelqoria.Tests.Editor.exe`

## 完了条件

- GameObject が親ノードとして表示される
- SpriteComponent / Material / Collider2D が GameObject の子として表示される
- Component を選択できる
- 展開状態が保持される
