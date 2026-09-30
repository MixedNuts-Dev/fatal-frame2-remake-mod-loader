// FATAL FRAME II: Crimson Butterfly REMAKE — MixedNuts Mod 共通コード
// Created by MixedNuts - https://github.com/MixedNutsJP/fatal-frame2-remake-mod-loader
// Licensed under the MIT License. See LICENSE for details.
//
// パスの解決と比較。フォルダはどれも末尾に \ を付けて返す。

#pragma once

#include <windows.h>
#include <cwchar>
#include <cwctype>
#include <string>

namespace mixednuts {

// mod が置かれているフォルダ。nullptr ならゲームの exe のフォルダ
inline std::wstring ModuleDir(HMODULE mod)
{
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(mod, path, MAX_PATH);
    std::wstring dir(path);
    dir.resize(dir.find_last_of(L'\\') + 1);
    return dir;
}

inline std::wstring GameDir() { return ModuleDir(nullptr); }

// パスの末尾が suffix と一致するか。大文字小文字と / \ の違いは区別しない
inline bool EndsWithPath(const wchar_t* s, const wchar_t* suffix)
{
    if (!s) return false;
    const size_t ls = wcslen(s), lt = wcslen(suffix);
    if (ls < lt) return false;
    for (size_t i = 0; i < lt; ++i)
    {
        wchar_t a = s[ls - lt + i], b = suffix[i];
        if (a == L'/') a = L'\\';
        if (b == L'/') b = L'\\';
        if (towlower(a) != towlower(b)) return false;
    }
    return true;
}

} // namespace mixednuts
