// FATAL FRAME II: Crimson Butterfly REMAKE — MixedNuts Mod 共通コード
// Created by MixedNuts - https://github.com/MixedNutsJP/fatal-frame2-remake-mod-loader
// Licensed under the MIT License. See LICENSE for details.
//
// 環境情報のログ。不具合報告のログだけで、報告者の環境がこちらの検証環境と
// 同じかどうかを判断できるようにする。ゲームのバージョン・Steam のビルド番号・
// 言語・ディスプレイ・OS まで出す。

#pragma once

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>

#include "code.hpp"
#include "file.hpp"
#include "log.hpp"

#pragma comment(lib, "version.lib")
#pragma comment(lib, "user32.lib")

namespace mixednuts::env {

// Steam の acf は `"key"  "value"` 形式
inline std::string AcfValue(const std::string& s, const char* key)
{
    const std::string k = std::string("\"") + key + "\"";
    size_t at = s.find(k);
    if (at == std::string::npos) return std::string();
    at = s.find('"', at + k.size());
    if (at == std::string::npos) return std::string();
    const size_t end = s.find('"', at + 1);
    if (end == std::string::npos) return std::string();
    return s.substr(at + 1, end - at - 1);
}

inline void LogExe(const wchar_t* exe)
{
    Log("exe: %s", Utf8(exe).c_str());

    WIN32_FILE_ATTRIBUTE_DATA fad{};
    if (GetFileAttributesExW(exe, GetFileExInfoStandard, &fad))
    {
        const unsigned long long bytes =
            (static_cast<unsigned long long>(fad.nFileSizeHigh) << 32) | fad.nFileSizeLow;
        FILETIME   lt{};
        SYSTEMTIME st{};
        FileTimeToLocalFileTime(&fad.ftLastWriteTime, &lt);
        FileTimeToSystemTime(&lt, &st);
        Log("exe size: %llu bytes / modified: %04d-%02d-%02d %02d:%02d",
            bytes, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
    }
}

// exe のバージョンリソースから FileVersion / ProductVersion を読む
inline void LogExeVersion(const wchar_t* exe)
{
    DWORD dummy = 0;
    const DWORD n = GetFileVersionInfoSizeW(exe, &dummy);
    if (n == 0)
    {
        Log("Game version: (no version resource)");
        return;
    }
    std::vector<uint8_t> buf(n);
    if (!GetFileVersionInfoW(exe, 0, n, buf.data()))
    {
        Log("Game version: (unreadable)");
        return;
    }
    VS_FIXEDFILEINFO* ffi = nullptr;
    UINT len = 0;
    if (!VerQueryValueW(buf.data(), L"\\", reinterpret_cast<LPVOID*>(&ffi), &len) || !ffi)
    {
        Log("Game version: (unreadable)");
        return;
    }
    Log("Game version: file %u.%u.%u.%u / product %u.%u.%u.%u",
        HIWORD(ffi->dwFileVersionMS), LOWORD(ffi->dwFileVersionMS),
        HIWORD(ffi->dwFileVersionLS), LOWORD(ffi->dwFileVersionLS),
        HIWORD(ffi->dwProductVersionMS), LOWORD(ffi->dwProductVersionMS),
        HIWORD(ffi->dwProductVersionLS), LOWORD(ffi->dwProductVersionLS));
}

// Steam のインストール情報。exe から 2 階層上（steamapps）にある
//   steamapps\common\FatalFrameII\FatalFrameII.exe
//   steamapps\appmanifest_3920610.acf
// buildid はゲームのビルドを一意に示すので、バージョン違いの判定に一番効く。
inline void LogSteamManifest(const wchar_t* exe)
{
    std::wstring dir(exe);
    for (int up = 0; up < 3; ++up)   // exe 名 / FatalFrameII / common
    {
        const size_t slash = dir.find_last_of(L'\\');
        if (slash == std::wstring::npos) return;
        dir.resize(slash);
    }
    const std::wstring acf = dir + L"\\appmanifest_3920610.acf";
    const std::string s = file::ReadText(acf, 64 * 1024);
    if (s.empty())
    {
        Log("Steam manifest: not found (%s)", Utf8(acf).c_str());
        return;
    }
    const std::string build = AcfValue(s, "buildid");
    const std::string lang  = AcfValue(s, "language");
    Log("Steam build: %s / install language: %s",
        build.empty() ? "?" : build.c_str(), lang.empty() ? "?" : lang.c_str());
}

inline void LogTextSection()
{
    uint8_t* base = nullptr;
    size_t   size = 0;
    if (code::TextSection(base, size))
        Log("module: 0x%p / .text: 0x%p (%llu bytes)",
            GetModuleHandleW(nullptr), base, static_cast<unsigned long long>(size));
    else
        Log(".text: could not be located");
}

// 実際のリフレッシュレートとアダプタ
inline void LogDisplay()
{
    DEVMODEW dm{};
    dm.dmSize = sizeof(dm);
    if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &dm))
        Log("Display: %ux%u @ %u Hz (%u bpp)",
            dm.dmPelsWidth, dm.dmPelsHeight, dm.dmDisplayFrequency, dm.dmBitsPerPel);
    else
        Log("Display: unknown");

    DISPLAY_DEVICEW dd{};
    dd.cb = sizeof(dd);
    if (EnumDisplayDevicesW(nullptr, 0, &dd, 0))
        Log("Adapter: %s", Utf8(dd.DeviceString).c_str());
}

// RTL_OSVERSIONINFOW と同じ並び。winternl.h を持ち込まずに使う
struct OsVerInfo {
    ULONG dwOSVersionInfoSize;
    ULONG dwMajorVersion;
    ULONG dwMinorVersion;
    ULONG dwBuildNumber;
    ULONG dwPlatformId;
    WCHAR szCSDVersion[128];
};

inline void LogSystem()
{
    // GetVersionEx は互換シムで嘘をつくので ntdll を直接呼ぶ
    OsVerInfo vi{};
    vi.dwOSVersionInfoSize = sizeof(vi);
    using Fn = LONG(WINAPI*)(OsVerInfo*);
    if (HMODULE nt = GetModuleHandleW(L"ntdll.dll"))
    {
        if (auto fn = reinterpret_cast<Fn>(
                reinterpret_cast<void*>(GetProcAddress(nt, "RtlGetVersion"))))
        {
            if (fn(&vi) == 0)
                Log("OS: Windows %u.%u build %u",
                    vi.dwMajorVersion, vi.dwMinorVersion, vi.dwBuildNumber);
        }
    }

    wchar_t loc[LOCALE_NAME_MAX_LENGTH]{};
    if (GetUserDefaultLocaleName(loc, LOCALE_NAME_MAX_LENGTH) > 0)
        Log("Locale: %s / UI language: 0x%04X",
            Utf8(loc).c_str(), GetUserDefaultUILanguage());
}

// 上をすべて出す
inline void LogAll()
{
    wchar_t exe[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    LogExe(exe);
    LogExeVersion(exe);
    LogSteamManifest(exe);
    LogTextSection();
    LogDisplay();
    LogSystem();
}

} // namespace mixednuts::env
