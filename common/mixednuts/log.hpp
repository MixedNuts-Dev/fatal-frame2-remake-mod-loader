// FATAL FRAME II: Crimson Butterfly REMAKE — MixedNuts Mod 共通コード
// Created by MixedNuts - https://github.com/MixedNuts-Dev/fatal-frame2-remake-mod-loader
// Licensed under the MIT License. See LICENSE for details.
//
// ログ出力。Mod ごとに自分のフォルダへ自分のファイル名で書く（不具合報告では
// 該当する Mod のログを 1 つ送ってもらえば足りるように、ファイルは混ぜない）。
//
// ログは英語で書く。配布先の利用者は大半が英語話者で、日本語のログでは自分の
// 状況を判断できない（Native 120FPS Option の 1.0.1 で実際に起きた）。
//
// ファイルは UTF-8 で書くので、ワイド文字列は %ls ではなく Utf8() で変換して
// %s で渡す。%ls はロケール依存で、日本語などを含むパスが化ける。

#pragma once

#include <windows.h>
#include <cstdarg>
#include <cstdio>
#include <string>

namespace mixednuts {

inline std::string Utf8(const wchar_t* w)
{
    if (!w || !*w) return std::string();
    const int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    if (n <= 1) return std::string();
    std::string s(static_cast<size_t>(n - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w, -1, &s[0], n, nullptr, nullptr);
    return s;
}

inline std::string Utf8(const std::wstring& w) { return Utf8(w.c_str()); }

namespace log {

inline std::wstring g_path;
inline bool         g_enabled = false;

// dir は末尾に \ を付けて渡す。Open より前の Log は捨てる
inline void Open(const std::wstring& dir, const wchar_t* fileName, bool enabled = true)
{
    g_path    = dir + fileName;
    g_enabled = enabled;
}

} // namespace log

inline void Log(const char* fmt, ...)
{
    if (!log::g_enabled || log::g_path.empty()) return;
    const bool isNew = (GetFileAttributesW(log::g_path.c_str()) == INVALID_FILE_ATTRIBUTES);
    FILE* f = nullptr;
    if (_wfopen_s(&f, log::g_path.c_str(), L"a") != 0 || !f) return;
    if (isNew) fwrite("\xEF\xBB\xBF", 1, 3, f);   // UTF-8 BOM（メモ帳で文字化けしないように）

    SYSTEMTIME st{};
    GetLocalTime(&st);
    fprintf(f, "[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fputc('\n', f);
    fclose(f);
}

} // namespace mixednuts
