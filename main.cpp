#include <windows.h>
#include <cstdio>
#include <cstring>

static const char* CLIENT_VERSION  = "2000258";

static LPSTR skip_token(LPSTR p) {
    if (*p == '"') { ++p; while (*p && *p != '"') ++p; if (*p == '"') ++p; }
    else           { while (*p && *p != ' ') ++p; }
    while (*p == ' ') ++p;
    return p;
}

static void first_path(LPSTR p, char* out, size_t cap) {
    const char *b, *e;
    if (*p == '"') { b = p + 1; e = b; while (*e && *e != '"') ++e; }
    else           { b = p;     e = b; while (*e && *e != ' ') ++e; }
    size_t len = (size_t)(e - b); if (len >= cap) len = cap - 1;
    memcpy(out, b, len); out[len] = 0;
}

static void pin_client_version(const char* gamedir) {
    char path[MAX_PATH * 2];
    snprintf(path, sizeof(path), "%s\\csgo\\steam.inf", gamedir);

    SetFileAttributesA(path, FILE_ATTRIBUTE_NORMAL);
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return;
    char in[16384]; DWORD n = 0;
    ReadFile(f, in, sizeof(in) - 1, &n, nullptr); in[n] = 0; CloseHandle(f);

    char out[16384]; int o = 0; bool done = false;
    for (const char* p = in; *p; ) {
        const char* eol = p; while (*eol && *eol != '\n') ++eol;
        int len = (int)(eol - p); while (len > 0 && p[len - 1] == '\r') --len;
        if (_strnicmp(p, "ClientVersion=", 14) == 0) {
            o += snprintf(out + o, sizeof(out) - o, "ClientVersion=%s\r\n", CLIENT_VERSION);
            done = true;
        } else if (o + len + 2 < (int)sizeof(out)) {
            memcpy(out + o, p, len); o += len; out[o++] = '\r'; out[o++] = '\n';
        }
        p = *eol ? eol + 1 : eol;
    }
    if (!done) o += snprintf(out + o, sizeof(out) - o, "ClientVersion=%s\r\n", CLIENT_VERSION);

    f = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f != INVALID_HANDLE_VALUE) { DWORD w; WriteFile(f, out, o, &w, nullptr); CloseHandle(f); }
}

static void inject_dll(HANDLE proc, const char* dllpath) {
    SIZE_T len = strlen(dllpath) + 1;
    void* rem = VirtualAllocEx(proc, nullptr, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!rem) return;
    if (WriteProcessMemory(proc, rem, dllpath, len, nullptr)) {
        auto pLoad = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
        HANDLE th = CreateRemoteThread(proc, nullptr, 0, pLoad, rem, 0, nullptr);
        if (th) { WaitForSingleObject(th, 5000); CloseHandle(th); }
    }
    VirtualFreeEx(proc, rem, 0, MEM_RELEASE);
}

int main() {
    LPSTR cmd = skip_token(GetCommandLineA());
    if (!*cmd) {
        MessageBoxA(nullptr,
            "run this through steam, not directly.\n\n"
            "set csgo legacy launch options to:\n"
            "\"<path>\\inventory_fix.exe\" %command%",
            "inventory_fix", MB_ICONINFORMATION);
        return 1;
    }

    char exe[MAX_PATH * 2] = {0}, dir[MAX_PATH * 2] = {0};
    first_path(cmd, exe, sizeof(exe));
    lstrcpynA(dir, exe, sizeof(dir));
    if (char* s = strrchr(dir, '\\')) *s = 0; else dir[0] = 0;

    if (dir[0]) pin_client_version(dir);

    SetEnvironmentVariableA("SteamAppId",  "730");
    SetEnvironmentVariableA("SteamGameId", "730");

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    if (!CreateProcessA(nullptr, cmd, nullptr, nullptr, TRUE, 0,
                        nullptr, dir[0] ? dir : nullptr, &si, &pi)) {
        MessageBoxA(nullptr, "failed to launch the game!\ncheck the path in your launch options.",
                    "inventory_fix", MB_ICONERROR);
        return 1;
    }

    char self[MAX_PATH * 2]; GetModuleFileNameA(nullptr, self, sizeof(self));
    if (char* s = strrchr(self, '\\')) {
        lstrcpynA(s + 1, "inventory_fix.dll", (int)(sizeof(self) - (s + 1 - self)));
        if (GetFileAttributesA(self) != INVALID_FILE_ATTRIBUTES) {
            inject_dll(pi.hProcess, self);
        }
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;
}