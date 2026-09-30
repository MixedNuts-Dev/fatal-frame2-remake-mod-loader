/* FATAL FRAME II: Crimson Butterfly REMAKE — MixedNuts Mod Loader プラグイン API
 * Created by MixedNuts - https://github.com/MixedNutsJP/fatal-frame2-remake-mod-loader
 * Licensed under the MIT License. See LICENSE for details.
 *
 * ローダーは MixedNuts\Mods\<名前>\<名前>.dll をフォルダ名の順に読み込み、
 * エクスポートされた MixedNutsPluginInit を呼ぶ。
 *
 * MixedNutsPluginInit はゲームの DllMain の途中（ゲームのコードが動く前）で
 * 呼ばれる。ここでは設定を読んでファイル改変を登録する程度に留め、コードの
 * 探索など時間のかかる処理は自分でスレッドを立てて行う（スレッドの終了を
 * 待ってはいけない。ローダロック中なのでデッドロックする）。
 *
 * ファイル改変:
 *   RegisterPatch で「対象のファイル」と「生成関数」を登録する。ゲームが対象の
 *   どれかを初めて開いたとき、ローダーは登録されたすべての生成関数を読み込み順に
 *   呼び、その結果を MixedNuts\cache に置いて、以後ゲームにはそちらを開かせる。
 *   後に呼ばれる生成関数には、先の生成関数が書いた内容が渡る（同じファイルを
 *   複数の Mod が順に改変できる）。元のファイルか、いずれかの Mod の tag が
 *   変わらない限り、次回からは生成し直さずにキャッシュを使う。
 *
 * パスはどれもゲームのフォルダからの相対パス（例 L"fdata_package\\root.rdb"）。
 * 大文字小文字と / \ は区別しない。
 */

#ifndef MIXEDNUTS_PLUGIN_H
#define MIXEDNUTS_PLUGIN_H

#include <windows.h>
#include <stddef.h>
#include <stdint.h>

#define MIXEDNUTS_API_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

typedef HANDLE(WINAPI* MixedNutsCreateFileW)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES,
                                             DWORD, DWORD, HANDLE);

/* 生成関数に渡す入出力。どの関数も self に io->self を渡す。 */
typedef struct MixedNutsPatchIo {
    void* self;
    /* 現在の内容（先の生成関数の結果を含む）を丸ごと読む。成功で 1。
     * *data は生成関数から戻るまで有効。 */
    int (*Read)(void* self, const wchar_t* name, const uint8_t** data, size_t* size);
    /* 現在の内容が入っているファイルの絶対パス。大きなファイルを部分的に読むとき用。
     * 生成関数から戻るまで有効。開くときは api->CreateFileOriginal を使う。 */
    const wchar_t* (*Path)(void* self, const wchar_t* name);
    /* 新しい内容を書く。成功で 1。ゲームに無いファイル名でもよい（ゲームがその
     * 名前を開くと、書いた内容が渡る）。生成関数が失敗を返すと、書いた内容は
     * すべて捨てられる。 */
    int (*Write)(void* self, const wchar_t* name, const uint8_t* data, size_t size);
} MixedNutsPatchIo;

/* 成功で 1。note には結果の要約（成功時）か理由（失敗時）を英語で書く。 */
typedef int (*MixedNutsGenerateFn)(void* ctx, const MixedNutsPatchIo* io, char* note,
                                   size_t noteCap);

typedef struct MixedNutsPatch {
    /* NULL 終端。ゲームがこれらのどれかを開くと生成が始まる。読むファイルと
     * 書き換えるファイルを並べる（新しく作るだけのファイルは不要）。 */
    const wchar_t* const* targets;
    /* 改変ロジックと、結果に影響する設定の版。変わればキャッシュを作り直す。 */
    const char* tag;
    MixedNutsGenerateFn generate;
    void* ctx;
} MixedNutsPatch;

typedef struct MixedNutsApi {
    uint32_t       size;       /* sizeof(MixedNutsApi) */
    uint32_t       version;    /* MIXEDNUTS_API_VERSION */
    const wchar_t* gameDir;    /* ゲームのフォルダ。末尾に \ */
    const wchar_t* pluginDir;  /* この Mod のフォルダ。末尾に \ */
    /* 差し替えを通らない CreateFileW。Mod 自身のファイル操作に使う。 */
    MixedNutsCreateFileW CreateFileOriginal;
    /* ファイル改変を登録する。内容はコピーされる。MixedNutsPluginInit の中でだけ呼べる。
     * 成功で 1。 */
    int (*RegisterPatch)(const struct MixedNutsApi* self, const MixedNutsPatch* patch);
} MixedNutsApi;

/* プラグインがエクスポートする関数。成功で 1。api は読み込み中ずっと有効。 */
typedef int(WINAPI* MixedNutsPluginInitFn)(const MixedNutsApi* api);
#define MIXEDNUTS_PLUGIN_INIT "MixedNutsPluginInit"

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#define MIXEDNUTS_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define MIXEDNUTS_PLUGIN_EXPORT __declspec(dllexport)
#endif

#endif /* MIXEDNUTS_PLUGIN_H */
