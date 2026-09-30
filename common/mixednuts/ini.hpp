// FATAL FRAME II: Crimson Butterfly REMAKE — MixedNuts Mod 共通コード
// Created by MixedNuts - https://github.com/MixedNutsJP/fatal-frame2-remake-mod-loader
// Licensed under the MIT License. See LICENSE for details.
//
// ini の読み込み。項目が無ければ既定値を返す。

#pragma once

#include <windows.h>
#include <string>

namespace mixednuts::ini {

// 10 進のみ（GetPrivateProfileInt と同じ）
inline int Int(const std::wstring& ini, const wchar_t* section, const wchar_t* key, int def)
{
    return static_cast<int>(GetPrivateProfileIntW(section, key, def, ini.c_str()));
}

inline bool Bool(const std::wstring& ini, const wchar_t* section, const wchar_t* key, bool def)
{
    return Int(ini, section, key, def ? 1 : 0) != 0;
}

// 前後の空白と行末のコメント（; か # 以降）を落とす。GetPrivateProfileString は
// 値の後ろのコメントをそのまま返すので、"Main=mio ; 操作キャラ" が "mio" にならない
inline std::wstring String(const std::wstring& ini, const wchar_t* section, const wchar_t* key,
                           const wchar_t* def)
{
    wchar_t buf[256]{};
    GetPrivateProfileStringW(section, key, def, buf, 256, ini.c_str());
    std::wstring s(buf);
    const size_t cut = s.find_first_of(L";#");
    if (cut != std::wstring::npos) s.resize(cut);
    const size_t b = s.find_first_not_of(L" \t");
    if (b == std::wstring::npos) return std::wstring();
    const size_t e = s.find_last_not_of(L" \t");
    return s.substr(b, e - b + 1);
}

} // namespace mixednuts::ini
