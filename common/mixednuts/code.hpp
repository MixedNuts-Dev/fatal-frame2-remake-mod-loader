// FATAL FRAME II: Crimson Butterfly REMAKE — MixedNuts Mod 共通コード
// Created by MixedNuts - https://github.com/MixedNutsJP/fatal-frame2-remake-mod-loader
// Licensed under the MIT License. See LICENSE for details.
//
// ゲームのコードの探索と書き換え。
//
// ゲーム本体は Steam DRM により .text が暗号化されているため、ファイルへの静的
// パッチはできない。復号後のメモリを探して実行時に書き換える。復号前に探すと
// 何も見つからないので、呼び出し側で見つかるまで（有限回）繰り返す。

#pragma once

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "bytes.hpp"

namespace mixednuts::code {

// 実行ファイルの .text 範囲
inline bool TextSection(uint8_t*& base, size_t& size)
{
    auto mod = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
    if (!mod) return false;
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(mod);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(mod + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return false;

    auto sec = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++sec)
    {
        if (memcmp(sec->Name, ".text", 5) == 0)
        {
            base = mod + sec->VirtualAddress;
            size = sec->Misc.VirtualSize;
            return true;
        }
    }
    return false;
}

// exe 先頭からの相対位置。ログに出すためだけに使う
inline unsigned long long Rva(const void* p)
{
    return static_cast<unsigned long long>(
        static_cast<const uint8_t*>(p) - reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr)));
}

// mask 0 = ワイルドカード
struct Pattern {
    const uint8_t* bytes;
    const uint8_t* mask;
    size_t         len;
};

inline bool Match(const uint8_t* p, const Pattern& pat)
{
    for (size_t i = 0; i < pat.len; ++i)
        if (pat.mask[i] && p[i] != pat.bytes[i]) return false;
    return true;
}

// 出現を数え、最初の位置を返す。先頭バイトは必ず固定にしておく（それで振り落とす）
inline int FindAll(const uint8_t* base, size_t size, const Pattern& pat, const uint8_t*& first)
{
    int n = 0;
    first = nullptr;
    const uint8_t head = pat.bytes[0];
    for (size_t i = 0; i + pat.len <= size; ++i)
    {
        if (base[i] != head || !Match(base + i, pat)) continue;
        if (n++ == 0) first = base + i;
    }
    return n;
}

// コードを書き換える
inline bool Write(void* addr, const void* data, size_t len)
{
    DWORD old = 0;
    if (!VirtualProtect(addr, len, PAGE_EXECUTE_READWRITE, &old)) return false;
    memcpy(addr, data, len);
    DWORD tmp = 0;
    VirtualProtect(addr, len, old, &tmp);
    FlushInstructionCache(GetCurrentProcess(), addr, len);
    return true;
}

// バイト列を "AA BB CC " 形式にする
inline std::string Hex(const uint8_t* p, size_t n)
{
    std::string s;
    s.reserve(n * 3);
    char buf[4]{};
    for (size_t i = 0; i < n; ++i)
    {
        sprintf_s(buf, "%02X ", p[i]);
        s += buf;
    }
    return s;
}

} // namespace mixednuts::code
