// FATAL FRAME II: Crimson Butterfly REMAKE — MixedNuts Mod 共通コード
// Created by MixedNuts - https://github.com/MixedNutsJP/fatal-frame2-remake-mod-loader
// Licensed under the MIT License. See LICENSE for details.
//
// ファイル入出力。
//
// CreateFileW をフックしている Mod は、自分のファイル操作がフックを通らないよう、
// open に元の関数を渡す（渡さなければ CreateFileW を使う）。

#pragma once

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace mixednuts::file {

using OpenFn = HANDLE(WINAPI*)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD,
                               HANDLE);

inline HANDLE OpenRead(const std::wstring& path, OpenFn open = nullptr)
{
    if (!open) open = &CreateFileW;
    return open(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, nullptr);
}

inline bool ReadAt(HANDLE h, uint64_t off, void* dst, size_t n)
{
    LARGE_INTEGER li{};
    li.QuadPart = static_cast<LONGLONG>(off);
    if (!SetFilePointerEx(h, li, nullptr, FILE_BEGIN)) return false;
    auto p = static_cast<uint8_t*>(dst);
    size_t done = 0;
    while (done < n)
    {
        const size_t left = n - done;
        const DWORD want = static_cast<DWORD>(left > (16u << 20) ? (16u << 20) : left);
        DWORD got = 0;
        if (!ReadFile(h, p + done, want, &got, nullptr) || got == 0) return false;
        done += got;
    }
    return true;
}

// 空のファイルと maxBytes を超えるファイルは失敗にする
inline bool ReadAll(const std::wstring& path, std::vector<uint8_t>& out, uint64_t maxBytes,
                    OpenFn open = nullptr)
{
    HANDLE h = OpenRead(path, open);
    if (h == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER sz{};
    bool ok = GetFileSizeEx(h, &sz) && sz.QuadPart > 0 &&
              static_cast<uint64_t>(sz.QuadPart) <= maxBytes;
    if (ok)
    {
        out.resize(static_cast<size_t>(sz.QuadPart));
        ok = ReadAt(h, 0, out.data(), out.size());
    }
    CloseHandle(h);
    return ok;
}

// いったん path.tmp に書いてから置き換える。途中で失敗しても元のファイルは残る
inline bool WriteAll(const std::wstring& path, const std::vector<uint8_t>& data,
                     OpenFn open = nullptr)
{
    if (!open) open = &CreateFileW;
    const std::wstring tmp = path + L".tmp";
    HANDLE h = open(tmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    size_t done = 0;
    while (done < data.size())
    {
        const size_t left = data.size() - done;
        const DWORD want = static_cast<DWORD>(left > (16u << 20) ? (16u << 20) : left);
        DWORD put = 0;
        if (!WriteFile(h, data.data() + done, want, &put, nullptr) || put == 0) break;
        done += put;
    }
    CloseHandle(h);
    if (done != data.size() ||
        !MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING))
    {
        DeleteFileW(tmp.c_str());
        return false;
    }
    return true;
}

inline bool Exists(const std::wstring& path)
{
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

inline bool Size(const std::wstring& path, LONGLONG& size)
{
    WIN32_FILE_ATTRIBUTE_DATA fa{};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fa)) return false;
    size = (static_cast<LONGLONG>(fa.nFileSizeHigh) << 32) | fa.nFileSizeLow;
    return true;
}

// サイズと更新日時をつないだ目印。元のファイルが入れ替わったかの判定に使う
inline std::string Stamp(const std::wstring& path)
{
    WIN32_FILE_ATTRIBUTE_DATA fa{};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fa)) return "missing";
    char buf[64];
    sprintf_s(buf, "%lu:%lu:%lu:%lu", fa.nFileSizeHigh, fa.nFileSizeLow,
              fa.ftLastWriteTime.dwHighDateTime, fa.ftLastWriteTime.dwLowDateTime);
    return buf;
}

// 小さなテキストファイルを先頭から maxBytes まで読む。読めなければ空。
// 他のプロセス（Steam やゲーム）が書き込み中でも読めるように開く
inline std::string ReadText(const std::wstring& path, size_t maxBytes)
{
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return std::string();
    LARGE_INTEGER sz{};
    if (!GetFileSizeEx(h, &sz) || sz.QuadPart <= 0)
    {
        CloseHandle(h);
        return std::string();
    }
    const size_t n = (static_cast<unsigned long long>(sz.QuadPart) < maxBytes)
                     ? static_cast<size_t>(sz.QuadPart) : maxBytes;
    std::string s(n, '\0');
    DWORD got = 0;
    const BOOL ok = ReadFile(h, &s[0], static_cast<DWORD>(n), &got, nullptr);
    CloseHandle(h);
    if (!ok) return std::string();
    s.resize(got);
    return s;
}

} // namespace mixednuts::file
