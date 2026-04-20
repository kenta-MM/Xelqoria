# Class Diagram Index

この文書は、プロジェクト単位のクラス図への入口として使う。

## 一覧

- Core: `docs/class_diagram/core-class-diagram.md`
- Xelqoria.Math: `docs/class_diagram/math-class-diagram.md`
- RHI: `docs/class_diagram/rhi-class-diagram.md`
- Graphics: `docs/class_diagram/graphics-class-diagram.md`
- Game: `docs/class_diagram/game-class-diagram.md`
- Backends.D3D11: `docs/class_diagram/backends-d3d11-class-diagram.md`
- Backends.D3D12: `docs/class_diagram/backends-d3d12-class-diagram.md`
- App: `docs/class_diagram/app-class-diagram.md`
- Editor: `docs/class_diagram/editor-class-diagram.md`
- Tests.Core: `docs/class_diagram/tests-core-class-diagram.md`
- Tests.Graphics: `docs/class_diagram/tests-graphics-class-diagram.md`
- Tests.Game: `docs/class_diagram/tests-game-class-diagram.md`
- Tests.Editor: `docs/class_diagram/tests-editor-class-diagram.md`

## 見方

- 各図は「対象プロジェクト + そのプロジェクトが直接参照している下位レイヤー」を含める
- `RHI` は単体で完結する
- `Backends` の図には、実装対象である `RHI` のインターフェースを含める
- `App` と `Editor` は組み立て層なので、依存している共有ライブラリを含める

## 更新ルール

- `.h` の公開型や依存関係を変えた時は、対応するクラス図も更新する
- 全体図を 1 枚で増やすより、プロジェクト単位の図を小さく保つ
- 詳細化したい時は、プロジェクト内でさらにテーマ別図を追加してよい
