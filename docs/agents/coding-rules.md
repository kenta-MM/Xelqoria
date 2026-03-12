# Coding Rules For Agents

この文書は、Xelqoria で AI エージェント（Codex 等）がコードを追加・変更する際の共通ルールを定義する。

## 基本方針

1. 依存方向は `Game -> Graphics -> RHI -> Backends` を厳守する
2. 既存レイヤーの責務を崩す実装を行わない
3. 変更は最小限にし、影響範囲を明確にする

## 必須ルール

- Rule1: Sprite は描画しない
- Rule2: Renderer が描画する
- Rule3: RHI は低レベル API を提供する
- Rule4: RHI は Graphics 概念を知らない
- Rule5: Game は Direct3D を知らない

## 禁止事項

- Graphics に `ID3D11Device` / `ID3D11Texture2D` / `ID3D12Device` 等を入れる
- RHI に `Sprite` / `Camera` / `Material` / `SpriteRenderer` を追加する
- Game から Backends を直接参照する
- 依存方向に逆らった `#include` を作る

## AI コード生成ルール

新規コード追加時は次を強制する。

- 描画機能 (`Sprite`, `Camera`, `Material`) は Graphics に置く
- GPU API 直接操作コードは Backends に置く
- GPU 抽象 (`Buffer`, `Texture`, `Shader`, `Pipeline`) は RHI に置く
- エンジンシステム (`RenderSystem`, `SceneSystem`, `ResourceManager`) は Graphics または Core
- ゲームロジック (`Player`, `Enemy`, `Stage`, `UI`) は Game

## ドキュメントコメント規約（必須）

1. コーディング時は必ずドキュメントコメントを作成する
2. クラス・構造体・関数・メソッドなど、公開 API だけでなく意図が重要な要素にもコメントを付与する
3. ドキュメントコメントは「何をするか」「入力と出力」「前提条件」を簡潔に記載する
4. ドキュメントコメントはファイル先頭ではなく、対象の宣言（class / struct / enum / 関数）の直前に記述する
5. 形式は DocFX 互換の XML ドキュメントコメントを使用し、`/// <summary>` を基本とする
6. 引数と戻り値がある場合は `/// <param name="...">` と `/// <returns>` を記述する

## コメント言語ルール（必須）

1. ドキュメントコメントを含むコメント本文は必ず日本語で記述する
2. 変数名・関数名・型名などの識別子は変更せず、そのまま記載してよい
3. 英語の仕様語が必要な場合は、識別子や用語のみ英語のまま使用し、説明文は日本語で記述する

## 変更レビュー用チェック項目

- レイヤー責務が明確か
- API 抽象が破綻していないか
- Direct3D 固有コードが Backends に閉じているか
- 拡張時に 2D / 3D 共存を阻害しないか

## 迷ったときの優先規則

1. 依存方向維持を最優先
2. 低レベル抽象は RHI、高レベル描画は Graphics
3. 不明な場合は Graphics を優先して責務を再確認
