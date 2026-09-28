// FATAL FRAME II: Crimson Butterfly REMAKE — MixedNuts Mod Loader（本体）
// Created by MixedNuts - https://github.com/MixedNuts-Dev/fatal-frame2-remake-mod-loader
// Licensed under the MIT License. See LICENSE for details.
//
// プロキシ（dinput8.dll など）から、ゲームのコードが動く前に呼ばれる。
//
//   1. MixedNuts\Mods\<名前>\<名前>.dll をフォルダ名の順に読み込み、
//      MixedNutsPluginInit を呼ぶ（プラグインはファイル改変を登録する）
//   2. 登録があれば CreateFileW をフックする
//   3. ゲームが対象のファイルを初めて開いたとき、登録された生成関数を読み込み順に
//      呼んで MixedNuts\cache に結果を置き、以後はそちらを開かせる
//
// フックは exe ではなく kernel32 の輸入テーブル（kernel32!CreateFileW が飛ぶ先）に
// 掛ける。exe の輸入テーブルは Steam DRM が起動時に組み直すことがあり、見張って
// 掛け直す必要があった（旧 Native 120FPS Option）。kernel32 側ならその必要が無く、
// kernel32 経由で開くすべてのファイルに効く（旧 TwinSwap で実績あり）。
//
// ログは英語で書く（利用者が自分で状況を判断できるように）。コメントは日本語。

#include <windows.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwctype>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <mixednuts/plugin.h>
#include <mixednuts/file.hpp>
#include <mixednuts/iat.hpp>
#include <mixednuts/ini.hpp>
#include <mixednuts/log.hpp>
#include <mixednuts/path.hpp>

