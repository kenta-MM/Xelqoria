# エージェント向けワークフロー

## 配置判断

1. 描画概念 → Graphics
2. OS 抽象 → Platform
3. Window / EventLoop / Input / Dialog / Clipboard / Cursor の OS 固有API → Platform.Win32 / Platform.Mac
4. GPU API 固有実装 → Backends
5. GPU抽象化 → RHI
6. エンジン基盤 → Core
7. ゲームロジック / 永続データ → Game
8. OS 非依存の Editor UI 入力補助 / SceneView 表示境界 → Editor.UI

迷った場合は architecture.md と project-map.md を確認する

## 実装手順

1. 変更の責務を特定
2. 適切なレイヤーを選択
3. 変更は最小限にする
4. Window / EventLoop / Input / Dialog / Clipboard / Cursor の OS 固有APIは Platform.* 以外に漏らさない
5. Editor の OS ネイティブ UI シェルは Editor に閉じ、Editor.UI へ持ち込まない
6. GPU API 固有コードは Backends 以外に漏らさない
7. SceneView / Game Preview は Editor.UI の描画先境界を経由して Editor 側描画へ接続する
8. ビルドと動作を確認

## チェックリスト

- レイヤー境界を破っていない
- Backends 以外で Direct3D 型を使用していない
- Window / EventLoop / Input / Dialog / Clipboard / Cursor の OS ネイティブ API が Platform.* 以外へ漏れていない
- Editor.UI に OS ネイティブ UI シェルが入っていない
- Editor.UI から Backends / Direct3D / Platform.Win32 を参照していない
- Core から Editor.UI / Platform.Win32 を参照していない
- Game から Editor.UI / Platform.Win32 を参照していない
- データオブジェクトが描画していない
- Game から Backends を参照していない
- RHI に描画概念を追加していない
