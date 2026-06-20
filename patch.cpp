#include <windows.h>
#include <cstring>
#include <cstdint>

static const uint32_t RVA_TWOOPTIONS = 0x68F120;
static const uint32_t RVA_NEWCLIENT  = 0x648DE0;
static const uint32_t RVA_COOLDOWN   = 0x58AF60;
static const uint32_t RVA_VACBANNED  = 0x640370;
static const uint32_t RVA_SETBOOL    = 0x585970;

static int g_cntInsec = 0;

static void patch_legacy(uintptr_t base) {
    uint8_t* p = (uint8_t*)(base + RVA_TWOOPTIONS);
    if (!(p[0] == 0x55 && p[1] == 0x8B && p[2] == 0xEC)) return;
    DWORD op; VirtualProtect(p, 1, PAGE_EXECUTE_READWRITE, &op);
    p[0] = 0xC3;
    VirtualProtect(p, 1, op, &op);
    FlushInstructionCache(GetCurrentProcess(), p, 1);
}

extern "C" int __cdecl decide_insecure() { return (++g_cntInsec == 1) ? 1 : 0; }

static uintptr_t scan(uintptr_t base, uint32_t size, const uint8_t* pat, int len) {
    for (uintptr_t a = base; a + len <= base + size; ++a) {
        int j = 0; for (; j < len; ++j) if (*(uint8_t*)(a + j) != pat[j]) break;
        if (j == len) return a;
    }
    return 0;
}
static void patch_insecure(uintptr_t base, uint32_t size) {
    static const uint8_t sig[] = {
        0x8D, 0x4E, 0x05, 0x83, 0xC0, 0x05,
        0x6A, 0x00, 0x6A, 0x00, 0x6A, 0x00, 0x6A, 0x00, 0x6A, 0x00, 0x6A, 0x00,
        0x6A, 0x01, 0x51, 0x50, 0xE9
    };
    uintptr_t m = scan(base, size, sig, sizeof(sig));
    if (!m) return;
    uintptr_t jmpSite  = m + 0x16;
    int32_t   rel      = *(int32_t*)(jmpSite + 1);
    uintptr_t showTail = jmpSite + 5 + rel;
    static const uint8_t epi[] = { 0x5F, 0x5E, 0x5B, 0x8B, 0xE5, 0x5D };
    uintptr_t epilogue = 0;
    for (uintptr_t a = showTail; a < showTail + 0x40; ++a) if (memcmp((void*)a, epi, 6) == 0) { epilogue = a; break; }
    if (!epilogue) return;

    uint8_t* st = (uint8_t*)VirtualAlloc(nullptr, 32, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!st) return;
    int o = 0;
    st[o++] = 0xE8; *(int32_t*)(st + o) = (int32_t)((uintptr_t)&decide_insecure - ((uintptr_t)st + o + 4)); o += 4;
    st[o++] = 0x85; st[o++] = 0xC0;
    st[o++] = 0x74; st[o++] = 0x08;
    st[o++] = 0x83; st[o++] = 0xC4; st[o++] = 0x24;
    st[o++] = 0xE9; *(int32_t*)(st + o) = (int32_t)(epilogue - ((uintptr_t)st + o + 4)); o += 4;
    st[o++] = 0xE9; *(int32_t*)(st + o) = (int32_t)(showTail - ((uintptr_t)st + o + 4)); o += 4;

    DWORD op; VirtualProtect((void*)jmpSite, 5, PAGE_EXECUTE_READWRITE, &op);
    *(int32_t*)(jmpSite + 1) = (int32_t)((uintptr_t)st - (jmpSite + 5));
    VirtualProtect((void*)jmpSite, 5, op, &op);
    FlushInstructionCache(GetCurrentProcess(), (void*)jmpSite, 5);
}

static void hook_banner(uintptr_t base) {
    uintptr_t setBool = base + RVA_SETBOOL;
    uint8_t* st = (uint8_t*)VirtualAlloc(nullptr, 16, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!st) return;
    int o = 0;
    st[o++] = 0x6A; st[o++] = 0x00;
    st[o++] = 0xFF; st[o++] = 0x74; st[o++] = 0x24; st[o++] = 0x08;
    st[o++] = 0xE8; *(int32_t*)(st + o) = (int32_t)(setBool - ((uintptr_t)st + o + 4)); o += 4;
    st[o++] = 0xC3;

    const uint32_t rva[3] = { RVA_NEWCLIENT, RVA_COOLDOWN, RVA_VACBANNED };
    for (int i = 0; i < 3; ++i) {
        uint8_t* p = (uint8_t*)(base + rva[i]);
        if (!(p[0] == 0x55 && p[1] == 0x8B && p[2] == 0xEC)) continue;
        DWORD op; VirtualProtect(p, 5, PAGE_EXECUTE_READWRITE, &op);
        p[0] = 0xE9; *(int32_t*)(p + 1) = (int32_t)((uintptr_t)st - ((uintptr_t)p + 5));
        VirtualProtect(p, 5, op, &op);
        FlushInstructionCache(GetCurrentProcess(), p, 5);
    }
}

static uint32_t image_size(uintptr_t base) {
    auto dos = (IMAGE_DOS_HEADER*)base;
    auto nt  = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    return nt->OptionalHeader.SizeOfImage;
}

static DWORD WINAPI worker(LPVOID) {
    HMODULE cl = nullptr;
    for (int i = 0; i < 12000 && !cl; ++i) { cl = GetModuleHandleA("client.dll"); if (!cl) Sleep(5); }
    if (!cl) return 0;
    uintptr_t base = (uintptr_t)cl;
    patch_legacy(base);
    patch_insecure(base, image_size(base));
    hook_banner(base);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        CreateThread(nullptr, 0, worker, nullptr, 0, nullptr);
    }
    return TRUE;
}