namespace {

using mixednuts::Log;
using mixednuts::Utf8;

constexpr char kVersion[]  = "1.0.0";
constexpr char kCacheTag[] = "mixednuts-cache-v1";   // キャッシュの形式を変えたら上げる

// どれも末尾に区切りを付ける
std::wstring g_gameDir;    // ゲームのフォルダ
std::wstring g_rootDir;    // MixedNuts フォルダ
std::wstring g_cacheDir;   // MixedNuts の cache フォルダ

// ---- 相対パス -----------------------------------------------------------

// 比較用に小文字・\ 区切りへそろえる
std::wstring Normalize(const wchar_t* rel)
{
    std::wstring s(rel ? rel : L"");
    for (auto& c : s)
    {
        if (c == L'/') c = L'\\';
        c = static_cast<wchar_t>(towlower(c));
    }
    while (!s.empty() && s[0] == L'\\') s.erase(0, 1);
    return s;
}

// path の末尾が rel と一致し、その直前がパスの区切りか先頭か
bool MatchesRel(const wchar_t* path, const std::wstring& rel)
{
    if (!mixednuts::EndsWithPath(path, rel.c_str())) return false;
    const size_t lp = wcslen(path), lr = rel.size();
    if (lp == lr) return true;
    const wchar_t c = path[lp - lr - 1];
    return c == L'\\' || c == L'/';
}

// dir（末尾に \）までのフォルダを作る。ゲームのフォルダより上は作らない
bool MakeDirs(const std::wstring& dir)
{
    for (size_t i = g_gameDir.size(); i < dir.size(); ++i)
    {
        if (dir[i] != L'\\') continue;
        const std::wstring d = dir.substr(0, i);
        if (!CreateDirectoryW(d.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
            return false;
    }
    return true;
}

// ---- CreateFileW のフック（本来の関数） ---------------------------------

using PFN_CreateFileW = HANDLE(WINAPI*)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES,
                                        DWORD, DWORD, HANDLE);
PFN_CreateFileW g_origCreateFileW = nullptr;

// 差し替えを通らない CreateFileW。フックを掛ける前は本来の関数そのもの
HANDLE WINAPI OriginalCreateFileW(LPCWSTR name, DWORD access, DWORD share,
                                  LPSECURITY_ATTRIBUTES sa, DWORD disp, DWORD flags,
                                  HANDLE tmpl)
{
    PFN_CreateFileW fn = g_origCreateFileW ? g_origCreateFileW : &CreateFileW;
    return fn(name, access, share, sa, disp, flags, tmpl);
}

// ---- プラグインと登録 ---------------------------------------------------

struct Plugin {
    std::wstring name;
    std::wstring dir;
    HMODULE      module = nullptr;
    MixedNutsApi api{};
};

struct Patch {
    size_t                    plugin = 0;
    std::vector<std::wstring> targets;   // Normalize 済み
    std::string               tag;
    MixedNutsGenerateFn       generate = nullptr;
    void*                     ctx = nullptr;
};

std::deque<Plugin> g_plugins;   // api のアドレスを保つため deque
std::vector<Patch> g_patches;
bool               g_initializing = false;   // RegisterPatch を受け付ける間だけ true

size_t PluginIndex(const MixedNutsApi* api)
{
    for (size_t i = 0; i < g_plugins.size(); ++i)
        if (&g_plugins[i].api == api) return i;
    return SIZE_MAX;
}

int RegisterPatch(const MixedNutsApi* self, const MixedNutsPatch* p)
{
    const size_t idx = PluginIndex(self);
    if (!g_initializing || idx == SIZE_MAX || !p || !p->generate || !p->targets) return 0;
    Patch patch;
    patch.plugin   = idx;
    patch.tag      = p->tag ? p->tag : "";
    patch.generate = p->generate;
    patch.ctx      = p->ctx;
    for (const wchar_t* const* t = p->targets; *t; ++t) patch.targets.push_back(Normalize(*t));
    if (patch.targets.empty()) return 0;
    g_patches.push_back(std::move(patch));
    return 1;
}

// ---- 生成 ---------------------------------------------------------------

// 生成関数に渡す入出力の実体（生成関数 1 回ぶん）
struct IoState {
    const std::map<std::wstring, std::wstring>* current = nullptr;   // 相対 → 現在の実体
    std::map<std::wstring, std::vector<uint8_t>> staged;             // 書かれた内容
    std::map<std::wstring, std::vector<uint8_t>> reads;              // 読んだ内容（寿命の保持）
    std::map<std::wstring, std::wstring>         paths;              // Path の戻り値の保持
};

std::wstring CurrentPath(const IoState& s, const std::wstring& rel)
{
    auto it = s.current->find(rel);
    return it != s.current->end() ? it->second : g_gameDir + rel;
}

int IoRead(void* self, const wchar_t* name, const uint8_t** data, size_t* size)
{
    auto& s = *static_cast<IoState*>(self);
    if (!name || !data || !size) return 0;
    const std::wstring rel = Normalize(name);
    const std::vector<uint8_t>* v = nullptr;
    auto st = s.staged.find(rel);
    if (st != s.staged.end())
    {
        v = &st->second;
    }
    else
    {
        auto rd = s.reads.find(rel);
        if (rd == s.reads.end())
        {
            std::vector<uint8_t> buf;
            if (!mixednuts::file::ReadAll(CurrentPath(s, rel), buf, 1ull << 31,
                                          &OriginalCreateFileW))
                return 0;
            rd = s.reads.emplace(rel, std::move(buf)).first;
        }
        v = &rd->second;
    }
    *data = v->data();
    *size = v->size();
    return 1;
}

const wchar_t* IoPath(void* self, const wchar_t* name)
{
    auto& s = *static_cast<IoState*>(self);
    if (!name) return nullptr;
    const std::wstring rel = Normalize(name);
    if (s.staged.count(rel)) return nullptr;   // 書いたばかりの内容はまだファイルに無い
    return (s.paths[rel] = CurrentPath(s, rel)).c_str();
}

int IoWrite(void* self, const wchar_t* name, const uint8_t* data, size_t size)
{
    auto& s = *static_cast<IoState*>(self);
    if (!name || (!data && size)) return 0;
    s.staged[Normalize(name)].assign(data, data + size);
    return 1;
}

// プラグインの不具合でゲームごと落ちないように、例外は失敗として扱う
int CallGenerate(const Patch& p, const MixedNutsPatchIo* io, char* note, size_t cap)
{
    __try
    {
        return p.generate(p.ctx, io, note, cap);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        sprintf_s(note, cap, "crashed (exception 0x%08lX)", GetExceptionCode());
        return 0;
    }
}

// キャッシュの目印。元のファイルの目印と、読み込み順の Mod 名と tag をつなげる
std::string CacheKey()
{
    std::set<std::wstring> targets;
    for (const auto& p : g_patches) targets.insert(p.targets.begin(), p.targets.end());

    std::string key = std::string(kCacheTag) + "\n";
    for (const auto& t : targets)
        key += "src " + Utf8(t) + " " + mixednuts::file::Stamp(g_gameDir + t) + "\n";
    for (const auto& p : g_patches)
        key += "mod " + Utf8(g_plugins[p.plugin].name) + " " + p.tag + "\n";
    return key;
}

std::map<std::wstring, std::wstring> g_redirect;   // 相対 → キャッシュの実体

// 目印が一致し、出力がそろっていれば使う
bool LoadCache(const std::string& key)
{
    std::vector<uint8_t> raw;
    if (!mixednuts::file::ReadAll(g_cacheDir + L"cache.tag", raw, 1 << 20, &OriginalCreateFileW))
        return false;
    const std::string text(raw.begin(), raw.end());
    if (text.compare(0, key.size(), key) != 0) return false;

    std::map<std::wstring, std::wstring> outs;
    size_t at = key.size();
    while (at < text.size())
    {
        size_t end = text.find('\n', at);
        if (end == std::string::npos) end = text.size();
        const std::string line = text.substr(at, end - at);
        at = end + 1;
        if (line.compare(0, 4, "out ") != 0) return false;
        const std::string rel8 = line.substr(4);
        std::wstring rel(rel8.size(), L'\0');
        rel.resize(static_cast<size_t>(
            MultiByteToWideChar(CP_UTF8, 0, rel8.data(), static_cast<int>(rel8.size()), &rel[0],
                                static_cast<int>(rel.size()))));
        const std::wstring path = g_cacheDir + rel;
        if (!mixednuts::file::Exists(path)) return false;
        outs[rel] = path;
    }
    g_redirect = std::move(outs);
    return true;
}

void Generate()
{
    const DWORD t0 = GetTickCount();
    const std::string key = CacheKey();
    if (LoadCache(key))
    {
        Log("[OK] Using the cached files (%zu)", g_redirect.size());
        return;
    }
    DeleteFileW((g_cacheDir + L"cache.tag").c_str());

    std::map<std::wstring, std::wstring> current;   // 相対 → 現在の実体（キャッシュ）
    bool allOk = true;
    for (const auto& p : g_patches)
    {
        const std::wstring& mod = g_plugins[p.plugin].name;
        IoState s;
        s.current = &current;
        const MixedNutsPatchIo io{ &s, &IoRead, &IoPath, &IoWrite };
        char note[256] = "";
        if (!CallGenerate(p, &io, note, sizeof(note)))
        {
            Log("[NG] %s: %s. Its changes are skipped.", Utf8(mod).c_str(),
                note[0] ? note : "failed");
            allOk = false;
            continue;
        }
        // 書けたものだけを採る（途中で失敗したら、その Mod の残りは捨てる）
        std::map<std::wstring, std::wstring> written;
        bool ok = true;
        for (const auto& [rel, data] : s.staged)
        {
            const std::wstring path = g_cacheDir + rel;
            if (!MakeDirs(path.substr(0, path.find_last_of(L'\\') + 1)) ||
                !mixednuts::file::WriteAll(path, data, &OriginalCreateFileW))
            {
                Log("[NG] %s: cannot write %s", Utf8(mod).c_str(), Utf8(path).c_str());
                ok = false;
                break;
            }
            written[rel] = path;
        }
        if (!ok)
        {
            allOk = false;
            continue;
        }
        for (const auto& [rel, path] : written) current[rel] = path;
        Log("[OK] %s: %s (%zu files)", Utf8(mod).c_str(), note[0] ? note : "done",
            s.staged.size());
    }

    // 失敗があったら目印を残さず、次回の起動で作り直す
    if (allOk)
    {
        std::string tag = key;
        for (const auto& [rel, path] : current) tag += "out " + Utf8(rel) + "\n";
        mixednuts::file::WriteAll(g_cacheDir + L"cache.tag",
                                  std::vector<uint8_t>(tag.begin(), tag.end()),
                                  &OriginalCreateFileW);
    }
    g_redirect = std::move(current);
    Log("[OK] Generated %zu files (took %lu ms)", g_redirect.size(), GetTickCount() - t0);
}

// ---- 差し替え -----------------------------------------------------------

CRITICAL_SECTION          g_lock{};
volatile LONG             g_state = 0;   // 0=未生成 / 1=生成中 / 2=済
DWORD                     g_genThread = 0;
std::vector<std::wstring> g_triggers;    // 生成のきっかけになる相対パス

void EnsureGenerated()
{
    EnterCriticalSection(&g_lock);
    if (g_state == 0)
    {
        g_genThread = GetCurrentThreadId();
        InterlockedExchange(&g_state, 1);
        Generate();
        InterlockedExchange(&g_state, 2);
    }
    LeaveCriticalSection(&g_lock);
}

HANDLE WINAPI MyCreateFileW(LPCWSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa,
                            DWORD disp, DWORD flags, HANDLE tmpl)
{
    // 生成中に同じスレッドから来たもの（生成関数やログの読み書き）はそのまま通す
    if (name && (access & GENERIC_READ) && !(access & GENERIC_WRITE) &&
        !(g_state == 1 && g_genThread == GetCurrentThreadId()))
    {
        if (g_state != 2)
        {
            for (const auto& t : g_triggers)
            {
                if (!MatchesRel(name, t)) continue;
                EnsureGenerated();
                break;
            }
        }
        if (g_state == 2)
        {
            for (const auto& [rel, path] : g_redirect)
            {
                if (!MatchesRel(name, rel)) continue;
                HANDLE h = g_origCreateFileW(path.c_str(), access, share, sa, disp, flags, tmpl);
                if (h != INVALID_HANDLE_VALUE) return h;
                Log("[NG] Cannot open the cached %s; the game uses its own file",
                    Utf8(rel).c_str());
                break;
            }
        }
    }
    return g_origCreateFileW(name, access, share, sa, disp, flags, tmpl);
}

bool InstallHook()
{
    HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    auto stub = reinterpret_cast<const uint8_t*>(GetProcAddress(k32, "CreateFileW"));
    if (!stub) return false;
    PVOID* slot = mixednuts::iat::FindJumpSlot(stub);
    if (!slot)
    {
        Log("[NG] kernel32!CreateFileW has an unexpected form (%02X %02X %02X)",
            stub[0], stub[1], stub[2]);
        return false;
    }
    return mixednuts::iat::Swap(slot, reinterpret_cast<PVOID>(&MyCreateFileW), g_origCreateFileW);
}

// ---- 旧版の検出 ---------------------------------------------------------
//
// 共通ローダー以前の版は、Mod ごとに別の名前のプロキシ（dinput8.dll / version.dll /
// xinput1_4.dll）で動いていた。残っていると旧版も動いて同じ改変が二重に掛かるので、
// その Mod は読み込まずに、削除を促す。旧版のプロキシには "Mods\<名前>\" という
// 文字列（UTF-16）が必ず入っている。

struct Legacy {
    const wchar_t* plugin;
    const wchar_t* marker;
};
const Legacy kLegacy[] = {
    { L"native120fps", L"Mods\\native120fps\\" },
    { L"wheelspeed",   L"Mods\\wheelspeed\\" },
    { L"twinswap",     L"Mods\\twinswap\\" },
};
const wchar_t* const kProxyNames[] = { L"dinput8.dll", L"version.dll", L"xinput1_4.dll" };

bool Contains(const std::vector<uint8_t>& data, const wchar_t* w)
{
    const auto* p = reinterpret_cast<const uint8_t*>(w);
    const size_t n = wcslen(w) * sizeof(wchar_t);
    return std::search(data.begin(), data.end(), p, p + n) != data.end();
}

std::set<std::wstring> FindLegacy(const std::wstring& proxyPath)
{
    std::set<std::wstring> found;
    for (const wchar_t* name : kProxyNames)
    {
        const std::wstring path = g_gameDir + name;
        if (_wcsicmp(path.c_str(), proxyPath.c_str()) == 0) continue;
        std::vector<uint8_t> data;
        if (!mixednuts::file::ReadAll(path, data, 16 << 20)) continue;
        for (const auto& l : kLegacy)
        {
            if (!Contains(data, l.marker)) continue;
            found.insert(l.plugin);
            Log("[!!] An old version of %s is still installed as %s. Delete %s from the game"
                " folder (and the old Mods\\%s folder). The new %s is NOT loaded until then,"
                " to avoid applying it twice.",
                Utf8(l.plugin).c_str(), Utf8(name).c_str(), Utf8(name).c_str(),
                Utf8(l.plugin).c_str(), Utf8(l.plugin).c_str());
        }
    }
    return found;
}

// 旧版のフォルダ（Mods\<名前>\）だけが残っている場合。旧 Native 120FPS Option の
// dinput8.dll はローダーの導入で上書きされるので、1.x のまま共通ローダーだけを入れると、
// その Mod は何のメッセージも無く動かなくなる。気づけるようにログに残す。
void FindLeftovers(const std::set<std::wstring>& legacy)
{
    for (const auto& l : kLegacy)
    {
        if (legacy.count(l.plugin)) continue;   // DLL が残っている方は FindLegacy で報告済み
        const std::wstring oldDir = g_gameDir + L"Mods\\" + l.plugin;
        const DWORD attr = GetFileAttributesW(oldDir.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) continue;

        const std::string name = Utf8(l.plugin);
        const std::wstring newDll =
            g_rootDir + L"Mods\\" + l.plugin + L"\\" + l.plugin + L".dll";
        if (mixednuts::file::Exists(newDll))
            Log("[--] The old folder Mods\\%s (version 1.x) is no longer used and can be deleted.",
                name.c_str());
        else
            Log("[!!] Files of %s 1.x were found in Mods\\%s, but its DLL is gone (it may have been"
                " overwritten when the loader was installed), so %s is NOT running. Install %s"
                " 2.0.0 or later into MixedNuts\\Mods\\%s\\ and delete Mods\\%s.",
                name.c_str(), name.c_str(), name.c_str(), name.c_str(), name.c_str(),
                name.c_str());
    }
}

// ---- プラグインの読み込み -----------------------------------------------

std::vector<std::wstring> ListPluginDirs()
{
    std::vector<std::wstring> names;
    WIN32_FIND_DATAW fd{};
    HANDLE h = FindFirstFileW((g_rootDir + L"Mods\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return names;
    do
    {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        names.push_back(fd.cFileName);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    std::sort(names.begin(), names.end(), [](const std::wstring& a, const std::wstring& b) {
        return _wcsicmp(a.c_str(), b.c_str()) < 0;
    });
    return names;
}

int CallInit(MixedNutsPluginInitFn init, const MixedNutsApi* api, DWORD& code)
{
    __try
    {
        return init(api);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        code = GetExceptionCode();
        return -1;
    }
}

void LoadPlugins(const std::set<std::wstring>& legacy)
{
    for (const auto& name : ListPluginDirs())
    {
        const std::wstring dir = g_rootDir + L"Mods\\" + name + L"\\";
        const std::wstring dll = dir + name + L".dll";
        if (!mixednuts::file::Exists(dll)) continue;   // DLL の無いフォルダは無視する

        std::wstring lower = name;
        for (auto& c : lower) c = static_cast<wchar_t>(towlower(c));
        if (legacy.count(lower))
        {
            Log("[--] %s: skipped (old version still installed, see above)", Utf8(name).c_str());
            continue;
        }

        // Mod 側の DLL が依存物を自分のフォルダから引けるようにする
        SetDllDirectoryW(dir.c_str());
        HMODULE mod = LoadLibraryW(dll.c_str());
        SetDllDirectoryW(nullptr);
        if (!mod)
        {
            Log("[NG] %s: cannot load (error %lu)", Utf8(name).c_str(), GetLastError());
            continue;
        }
        auto init = reinterpret_cast<MixedNutsPluginInitFn>(
            reinterpret_cast<void*>(GetProcAddress(mod, MIXEDNUTS_PLUGIN_INIT)));
        if (!init)
        {
            // 旧形式の DLL などは DllMain で既に動き出している可能性があるので解放しない
            Log("[NG] %s: not a MixedNuts plugin (no %s)", Utf8(name).c_str(),
                MIXEDNUTS_PLUGIN_INIT);
            continue;
        }

        g_plugins.emplace_back();
        Plugin& p = g_plugins.back();
        p.name   = name;
        p.dir    = dir;
        p.module = mod;
        p.api.size               = sizeof(MixedNutsApi);
        p.api.version            = MIXEDNUTS_API_VERSION;
        p.api.gameDir            = g_gameDir.c_str();
        p.api.pluginDir          = p.dir.c_str();
        p.api.CreateFileOriginal = &OriginalCreateFileW;
        p.api.RegisterPatch      = &RegisterPatch;

        const size_t before = g_patches.size();
        DWORD code = 0;
        g_initializing = true;
        const int r = CallInit(init, &p.api, code);
        g_initializing = false;
        if (r == 1)
            Log("[OK] %s: loaded (%zu file patches)", Utf8(name).c_str(),
                g_patches.size() - before);
        else if (r == -1)
            Log("[NG] %s: crashed during init (exception 0x%08lX)", Utf8(name).c_str(), code);
        else
            Log("[NG] %s: init failed (see its own log)", Utf8(name).c_str());
        if (r != 1) g_patches.resize(before);   // 失敗した Mod の登録は使わない
    }
}

void Start(const wchar_t* proxyPath, const wchar_t* forwardTo)
{
    g_gameDir  = mixednuts::GameDir();
    g_rootDir  = g_gameDir + L"MixedNuts\\";
    g_cacheDir = g_rootDir + L"cache\\";

    const std::wstring ini = g_rootDir + L"loader.ini";
    mixednuts::log::Open(g_rootDir, L"loader.log",
                         mixednuts::ini::Bool(ini, L"General", L"Log", true));
    Log("=== MixedNuts Mod Loader %s / Created by MixedNuts ===", kVersion);
    Log("Proxy: %s (forwarding to System32\\%s)", Utf8(proxyPath).c_str(),
        forwardTo ? Utf8(forwardTo).c_str() : "- : unsupported file name");

    const std::set<std::wstring> legacy = FindLegacy(proxyPath ? proxyPath : L"");
    FindLeftovers(legacy);
    LoadPlugins(legacy);

    if (g_patches.empty())
    {
        Log("No file patches registered");
        return;
    }
    std::set<std::wstring> triggers;
    for (const auto& p : g_patches) triggers.insert(p.targets.begin(), p.targets.end());
    g_triggers.assign(triggers.begin(), triggers.end());
    if (InstallHook())
        Log("[OK] File hook installed (%zu patches, %zu trigger files)", g_patches.size(),
            g_triggers.size());
    else
        Log("[NG] Could not install the file hook; file patches are disabled");
}

} // namespace

extern "C" __declspec(dllexport) void WINAPI MixedNutsLoaderStart(const wchar_t* proxyPath,
                                                                 const wchar_t* forwardTo)
{
    static bool started = false;   // プロキシが 2 つ置かれていても 1 回だけ
    if (started) return;
    started = true;
    Start(proxyPath, forwardTo);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        InitializeCriticalSection(&g_lock);
    }
    return TRUE;
}
