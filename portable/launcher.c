/*
 * SerialMon portable launcher (C, Win32).
 * Exe tunggal: paket zip (SerialMonitor.exe + DLL Qt) ditanam sebagai resource.
 * Pertama dijalankan: ekstrak ke %LOCALAPPDATA%\SerialMon\<versi>_<ukuran> memakai tar.exe bawaan
 * Windows 10/11, lalu jalankan SerialMonitor.exe dengan argumen yang sama. Berikutnya langsung jalan.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <strsafe.h>

#ifndef APP_VER
#define APP_VER L"dev"
#endif

static void fail(const wchar_t* msg)
{
    MessageBoxW(NULL, msg, L"SerialMon", MB_ICONERROR | MB_OK);
    ExitProcess(1);
}

static int file_exists(const wchar_t* p)
{
    DWORD a = GetFileAttributesW(p);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

/* argumen setelah nama program pada command line */
static const wchar_t* skip_program(const wchar_t* c)
{
    if (*c == L'"') {
        c++;
        while (*c && *c != L'"') c++;
        if (*c) c++;
    } else {
        while (*c && *c != L' ' && *c != L'\t') c++;
    }
    while (*c == L' ' || *c == L'\t') c++;
    return c;
}

static void extract(const void* data, DWORD size, const wchar_t* parent, const wchar_t* dest)
{
    wchar_t tmp[MAX_PATH], zip[MAX_PATH], sysdir[MAX_PATH], cmd[4 * MAX_PATH];

    CreateDirectoryW(parent, NULL);
    StringCchPrintfW(tmp, MAX_PATH, L"%s.tmp%lu", dest, GetCurrentProcessId());
    if (!CreateDirectoryW(tmp, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        fail(L"Tidak bisa membuat folder di %LOCALAPPDATA%.");

    StringCchPrintfW(zip, MAX_PATH, L"%s\\pkg.zip", tmp);
    HANDLE f = CreateFileW(zip, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) fail(L"Tidak bisa menulis berkas sementara.");
    DWORD written = 0;
    BOOL ok = WriteFile(f, data, size, &written, NULL);
    CloseHandle(f);
    if (!ok || written != size) fail(L"Gagal menulis paket (disk penuh?).");

    GetSystemDirectoryW(sysdir, MAX_PATH);
    StringCchPrintfW(cmd, 4 * MAX_PATH, L"\"%s\\tar.exe\" -xf \"%s\" -C \"%s\"", sysdir, zip, tmp);
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;
    if (!CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
        fail(L"tar.exe tidak ditemukan. SerialMon butuh Windows 10/11.");
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    DeleteFileW(zip);
    if (code != 0) fail(L"Gagal mengekstrak paket.");

    /* pindahkan ke nama final; kalau instance lain sudah menang, tidak apa-apa */
    MoveFileExW(tmp, dest, 0);
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, PWSTR cmdline, int show)
{
    (void)inst; (void)prev; (void)cmdline; (void)show;

    HRSRC r = FindResourceW(NULL, MAKEINTRESOURCEW(1), MAKEINTRESOURCEW(10));
    if (!r) fail(L"Paket tidak ditemukan di dalam exe.");
    DWORD size = SizeofResource(NULL, r);
    const void* data = LockResource(LoadResource(NULL, r));
    if (!data || !size) fail(L"Paket di dalam exe rusak.");

    wchar_t base[MAX_PATH], parent[MAX_PATH], dest[MAX_PATH], dir[MAX_PATH], exe[MAX_PATH];
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", base, MAX_PATH);
    if (!n || n >= MAX_PATH) fail(L"%LOCALAPPDATA% tidak tersedia.");
    StringCchPrintfW(parent, MAX_PATH, L"%s\\SerialMon", base);
    StringCchPrintfW(dest, MAX_PATH, L"%s\\%s_%lu", parent, APP_VER, size);
    StringCchPrintfW(dir, MAX_PATH, L"%s\\SerialMon", dest);
    StringCchPrintfW(exe, MAX_PATH, L"%s\\SerialMonitor.exe", dir);

    if (!file_exists(exe)) {
        extract(data, size, parent, dest);
        if (!file_exists(exe)) fail(L"SerialMonitor.exe tidak ada setelah ekstrak.");
    }

    static wchar_t cmd[32768];
    StringCchPrintfW(cmd, 32768, L"\"%s\" %s", exe, skip_program(GetCommandLineW()));
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;
    if (!CreateProcessW(exe, cmd, NULL, NULL, FALSE, 0, NULL, dir, &si, &pi))
        fail(L"Gagal menjalankan SerialMonitor.exe.");
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}
