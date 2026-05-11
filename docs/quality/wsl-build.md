# WSL からのビルド

Xelqoria は Win32/Direct3D を使うため、WSL の Linux ネイティブ C++ ツールチェーンではなく、WSL から Windows 側の Visual Studio/MSBuild を呼び出してビルドする。

## 前提

- Windows 側に Visual Studio または Build Tools が入っていること
- MSBuild と C++ workload が入っていること
- Windows 側または WSL 側に .NET SDK 8 以上が入っていること

## ビルド

```bash
tools/wsl/build.sh
```

既定では `Debug|x64` で `Xelqoria.slnx` をビルドする。

```bash
tools/wsl/build.sh -c Release
tools/wsl/build.sh tests/Editor/Xelqoria.Tests.Editor.vcxproj
```

## テスト

```bash
tools/wsl/test.sh
```

ビルド済みバイナリだけを実行する場合:

```bash
tools/wsl/test.sh --no-build
```
