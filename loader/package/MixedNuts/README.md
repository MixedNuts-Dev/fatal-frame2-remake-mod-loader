# MixedNuts Mod Loader

**FATAL FRAME II: Crimson Butterfly REMAKE** 用の、MixedNuts の Mod に共通のローダーです。
The common mod loader for the MixedNuts mods for FATAL FRAME / PROJECT ZERO II:
Crimson Butterfly REMAKE.

Created by MixedNuts

---

# 日本語

## これは何か

MixedNuts の Mod（Native 120FPS Option / Mouse Wheel Camera Speed / TwinSwap の
2.0.0 以降）を動かすためのローダーです。**これだけ入れても、ゲームは何も変わりません。**
Mod は `MixedNuts\Mods\` の中に入れます。

## 動作環境

- FATAL FRAME II: Crimson Butterfly REMAKE（Steam 版）

ゲームのファイルは一切変更しないため、Steam のファイル整合性チェックに
引っかかることはありません。

## 同梱ファイル

| ファイル | 役割 |
|---|---|
| `dinput8.dll` | ゲームに読み込ませるための入口 |
| `MixedNuts\MixedNutsLoader.dll` | ローダー本体 |
| `MixedNuts\loader.ini` | 設定ファイル |
| `MixedNuts\README.md` | このファイル |

このうち **`dinput8.dll` と `MixedNuts` フォルダの 2 つ**をコピーします。
起動すると `MixedNuts\` の中に、ログ `loader.log` と、Mod が改変したデータを置く
`cache` フォルダが作られます。

## 導入方法

1. ゲームを終了します

2. 同梱の `dinput8.dll` と `MixedNuts` フォルダを、ゲームのルートディレクトリ
   （`FatalFrameII.exe` と同じ場所）にそのままコピーします

   ```
   ...\steamapps\common\FatalFrameII\FatalFrameII.exe
   ...\steamapps\common\FatalFrameII\dinput8.dll        ← 追加
   ...\steamapps\common\FatalFrameII\MixedNuts\         ← 追加
   ```

   ゲームフォルダの開き方：Steam ライブラリでタイトルを右クリック →
   **管理** → **ローカルファイルを閲覧**

3. 使いたい Mod を、それぞれの説明に従って `MixedNuts\Mods\` の中に入れます

### 旧版（1.x）からの移行

各 Mod の 1.x を入れている場合は、ゲームのルートから次を**削除してから**入れてください。

| 旧版 | 削除するもの |
|---|---|
| Native 120FPS Option 1.x | `dinput8.dll`、`Mods\native120fps\` |
| Mouse Wheel Camera Speed 1.x | `version.dll`、`Mods\wheelspeed\` |
| TwinSwap 1.x | `xinput1_4.dll`、`Mods\twinswap\` |

`Mods` フォルダに他の Mod が入っている場合は、上の 3 つのフォルダだけを消してください。
旧版の DLL が残っていると、ローダーはその Mod を読み込まず、`loader.log` に削除を
促すメッセージを書きます（旧版と新版が二重に掛からないようにするため）。

**Mod も 2.0.0 に更新してください。** ローダーだけを入れて Mod を 1.x のままにすると、
その Mod は動きません（特に Native 120FPS Option 1.x は、`dinput8.dll` がローダーで
上書きされて止まります）。この場合も `loader.log` に更新を促すメッセージを書きます。

### 他の Mod と `dinput8.dll` がぶつかるとき

ゲームのルートに **別の `dinput8.dll` が既にある場合は、上書きしないでください。**
代わりに、こちらの `dinput8.dll` の名前を **`version.dll` か `xinput1_4.dll`** に
変えてから置いてください。同じファイルが 3 つの名前のどれでも動きます。

## 削除方法

`dinput8.dll`（名前を変えた場合はそのファイル）と `MixedNuts` フォルダを削除するだけです。
ゲームのファイルは一切変更していないため、完全に元に戻ります。

## 設定ファイル

`loader.ini` で次の項目を変更できます。変更はゲームの再起動後に反映されます。

| 項目 | 意味 |
|---|---|
| `Log` | `1` = `loader.log` を出力 / `0` = 出力しない |

各 Mod の設定は、それぞれのフォルダの ini にあります。

## 注意事項

- `MixedNuts\cache` は、初回の起動時と、ゲームのファイル・Mod・その設定のどれかが
  変わった後の起動時に作り直されます。それ以外の起動では、作ったものをそのまま使います。
  消しても、次の起動で作り直されます
- Mod はフォルダ名の順（アルファベット順）に読み込まれ、同じファイルを改変する Mod は
  その順に改変を重ねます
- セーブデータには何も書き込みません
- ウイルス対策ソフトが誤検知することがあります。ゲームがファイルを開く処理に
  割り込む仕組みのためです。このローダーはネットワーク通信を一切行わず、
  ファイルを書き込むのも `MixedNuts` フォルダの中だけです

## 免責事項

**このソフトウェアは無保証で提供されます。使用によって生じたいかなる損害についても、
作者は一切の責任を負いません。** セーブデータの破損・消失、ゲームの動作不良、
その他の不具合を含みます。自己責任でご使用ください。

**導入前に、必ずセーブデータのバックアップを取ってください。**

```
%LOCALAPPDATA%\KoeiTecmo\FatalFrameII\Savedata\
```

## うまく動かないとき

1. `dinput8.dll` がゲームのルート（`FatalFrameII.exe` と同じ場所）にあるか。
   **`MixedNuts` フォルダの中ではありません**
2. `MixedNuts\MixedNutsLoader.dll` があるか
3. `MixedNuts\loader.log` が生成されているか。
   生成されていなければ `dinput8.dll` が読み込まれていません

ログには、読み込んだ Mod が 1 つずつ記録されます（ログは英語で出力されます）。

```
[OK] native120fps: loaded (2 file patches)
[OK] twinswap: loaded (1 file patches)
[OK] File hook installed (...)
```

`[NG]` や `[!!]` の行があれば、そこに理由が書かれています。
不具合を報告するときは、GitHub の Issue で `loader.log` と該当する Mod のログを
添付してください。

https://github.com/MixedNuts-Dev/fatal-frame2-remake-mod-loader/issues

---

# English

## What this does

This is the loader the MixedNuts mods (Native 120FPS Option / Mouse Wheel Camera Speed /
TwinSwap, 2.0.0 and later) run on. **On its own it changes nothing in the game.**
Mods go inside `MixedNuts\Mods\`.

## Requirements

- FATAL FRAME II: Crimson Butterfly REMAKE (Steam)

No game files are modified, so this will not trip Steam's file integrity verification.

## What's included

| File | Role |
|---|---|
| `dinput8.dll` | the entry point the game loads |
| `MixedNuts\MixedNutsLoader.dll` | the loader itself |
| `MixedNuts\loader.ini` | configuration |
| `MixedNuts\README.md` | this file |

You copy two things: **`dinput8.dll` and the `MixedNuts` folder.**
When the game runs, a log (`loader.log`) and a `cache` folder holding the data the mods
modify are created in `MixedNuts\`.

## Installation

1. Close the game.

2. Copy `dinput8.dll` and the `MixedNuts` folder into the game's root directory
   (the folder containing `FatalFrameII.exe`).

   ```
   ...\steamapps\common\FatalFrameII\FatalFrameII.exe
   ...\steamapps\common\FatalFrameII\dinput8.dll        <- added
   ...\steamapps\common\FatalFrameII\MixedNuts\         <- added
   ```

   To open the game folder: right-click the title in your Steam library →
   **Manage** → **Browse local files**

3. Put the mods you want into `MixedNuts\Mods\`, following their own instructions.

### Upgrading from the 1.x mods

If you have the 1.x versions installed, **delete the following** from the game's root
first.

| Old version | Delete |
|---|---|
| Native 120FPS Option 1.x | `dinput8.dll`, `Mods\native120fps\` |
| Mouse Wheel Camera Speed 1.x | `version.dll`, `Mods\wheelspeed\` |
| TwinSwap 1.x | `xinput1_4.dll`, `Mods\twinswap\` |

If the `Mods` folder holds other mods, delete only the three folders above. While an old
DLL is still there, the loader does not load that mod and writes a message to
`loader.log` asking you to delete it (so that the old and the new version are never
applied together).

**Update the mods to 2.0.0 as well.** Installing only the loader and keeping a mod at 1.x
leaves that mod not running (Native 120FPS Option 1.x in particular stops, because the
loader's `dinput8.dll` overwrites its own). `loader.log` asks you to update in that case
too.

### If another mod also uses `dinput8.dll`

If the game folder **already contains a different `dinput8.dll`, do not overwrite it.**
Rename this `dinput8.dll` to **`version.dll` or `xinput1_4.dll`** before copying it
instead. The same file works under any of the three names.

## Uninstallation

Delete `dinput8.dll` (or whatever you renamed it to) and the `MixedNuts` folder. No game
files are modified, so removal restores the original state completely.

## Configuration

`loader.ini` exposes the following. Changes take effect after restarting the game.

| Key | Meaning |
|---|---|
| `Log` | `1` = write `loader.log` / `0` = no log |

Each mod has its own ini in its folder.

## Notes

- `MixedNuts\cache` is built on the first launch, and again after the game files, the
  mods or their settings change. Other launches reuse it. If you delete it, it is
  rebuilt on the next launch
- Mods are loaded in folder-name (alphabetical) order, and mods that modify the same file
  apply their changes in that order
- Nothing is written to your save data
- Antivirus software may flag the loader because it intercepts the game opening its
  files. It performs no network activity, and the only files it writes are inside the
  `MixedNuts` folder

## Disclaimer

**This software is provided as-is, without any warranty. The author accepts no liability
for any damage arising from its use,** including but not limited to corruption or
loss of save data, game malfunction, or any other problem. Use it at your own risk.

**Always back up your save data before installing.**

```
%LOCALAPPDATA%\KoeiTecmo\FatalFrameII\Savedata\
```

## If it doesn't work

1. Is `dinput8.dll` in the game's root folder (next to `FatalFrameII.exe`)?
   **It does not go inside the `MixedNuts` folder**
2. Is `MixedNuts\MixedNutsLoader.dll` present?
3. Has `MixedNuts\loader.log` been created?
   If not, `dinput8.dll` is not being loaded

The log lists every mod it loads:

```
[OK] native120fps: loaded (2 file patches)
[OK] twinswap: loaded (1 file patches)
[OK] File hook installed (...)
```

Lines starting with `[NG]` or `[!!]` explain what went wrong.
When reporting a problem, please open a GitHub Issue and attach `loader.log` and the log
of the mod concerned.

https://github.com/MixedNuts-Dev/fatal-frame2-remake-mod-loader/issues

---

## License

MIT License — Copyright (c) 2026 MixedNuts

本ソフトウェアは MIT ライセンスで提供されます。再配布・改変は自由ですが、
著作権表示とライセンス文を必ず残してください。

This software is provided under the MIT License. You are free to redistribute and
modify it, but the copyright notice and the license text must be retained.

https://github.com/MixedNuts-Dev/fatal-frame2-remake-mod-loader
