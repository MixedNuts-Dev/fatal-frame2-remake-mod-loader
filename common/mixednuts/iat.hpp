// FATAL FRAME II: Crimson Butterfly REMAKE — MixedNuts Mod 共通コード
// Created by MixedNuts - https://github.com/MixedNutsJP/fatal-frame2-remake-mod-loader
// Licensed under the MIT License. See LICENSE for details.
//
// 輸入テーブル（IAT）の枠を探して差し替える。
//
// 枠は 2 種類ある:
//   FindImport   … exe などが dll!func を呼ぶときに参照する枠。そのモジュールからの呼び出しだけに効く
//   FindJumpSlot … kernel32!CreateFileW のような「jmp [rip+X]」だけの中継が飛ぶ先の枠。
//                  kernel32 経由で呼ぶすべてのモジュールに効く
//
// 同じ枠に複数の Mod が掛けると、互いを「元の関数」として保存し合って無限再帰になり
// うる。別の Mod と同じ関数を横取りするときは、違う種類の枠を使う。

#pragma once

#include <windows.h>
#include <cstdint>
#include <cstring>

namespace mixednuts::iat {

// 名前で輸入しているものだけを探す（番号での輸入は対象外）
inline PVOID* FindImport(HMODULE mod, const char* dll, const char* func)
{
    auto base = reinterpret_cast<BYTE*>(mod);
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return nullptr;

    const auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!dir.VirtualAddress) return nullptr;

    auto imp = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + dir.VirtualAddress);
    for (; imp->Name; ++imp)
    {
        if (_stricmp(reinterpret_cast<const char*>(base + imp->Name), dll) != 0) continue;

        auto thunk = reinterpret_cast<IMAGE_THUNK_DATA64*>(base + imp->FirstThunk);
        auto orig = reinterpret_cast<IMAGE_THUNK_DATA64*>(
            base + (imp->OriginalFirstThunk ? imp->OriginalFirstThunk : imp->FirstThunk));
        for (; orig->u1.AddressOfData; ++orig, ++thunk)
        {
            if (orig->u1.Ordinal & IMAGE_ORDINAL_FLAG64) continue;
            auto ibn = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + orig->u1.AddressOfData);
            if (strcmp(reinterpret_cast<const char*>(ibn->Name), func) == 0)
                return reinterpret_cast<PVOID*>(&thunk->u1.Function);
        }
    }
    return nullptr;
}

// proc が「jmp [rip+X]」（rex.w 付きも）だけの中継なら、その飛び先の枠を返す
inline PVOID* FindJumpSlot(const void* proc)
{
    auto p = static_cast<const uint8_t*>(proc);
    if (!p) return nullptr;
    int32_t rel = 0;
    if (p[0] == 0xFF && p[1] == 0x25)
    {
        memcpy(&rel, p + 2, 4);
        return reinterpret_cast<PVOID*>(const_cast<uint8_t*>(p) + 6 + rel);
    }
    if (p[0] == 0x48 && p[1] == 0xFF && p[2] == 0x25)
    {
        memcpy(&rel, p + 3, 4);
        return reinterpret_cast<PVOID*>(const_cast<uint8_t*>(p) + 7 + rel);
    }
    return nullptr;
}

// 枠を hook に差し替える。元の値は、枠を書き換える前に original へ入れる
// （差し替えた直後に別スレッドから hook が呼ばれても、original が使えるように）
template <class Fn>
bool Swap(PVOID* slot, PVOID hook, Fn& original)
{
    if (!slot) return false;
    DWORD old = 0;
    if (!VirtualProtect(slot, sizeof(PVOID), PAGE_READWRITE, &old)) return false;
    original = reinterpret_cast<Fn>(*slot);
    *slot = hook;
    DWORD tmp = 0;
    VirtualProtect(slot, sizeof(PVOID), old, &tmp);
    return true;
}

} // namespace mixednuts::iat
