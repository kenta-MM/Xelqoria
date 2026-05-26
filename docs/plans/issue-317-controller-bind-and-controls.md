# Issue 317: Controller Bind And CollectControls

## Goal

Controller の Bind 先を `EditorShell` から対応する PanelView へ移し、`EditorShell::CollectControls()` を固定長配列から `std::vector<HWND>` へ変更する。

## Context

- Parent issue: #313
- Depends on: #316
- Branch: `issue-317`

## Plan

- [x] `issue-316` を取り込む。
- [x] 各 PanelView に Controller が必要とする HWND getter を追加する。
- [x] 対象 Controller の `Bind` 引数を PanelView に変更する。
- [x] Project UI は既存配置に合わせて `SceneViewPanelView` から Bind する。
- [x] `EditorShell::CollectControls()` を `std::vector<HWND>` 返却に変更し、PanelView の `CollectControls` で集約する。
- [x] ビルドと検証を実行する。

## Verification

- [x] `git diff --check`
- [x] `dotnet run --project Tools/LayerDependencyChecker/LayerDependencyChecker.csproj -- .`
- [x] `MSBuild.exe Editor\Xelqoria.Editor.vcxproj /p:Configuration=Debug /p:Platform=x64`
