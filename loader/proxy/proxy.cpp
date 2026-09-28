// FATAL FRAME II: Crimson Butterfly REMAKE — MixedNuts Mod Loader
// プロキシ（dinput8.dll / version.dll / xinput1_4.dll のどの名前でも動く）
// Created by MixedNuts - https://github.com/MixedNuts-Dev/fatal-frame2-remake-mod-loader
// Licensed under the MIT License. See LICENSE for details.
//
// ゲームのルートに置くと自動的にロードされ、MixedNuts\MixedNutsLoader.dll を読み込む。
// ここでは転送とローダー本体の読み込みだけを行い、機能はすべて本体側に置く
// （プロキシはほとんど更新しないで済むようにする）。
//
// 3 種類の転送を 1 つの DLL に持ち、自分のファイル名で転送先を決める。既定は
// dinput8.dll で、他の Mod と名前がぶつかったら、利用者がこのファイルの名前を
// version.dll か xinput1_4.dll に変えれば共存できる。3 つともゲームが起動直後に
// 読み込む（dinput8 と xinput1_4 は exe が直接、version は間接的に）。
//
// ローダー本体はここ（DllMain の中）で同期的に読み込む。ファイルの差し替えは、
// ゲームがアーカイブを開く前に仕掛けておく必要があるため。

#include <windows.h>
#include <cwchar>
#include <string>

#include <mixednuts/path.hpp>
#include <mixednuts/proxy.hpp>

namespace {

// 転送先として受け付ける名前（System32 の DLL と同じ名前）
const wchar_t* const kNames[] = { L"dinput8.dll", L"version.dll", L"xinput1_4.dll" };

using StartFn = void(WINAPI*)(const wchar_t* proxyPath, const wchar_t* forwardTo);

void Start(HMODULE self)
{
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(self, path, MAX_PATH);
    const wchar_t* base = wcsrchr(path, L'\\');
    base = base ? base + 1 : path;

    const wchar_t* forwardTo = nullptr;
    for (const wchar_t* n : kNames)
        if (_wcsicmp(base, n) == 0) forwardTo = n;
    if (forwardTo) mixednuts::proxy::SetTarget(forwardTo);   // 実体は最初の転送で読み込む

    const std::wstring loader = mixednuts::GameDir() + L"MixedNuts\\MixedNutsLoader.dll";
    HMODULE h = LoadLibraryW(loader.c_str());
    if (!h) return;   // 本体が無い（プロキシだけ残っている）ときは転送だけ行う
    if (auto start = reinterpret_cast<StartFn>(GetProcAddress(h, "MixedNutsLoaderStart")))
        start(path, forwardTo);
}

} // namespace

// ---- 転送用エクスポート -------------------------------------------------
//
// 3 つとも整数・ポインタの引数を 8 個以下しか取らず、浮動小数点の引数も無いので、
// 8 個そのまま受け渡す転送で済ませる（proxy.hpp）。転送先に無い関数（dinput8.dll
// という名前のときの VerQueryValueW など）は fallback を返す。
// 番号は .def で決める。ゲームは xinput1_4 を番号で呼ぶので、その番号は本来の値を保つ。

// dinput8
MIXEDNUTS_FORWARD(Proxy_DirectInput8Create, "DirectInput8Create", E_FAIL)
MIXEDNUTS_FORWARD(Proxy_DllCanUnloadNow, "DllCanUnloadNow", S_FALSE)
MIXEDNUTS_FORWARD(Proxy_DllGetClassObject, "DllGetClassObject", E_FAIL)
MIXEDNUTS_FORWARD(Proxy_DllRegisterServer, "DllRegisterServer", E_FAIL)
MIXEDNUTS_FORWARD(Proxy_DllUnregisterServer, "DllUnregisterServer", E_FAIL)

// version
#define FORWARD_VERSION(name) MIXEDNUTS_FORWARD(Proxy_##name, #name, 0)
FORWARD_VERSION(GetFileVersionInfoA)
FORWARD_VERSION(GetFileVersionInfoByHandle)
FORWARD_VERSION(GetFileVersionInfoExA)
FORWARD_VERSION(GetFileVersionInfoExW)
FORWARD_VERSION(GetFileVersionInfoSizeA)
FORWARD_VERSION(GetFileVersionInfoSizeExA)
FORWARD_VERSION(GetFileVersionInfoSizeExW)
FORWARD_VERSION(GetFileVersionInfoSizeW)
FORWARD_VERSION(GetFileVersionInfoW)
FORWARD_VERSION(VerFindFileA)
FORWARD_VERSION(VerFindFileW)
FORWARD_VERSION(VerInstallFileA)
FORWARD_VERSION(VerInstallFileW)
FORWARD_VERSION(VerLanguageNameA)
FORWARD_VERSION(VerLanguageNameW)
FORWARD_VERSION(VerQueryValueA)
FORWARD_VERSION(VerQueryValueW)

// xinput1_4（番号だけの関数 100〜 も番号から引いて転送する）
#define FORWARD_XINPUT(export_name, lookup) \
    MIXEDNUTS_FORWARD(export_name, lookup, ERROR_DEVICE_NOT_CONNECTED)
FORWARD_XINPUT(Proxy_XInputGetState, "XInputGetState")
FORWARD_XINPUT(Proxy_XInputSetState, "XInputSetState")
FORWARD_XINPUT(Proxy_XInputGetCapabilities, "XInputGetCapabilities")
FORWARD_XINPUT(Proxy_XInputEnable, "XInputEnable")
FORWARD_XINPUT(Proxy_XInputGetBatteryInformation, "XInputGetBatteryInformation")
FORWARD_XINPUT(Proxy_XInputGetKeystroke, "XInputGetKeystroke")
FORWARD_XINPUT(Proxy_XInputGetAudioDeviceIds, "XInputGetAudioDeviceIds")
FORWARD_XINPUT(Proxy_Ordinal100, MAKEINTRESOURCEA(100))
FORWARD_XINPUT(Proxy_Ordinal101, MAKEINTRESOURCEA(101))
FORWARD_XINPUT(Proxy_Ordinal102, MAKEINTRESOURCEA(102))
FORWARD_XINPUT(Proxy_Ordinal103, MAKEINTRESOURCEA(103))
FORWARD_XINPUT(Proxy_Ordinal104, MAKEINTRESOURCEA(104))
FORWARD_XINPUT(Proxy_Ordinal108, MAKEINTRESOURCEA(108))
FORWARD_XINPUT(Proxy_Ordinal109, MAKEINTRESOURCEA(109))

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        Start(hModule);
    }
    return TRUE;
}
