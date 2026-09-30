// FATAL FRAME II: Crimson Butterfly REMAKE — MixedNuts Mod 共通コード
// Created by MixedNuts - https://github.com/MixedNutsJP/fatal-frame2-remake-mod-loader
// Licensed under the MIT License. See LICENSE for details.
//
// プロキシ DLL の転送。ゲームのルートに System32 の DLL と同じ名前で置き、
// 本来の関数は System32 の実体へそのまま渡す。
//
//   DllMain で mixednuts::proxy::Init(L"version.dll");
//   MIXEDNUTS_FORWARD(Proxy_VerQueryValueW, "VerQueryValueW", 0)
//   .def で VerQueryValueW=Proxy_VerQueryValueW
//
// 転送する関数が整数・ポインタの引数を 8 個以下しか取らず、浮動小数点の引数も
// 無いなら、x64 の呼び出し規約では 8 個をそのまま受け渡せば元の関数と同じに
// 振る舞う。型ごとに書き分けずに済む。dinput8 / version / xinput1_4 はどれも
// この条件を満たす。浮動小数点の引数を取る関数には使えない。
// 名前の無い関数（番号だけ）は MAKEINTRESOURCEA(番号) で引く。
//
// エクスポート名は .def で本来の名前に付け替える（windows.h の宣言と衝突しないように）。

#pragma once

#include <windows.h>
#include <cstdint>

namespace mixednuts::proxy {

inline const wchar_t* g_name = nullptr;
inline HMODULE        g_real = nullptr;

inline HMODULE Real()
{
    if (g_real || !g_name) return g_real;
    wchar_t path[MAX_PATH]{};
    GetSystemDirectoryW(path, MAX_PATH);
    wcscat_s(path, L"\\");
    wcscat_s(path, g_name);
    g_real = LoadLibraryW(path);
    return g_real;
}

// DllMain で呼ぶ。実体は最初に転送するときに読み込む
inline void SetTarget(const wchar_t* systemDll) { g_name = systemDll; }

// DllMain で呼ぶ。System32 の実体をすぐ読み込んでおく
inline void Init(const wchar_t* systemDll)
{
    SetTarget(systemDll);
    Real();
}

inline FARPROC Proc(const char* name)
{
    HMODULE m = Real();
    return m ? GetProcAddress(m, name) : nullptr;
}

using Fwd8 = uintptr_t(WINAPI*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t,
                                uintptr_t, uintptr_t, uintptr_t, uintptr_t);

} // namespace mixednuts::proxy

// fallback は実体が見つからないときの戻り値
#define MIXEDNUTS_FORWARD(export_name, lookup, fallback)                                   \
    extern "C" uintptr_t WINAPI export_name(uintptr_t a, uintptr_t b, uintptr_t c,         \
                                            uintptr_t d, uintptr_t e, uintptr_t f,         \
                                            uintptr_t g, uintptr_t h)                      \
    {                                                                                      \
        static ::mixednuts::proxy::Fwd8 fn =                                               \
            reinterpret_cast<::mixednuts::proxy::Fwd8>(::mixednuts::proxy::Proc(lookup));  \
        return fn ? fn(a, b, c, d, e, f, g, h) : static_cast<uintptr_t>(fallback);         \
    }
