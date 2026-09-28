# FATAL FRAME II: Crimson Butterfly REMAKE — MixedNuts Mod Loader

MixedNuts の Mod で使う共通コードと、共通の Mod ローダーです。

Shared code and a common mod loader for the MixedNuts mods.

> **現在は共通コードのみです。** ローダー本体は開発中です。
> **Only the shared code is here for now.** The loader itself is in development.

## 構成 / Layout

| パス / Path | 内容 / Contents |
|---|---|
| `common/mixednuts/` | 各 Mod が使う共通コード（ヘッダのみ） / Shared header-only code used by each mod |
| `api/` | （予定）ローダーのプラグイン API / (planned) Plugin API of the loader |
| `loader/` | （予定）ローダー本体 / (planned) The loader itself |

## 共通コード / Shared code

名前空間は `mixednuts::`。C++17、Visual Studio 2022 の C++ ツールセットを前提にしています。
Namespace `mixednuts::`. Requires C++17 and the Visual Studio 2022 C++ toolset.

| ヘッダ / Header | 内容 / Contents |
|---|---|
| `log.hpp` | Mod ごとのログ（UTF-8、時刻付き） / Per-mod log file (UTF-8, timestamped) |
| `path.hpp` | ゲーム / DLL のフォルダ、パスの末尾比較 / Game and DLL folders, path suffix match |
| `ini.hpp` | ini の読み込み（行末コメントを除去） / ini reading (trailing comments stripped) |
| `proxy.hpp` | プロキシ DLL の転送 / Forwarding for proxy DLLs |
| `iat.hpp` | 輸入テーブルの枠の検索と差し替え / Finding and swapping import table slots |
| `file.hpp` | ファイルの読み書き（置き換えは一時ファイル経由） / File I/O (replacement via a temp file) |
| `bytes.hpp` | バイト列の読み書き / Unaligned reads and writes |
| `code.hpp` | `.text` の探索とコードの書き換え / Scanning `.text` and patching code |
| `env.hpp` | 環境情報のログ / Environment logging for bug reports |

## 使い方 / Usage

各 Mod のリポジトリに submodule として取り込み、`common` を include パスに加えます。
Add this repository to a mod as a submodule and put `common` on the include path.

```
git submodule add https://github.com/MixedNuts-Dev/fatal-frame2-remake-mod-loader.git mod-loader
cl ... /I mod-loader\common ...
```

```cpp
#include <mixednuts/log.hpp>

mixednuts::log::Open(modDir, L"example.log");
mixednuts::Log("[OK] Hello");
```

## クレジット / Credits

**Created by MixedNuts**

## ライセンス / License

MIT License — 詳細は [LICENSE](LICENSE) を参照してください。
See [LICENSE](LICENSE) for details.
