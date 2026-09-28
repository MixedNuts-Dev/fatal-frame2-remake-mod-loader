// FATAL FRAME II: Crimson Butterfly REMAKE — MixedNuts Mod 共通コード
// Created by MixedNuts - https://github.com/MixedNuts-Dev/fatal-frame2-remake-mod-loader
// Licensed under the MIT License. See LICENSE for details.
//
// バイト列からの読み書き。未アライメントでも安全（MSVC は 1 命令に落とす）。

#pragma once

#include <cstdint>
#include <cstring>

namespace mixednuts {

template <class T> T Rd(const uint8_t* p) { T v; memcpy(&v, p, sizeof(v)); return v; }
template <class T> void Wr(uint8_t* p, T v) { memcpy(p, &v, sizeof(v)); }

} // namespace mixednuts
