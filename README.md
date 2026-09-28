# MixedNuts Mod Loader

**FATAL FRAME II: Crimson Butterfly REMAKE**（零 〜紅い蝶〜 REMAKE / Steam AppID 3920610）用の、
MixedNuts の Mod に共通のローダーです。
The common mod loader for the MixedNuts mods for FATAL FRAME / PROJECT ZERO II:
Crimson Butterfly REMAKE (Steam, AppID 3920610).

次の Mod は、このローダーの上で動きます（それぞれ 2.0.0 以降）。
The following mods run on this loader (each from 2.0.0 on):

- [Native 120FPS Option](https://github.com/MixedNuts-Dev/fatal-frame2-remake-native-120fps)
- [Mouse Wheel Camera Speed](https://github.com/MixedNuts-Dev/fatal-frame2-remake-mouse-wheel-camera-speed)
- [TwinSwap](https://github.com/MixedNuts-Dev/fatal-frame2-remake-twin-swap)

以前はそれぞれが別の名前の DLL（`dinput8.dll` / `version.dll` / `xinput1_4.dll`）で単独に
動いていました。ローダーにまとめたことで、ゲームのフォルダに置く DLL は 1 つになり、
**同じファイル（`root.rdb` など）を複数の Mod が順に改変できる**ようになりました。

Each of them used to run on its own, under a different DLL name (`dinput8.dll` /
`version.dll` / `xinput1_4.dll`). With the loader there is a single DLL in the game folder,
and **several mods can modify the same file (such as `root.rdb`) one after another.**

## 導入 / Installation

**ビルドは不要です。** [Releases](../../releases) から配布物をダウンロードし、中身の
`dinput8.dll` と `MixedNuts` フォルダを、ゲームのルート（`FatalFrameII.exe` と同じ場所）に
コピーします。Mod はその後、`MixedNuts\Mods\` の中に入れます。

**No build required.** Download the archive from [Releases](../../releases) and copy
`dinput8.dll` and the `MixedNuts` folder into the game's root directory (the folder
containing `FatalFrameII.exe`). Mods then go inside `MixedNuts\Mods\`.

```
FatalFrameII/
  FatalFrameII.exe
  dinput8.dll                          <- ローダー / the loader
  MixedNuts/MixedNutsLoader.dll        <- ローダー本体 / the loader itself
  MixedNuts/loader.ini
  MixedNuts/loader.log                 <- 起動時に生成 / generated at launch
  MixedNuts/cache/                     <- 起動時に生成 / generated at launch
  MixedNuts/Mods/native120fps/         <- Mod
  MixedNuts/Mods/twinswap/             <- Mod
```

削除は `dinput8.dll` と `MixedNuts` フォルダを消すだけです。ゲームのファイルは一切変更しません。
To uninstall, delete `dinput8.dll` and the `MixedNuts` folder. No game files are modified.

### 旧版からの移行 / Upgrading from the 1.x mods

各 Mod の 1.x を入れている場合は、ゲームのルートから次を**削除してから**入れてください。
If you have the 1.x versions installed, **delete the following** from the game's root first:

| 旧版 / Old version | 削除するもの / Delete |
|---|---|
| Native 120FPS Option 1.x | `dinput8.dll`、`Mods\native120fps\` |
| Mouse Wheel Camera Speed 1.x | `version.dll`、`Mods\wheelspeed\` |
| TwinSwap 1.x | `xinput1_4.dll`、`Mods\twinswap\` |

`Mods` フォルダに他の Mod が入っている場合は、上の 3 つのフォルダだけを消してください。
旧版の DLL が残っていると、ローダーはその Mod を読み込まず、`MixedNuts\loader.log` に
削除を促すメッセージを書きます（旧版と新版が二重に掛からないようにするため）。

If the `Mods` folder holds other mods, delete only the three folders above. While an old
DLL is still there, the loader does not load that mod and writes a message to
`MixedNuts\loader.log` asking you to delete it (so that the old and the new version are
never applied together).

ローダーだけを入れて Mod を 1.x のままにすると、その Mod は動きません（特に Native 120FPS
Option 1.x は、`dinput8.dll` がローダーで上書きされて止まります）。旧版のフォルダが残って
いて新版が無い場合も、`loader.log` に 2.0.0 への更新を促すメッセージを書きます。

Installing only the loader and keeping a mod at 1.x leaves that mod not running (Native
120FPS Option 1.x in particular stops, because the loader's `dinput8.dll` overwrites its
own). When an old folder is still there but the new version is not, `loader.log` says so
and asks you to update to 2.0.0.

### 他の Mod と `dinput8.dll` がぶつかるとき / If another mod also uses `dinput8.dll`

**上書きしないでください。** こちらの `dinput8.dll` の名前を `version.dll` か
`xinput1_4.dll` に変えれば、そのまま動きます（同じファイルが 3 つの名前のどれでも動くように
作ってあります）。

**Do not overwrite it.** Rename this `dinput8.dll` to `version.dll` or `xinput1_4.dll`
instead; the same file works under any of the three names.

## 設定 / Configuration

`MixedNuts\loader.ini`

| 項目 / Key | 意味 / Meaning |
|---|---|
| `Log` | `1` = `MixedNuts\loader.log` を出力 write a log / `0` = 出力しない no log |

各 Mod の設定は、それぞれのフォルダの ini にあります。
Each mod has its own ini in its folder.

## 仕組み / How it works

1. ゲームが起動直後に読み込む `dinput8.dll`（転送用）が、`MixedNuts\MixedNutsLoader.dll` を
   読み込みます。本来の関数は System32 の実体へ転送します
   The `dinput8.dll` the game loads at startup (a forwarder) loads
   `MixedNuts\MixedNutsLoader.dll`. The real functions are forwarded to System32.
2. ローダーは `MixedNuts\Mods\<名前>\<名前>.dll` をフォルダ名の順に読み込みます。
   ゲームのコードが動く前です
   The loader loads `MixedNuts\Mods\<name>\<name>.dll` in folder-name order, before any
   of the game's code runs.
3. Mod は「どのファイルをどう改変するか」を登録します。ゲームがそのファイルを初めて開いた
   とき、ローダーは登録された改変を読み込み順に適用して `MixedNuts\cache` に置き、ゲームには
   そちらを開かせます。後の Mod には、前の Mod が改変した結果が渡ります
   A mod registers which files it changes and how. When the game first opens one of
   them, the loader applies the registered changes in load order, stores the result in
   `MixedNuts\cache`, and has the game open that instead. Each mod receives what the
   previous ones produced.
4. ゲームのファイル、Mod、その設定のどれも変わっていなければ、次回からはキャッシュを
   そのまま使います。どれかが変われば作り直します
   If neither the game files, the mods nor their settings have changed, the cache is
   reused on later launches; otherwise it is rebuilt.

ファイルの差し替えは `CreateFileW` のフックで行います。フックは exe ではなく kernel32 の
輸入テーブル（`kernel32!CreateFileW` の中継先）に掛けます。exe の輸入テーブルは Steam DRM が
起動時に組み直すことがありますが、kernel32 側はその影響を受けません。

Files are redirected by hooking `CreateFileW`, in kernel32's import table (where
`kernel32!CreateFileW` jumps) rather than the exe's. Steam DRM may rebuild the exe's import
table at startup; kernel32's is not affected.

## Mod の作り方 / Writing a mod

Mod は `MixedNutsPluginInit` をエクスポートする DLL です。API は
[`api/mixednuts/plugin.h`](api/mixednuts/plugin.h) にあります（C の呼び出し規約）。

A mod is a DLL that exports `MixedNutsPluginInit`. The API is in
[`api/mixednuts/plugin.h`](api/mixednuts/plugin.h) (plain C ABI).

```cpp
#include <mixednuts/plugin.h>

int Generate(void* ctx, const MixedNutsPatchIo* io, char* note, size_t cap)
{
    const uint8_t* data; size_t size;
    if (!io->Read(io->self, L"archive\\archive_01.lnk", &data, &size)) return 0;
    std::vector<uint8_t> buf(data, data + size);
    /* ... modify buf ... */
    io->Write(io->self, L"archive\\archive_01.lnk", buf.data(), buf.size());
    return 1;
}

MIXEDNUTS_PLUGIN_EXPORT int WINAPI MixedNutsPluginInit(const MixedNutsApi* api)
{
    static const wchar_t* const targets[] = { L"archive\\archive_01.lnk", nullptr };
    const MixedNutsPatch patch{ targets, "example-v1", &Generate, nullptr };
    return api->RegisterPatch(api, &patch);
}
```

- `MixedNutsPluginInit` はゲームの起動処理の途中で呼ばれます。時間のかかる処理（コードの
  探索など）は自分でスレッドを立てて行い、そのスレッドの終了を待たないでください
  `MixedNutsPluginInit` runs in the middle of the game's startup. Do slow work (such as
  scanning code) on a thread of your own, and never wait for that thread there.
- `tag` には改変ロジックの版と、結果に影響する設定を入れます。変わればキャッシュが作り直されます
  Put the version of your logic and every setting that affects the result into `tag`;
  when it changes, the cache is rebuilt.
- 3 本の Mod のソースが実例です
  The three mods above are working examples.

### 共通コード / Shared code

`common/mixednuts/` に、各 Mod で使っているヘッダのみのコードがあります（名前空間 `mixednuts::`）。
`common/mixednuts/` holds the header-only code the mods share (namespace `mixednuts::`).

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

Mod のリポジトリにはこのリポジトリを submodule で取り込み、`common` と `api` を include パスに
加えます。
Mods add this repository as a submodule and put `common` and `api` on the include path.

```
git submodule add https://github.com/MixedNuts-Dev/fatal-frame2-remake-mod-loader.git mod-loader
cl ... /I mod-loader\common /I mod-loader\api ...
```

## ビルド / Build

Visual Studio 2022 の C++ ツールセットが必要です。
Requires the Visual Studio 2022 C++ toolset.

```
loader\build.bat
```

`loader\dist\` に配布用の一式が出力されます。 / The distributable set is written to `loader\dist\`.

## 免責事項 / Disclaimer

**このソフトウェアは無保証で提供されます。使用によって生じたいかなる損害についても、
作者は一切の責任を負いません。** セーブデータの破損・消失、ゲームの動作不良、
その他の不具合を含みます。自己責任でご使用ください。

**This software is provided as-is, without any warranty. The author accepts no liability
for any damage arising from its use,** including but not limited to corruption or loss
of save data, game malfunction, or any other problem. Use it at your own risk.

## 不具合の報告 / Reporting issues

不具合を見つけた場合は、GitHub の Issue でご報告ください。その際、**必ず
`MixedNuts\loader.log` と、該当する Mod のログを添付してください。**

If you run into a problem, please open a GitHub Issue. **Be sure to attach
`MixedNuts\loader.log` and the log of the mod concerned.**

---

## クレジット / Credits

**Created by MixedNuts**

## ライセンス / License

MIT License — 詳細は [LICENSE](LICENSE) を参照してください。
See [LICENSE](LICENSE) for details.

再配布・改変は自由ですが、**著作権表示とライセンス文を必ず残してください。**
MIT ライセンスの条件です。

You are free to redistribute and modify this, but **the copyright notice and the
license text must be retained** — that is a condition of the MIT License.
