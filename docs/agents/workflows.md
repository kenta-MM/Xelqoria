# エージェント向けワークフロー

## 配置判断

1. 描画概念 → Graphics
2. OS 抽象 → Platform
3. OS 固有API → Platform.Win32 / Platform.Mac
4. GPU API 固有実装 → Backends
5. GPU抽象化 → RHI
6. エンジン基盤 → Core
7. ゲームロジック / 永続データ → Game
8. Editor UI シェル / Dock / UI 入力補助 → Editor.UI

迷った場合は architecture.md と project-map.md を確認する

## 実装手順

1. 変更の責務を特定
2. 適切なレイヤーを選択
3. 変更は最小限にする
4. OS 固有APIは Platform.* 以外に漏らさない
5. GPU API 固有コードは Backends 以外に漏らさない
6. SceneView / Game Preview は Editor.UI の描画先境界を経由して Editor 側描画へ接続する
7. ビルドと動作を確認

## チェックリスト

- レイヤー境界を破っていない
- Backends 以外で Direct3D 型を使用していない
- Platform.* 以外で OS ネイティブ API を直接使用していない
- Editor.UI から Backends / Direct3D / Platform.Win32 を参照していない
- Core から Editor.UI / Platform.Win32 を参照していない
- Game から Editor.UI / Platform.Win32 を参照していない
- データオブジェクトが描画していない
- Game から Backends を参照していない
- RHI に描画概念を追加していない
