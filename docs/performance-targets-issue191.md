# Issue 191 Performance Targets

## 目的

親Issue #186 の性能目標として、100,000 個・Debug・D3D11/D3D12・フルHD・60fps を計測し、必要な最適化の入口を記録する。

## 前提

- 対象ブランチ: `issue-191`
- 構成: Debug x64
- 解像度目標: 1920 x 1080
- 個数目標: 100,000 個
- FPS目標: 60fps

## 現在の実装状態

- Sprite 描画は `Graphics::SpriteDrawInput` を共通入力として扱う。
- CPU SpriteBatch / GPU InstancedSpriteRenderer の共通入口で `SpriteCulling` を利用できる。
- Physics は `Physics2D` と `PhysicsSpatialHash2D` により、AABB 当たり判定・反発・spatial hash 候補抽出を提供する。

## 計測結果

| 対象 | 構成 | 結果 | 60fps目標 |
| --- | --- | --- | --- |
| Sprite 100,000 個 D3D11 フルHD | Debug x64 | 未計測 | 未判定 |
| Sprite 100,000 個 D3D12 フルHD | Debug x64 | 未計測 | 未判定 |
| Physics 100,000 個 | Debug x64 | 未計測 | 未判定 |

## 未計測理由

- 現在の `App` は対話ウィンドウとして起動し、フレーム数・Sprite数・解像度・バックエンドを指定して自動終了するベンチマークモードを持たない。
- `App::Initialize` は D3D11 固定で起動しており、同一条件で D3D12 を切り替えて計測する入口がない。
- このエージェント実行環境では、100,000 個の D3D11/D3D12 フルHD描画を人間が観測可能なウィンドウとして継続実行し、fps を安定記録する手段を確認できなかった。

## 必要な次作業

次のいずれかを追加すると、性能目標を再現可能に計測できる。

- `App` に `--backend d3d11|d3d12`、`--width 1920`、`--height 1080`、`--sprite-count 100000`、`--benchmark-frames <N>` を受け取るベンチマークモードを追加する。
- ベンチマークモードで平均 fps、最小 fps、CPU側 culling 時間、Physics broad phase 時間を標準出力またはログに記録する。
- D3D11 と D3D12 を同一 Sprite 配置・同一フレーム数で実行し、上記の表を実測値で更新する。

## 今回の確認

- `tools/LayerDependencyChecker` の Debug ビルドを実行した。
- `tests/Graphics/Xelqoria.Tests.Graphics.vcxproj` Debug x64 をビルドし、Graphics テストを実行した。
- `tests/Game/Xelqoria.Tests.Game.vcxproj` Debug x64 をビルドし、Game テストを実行した。
- 既存テストにより、共通 Sprite 入力、Sprite カリング、Physics2D、spatial hash の単体動作を確認した。
