/*
 * defender_gui.c  -  Win32 GUI one-click toggle for Microsoft Defender
 *
 * Green panel  = protection ON
 * Red panel    = fully OFF
 * Amber panel  = waiting for restart / partly off
 *
 * 需要系統管理員：由 manifest 的 requireAdministrator 保證。
 * 竄改防護（Tamper Protection）只能手動關，微軟鎖定。
 *
 * 服務 Start 值在 Windows 執行中多半改不動，所以由離線映像處理（資料夾預設
 * C:\DefenderOffline，可用環境變數 DEFENDER_OFFLINE_DIR 改到別的磁碟）：
 * 寫 action.txt + bcdedit /bootsequence（只生效一次），下次重開自動進映像改登錄檔，
 * 再重開回 Windows。Defender 沒在跑時改得動的服務就直接改，全部到位就不進映像。
 * 離線腳本內嵌在 exe（app.rc 的 RCDATA 201~204，原始檔在 offline\）。第一次需要映像時
 * 跑內嵌的 build_pe.ps1：從本機修復分割區複製 WinRE、改 winpeshl.ini、建隱藏 BCD 項目。
 *
 * 不帶 C 執行期，也不靜態連結 shell32（只有開設定頁才載入），啟動才快、記憶體才小。
 *
 * Build（需要 zig，建議直接跑 build.ps1）：
 *   zig rc /fo app.res app.rc
 *   zig cc -target x86_64-windows-gnu -Os -s -fno-stack-protector -fno-builtin
 *     -nostdlib -isystem <ZIG>/lib/libc/include/any-windows-any
 *     -Wl,--entry=start -Wl,--subsystem,windows defender_gui.c app.res
 *     -o defender_gui.exe -lkernel32 -luser32 -lgdi32 -ladvapi32 -limm32
 */

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#undef WINVER
#define WINVER 0x0A00
#include <windows.h>
#include <imm.h>
#include <tlhelp32.h>

unsigned int _tls_index;  /* 不帶 C 執行期時連結器要的符號 */

void *memset(void *d, int c, size_t n) {
    volatile unsigned char *p = d;
    while (n--) *p++ = (unsigned char)c;
    return d;
}
void *memcpy(void *d, const void *s, size_t n) {
    volatile unsigned char *a = d;
    const unsigned char *b = s;
    while (n--) *a++ = *b++;
    return d;
}
int memcmp(const void *x, const void *y, size_t n) {
    const unsigned char *a = x, *b = y;
    for (; n--; a++, b++) if (*a != *b) return *a < *b ? -1 : 1;
    return 0;
}

static int g_dpi = 96;
#define S(x) MulDiv((x), g_dpi, 96)   /* 依 DPI 等比縮放 */

#define POLICY_BASE "SOFTWARE\\Policies\\Microsoft\\Windows Defender"
#define TP_KEY      "SOFTWARE\\Microsoft\\Windows Defender\\Features"
#define RT_PREF_KEY "SOFTWARE\\Microsoft\\Windows Defender\\Real-Time Protection"
#define SVC_ROOT    "SYSTEM\\CurrentControlSet\\Services\\"
#define TRAY_KEY    "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run"
/* {bootmgr} 的固定 GUID；元素 24000002 = bootsequence */
#define BOOTSEQ_KEY "BCD00000000\\Objects\\{9dea862c-5cdd-4e70-acc1-f32b344d4795}\\Elements\\24000002"

#define TEXT_W_(s)   L##s
#define TEXT_W(s)    TEXT_W_(s)

/* 離線資料夾：可用環境變數 DEFENDER_OFFLINE_DIR 覆寫，預設 C:\DefenderOffline。
   必須是某個固定內接磁碟根目錄下的資料夾——重開進 WinRE 時磁碟代號可能改變，
   df_run.cmd 以「相對路徑」掃所有磁碟找它，所以存的是相對路徑而非絕對路徑。
   路徑請用 ASCII。 */
static wchar_t g_off_dir[MAX_PATH] = L"C:\\DefenderOffline";  /* 無尾斜線 */

static void resolve_off_dir(void) {
    wchar_t buf[MAX_PATH];
    DWORD n = GetEnvironmentVariableW(L"DEFENDER_OFFLINE_DIR", buf, MAX_PATH);
    if (n > 0 && n < MAX_PATH) {
        int len = (int)n;
        while (len > 0 && (buf[len-1] == L'\\' || buf[len-1] == L'/')) buf[--len] = 0;
        if (len >= 3 && buf[1] == L':') lstrcpyW(g_off_dir, buf);  /* 需像 X:\... */
    }
}

/* out = g_off_dir "\" sub  （out 需 MAX_PATH 大小） */
static void off_path(wchar_t *out, const wchar_t *sub) {
    lstrcpyW(out, g_off_dir);
    lstrcatW(out, L"\\");
    lstrcatW(out, sub);
}

#define IDC_DISABLE 1001
#define IDC_ENABLE  1002
#define IDC_TAMPER  1003
#define IDC_LANG    1004

/* ---- 語言 -------------------------------------------------------------- */
#define NLANG 2
#define L_EN 0
#define L_ZH 1
static int g_lang;
#define TR(a) ((a)[g_lang])

static const wchar_t *const S_WINTITLE[NLANG] = { L"Defender Toggle", L"Defender 一鍵切換" };
static const wchar_t *const S_BTN_DISABLE[NLANG] = { L"Disable protection", L"關閉防護" };
static const wchar_t *const S_BTN_ENABLE[NLANG]  = { L"Enable protection", L"開啟防護" };
static const wchar_t *const S_BTN_TAMPER_ON[NLANG] = {
    L"Disable Tamper Protection (open settings)", L"關閉竄改防護（開啟設定頁）" };
static const wchar_t *const S_BTN_TAMPER_OFF[NLANG] = {
    L"Tamper Protection is off \u2713", L"竄改防護已關閉 \u2713" };
static const wchar_t *const S_BTN_LANG[NLANG] = { L"\u4e2d\u6587", L"English" };

static const wchar_t *const S_TAMPER_ALREADY_OFF[NLANG] = {
    L"Tamper Protection is already off.", L"竄改防護已經是關閉狀態。" };
static const wchar_t *const S_TITLE_TAMPER[NLANG] = { L"Tamper Protection", L"竄改防護" };
static const wchar_t *const S_TITLE_DEFSWITCH[NLANG] = { L"Defender toggle", L"Defender 切換" };
static const wchar_t *const S_TAMPER_OPENED[NLANG] = {
    L"Opened the \"Virus & threat protection settings\" page.\n\n"
    L"Please turn off \"Tamper Protection\",\n"
    L"then come back to this window and click \"Disable/Enable protection\" again.\n\n"
    L"Note: Tamper Protection is locked by Microsoft; no program can turn it off "
    L"automatically. It can only be toggled manually on that page.",
    L"已開啟「病毒與威脅防護設定」頁。\n\n"
    L"請把「竄改保護 (Tamper Protection)」切為關閉，\n"
    L"再回到本視窗按「開啟/關閉防護」。\n\n"
    L"註：竄改防護是微軟鎖定的開關，任何程式都無法自動關閉，\n"
    L"只能在這個頁面手動撥動。" };

static const wchar_t *const S_BUILDING_TITLE[NLANG] = { L"Preparing boot image", L"正在準備開機映像" };
static const wchar_t *const S_BUILDING_SUB[NLANG] = {
    L"First-time setup, takes a few minutes, please don't close this window",
    L"第一次使用，約需幾分鐘，請不要關閉視窗" };
static const wchar_t *const S_PEND_DISABLE_TITLE[NLANG] = { L"Will fully disable after restart", L"重開後徹底關閉" };
static const wchar_t *const S_PEND_RESTORE_TITLE[NLANG] = { L"Will restore after restart", L"重開後恢復防護" };
static const wchar_t *const S_PEND_SUB[NLANG] = {
    L"Scheduled; it will finish automatically on restart, nothing else to do",
    L"已排好，重開時自動完成，不用按任何東西" };
static const wchar_t *const S_FULLOFF_TITLE[NLANG] = { L"Defender is fully disabled", L"Defender 已徹底關閉" };
static const wchar_t *const S_FULLOFF_SUB_PENDING[NLANG] = {
    L"Settings are ready; restart to actually stop it",
    L"設定已改好，重開才會真的停止" };
static const wchar_t *const S_FULLOFF_SUB_DONE[NLANG] = {
    L"Real-time protection, services, tasks and tray icon are all disabled",
    L"即時防護、服務、排程與小圖示皆已停用" };
static const wchar_t *const S_RUNNING_TITLE[NLANG] = { L"Defender is running", L"Defender 運作中" };
static const wchar_t *const S_RUNNING_SUB[NLANG] = { L"Real-time protection is normal", L"即時防護正常" };
static const wchar_t *const S_FULLON_SUB_PENDING[NLANG] = {
    L"Settings restored; restart to actually enable it",
    L"設定已還原，重開才會真的啟動" };
static const wchar_t *const S_PARTIAL_TITLE[NLANG] = { L"Defender is partially disabled", L"Defender 部分關閉" };
static const wchar_t *const S_PARTIAL_SUB_RUNNING[NLANG] = {
    L"Real-time protection may be off, but services are still running",
    L"即時防護可能已關，但服務還在跑" };
static const wchar_t *const S_PARTIAL_SUB_OTHER[NLANG] = {
    L"Some service settings are inconsistent, you can click the button again",
    L"有服務設定不一致，可再按一次按鈕" };
static const wchar_t *const S_TAMPER_WARN[NLANG] = {
    L"\u26a0 Tamper Protection is on, please turn it off manually first",
    L"\u26a0 竄改防護開啟中，請先手動關閉再操作" };

static const wchar_t *const S_VERB_DISABLE[NLANG] = { L"disabled", L"關閉" };
static const wchar_t *const S_VERB_ENABLE[NLANG]  = { L"restored", L"恢復" };
static const wchar_t *const S_FMT_SVC_NOT_SCHEDULED[NLANG] = {
    L"Real-time protection %s, but services were not scheduled: %s\n\nServices remain unchanged.",
    L"即時防護已%s，但服務沒有排好：%s\n\n目前服務維持原狀。" };
static const wchar_t *const S_IMG_NOT_READY[NLANG] = {
    L"the boot image isn't ready yet.", L"開機映像還沒準備好。" };
static const wchar_t *const S_SCHED_FAILED[NLANG] = {
    L"failed to schedule for next boot.", L"排定下次開機失敗。" };
static const wchar_t *const S_FMT_DONE_NOREBOOT[NLANG] = {
    L"%s complete, no restart needed.", L"%s完成，不用重開。" };
static const wchar_t *const S_FMT_REBOOT_ASK[NLANG] = {
    L"Real-time protection %s, %s\n\nRestart now? Choose \"No\" to apply it on your next restart.",
    L"即時防護已%s，%s\n\n要現在重開嗎？選「否」就等你下次重開才生效。" };
static const wchar_t *const S_REST_ARMED[NLANG] = {
    L"the rest is scheduled.\nIt will finish automatically on restart (an extra restart), nothing else to do.",
    L"其餘設定已排好。\n重開時會自動完成（會多重開一次），不用按任何東西。" };
static const wchar_t *const S_REST_DONE[NLANG] = {
    L"the rest has also been changed; one restart will apply it.",
    L"其餘設定也已改好，重開一次就生效。" };
static const wchar_t *const S_FMT_WRITE_FAIL[NLANG] = {
    L"%d setting(s) failed to write (Tamper Protection may be on).\n\n"
    L"Go to Windows Security > Virus & threat protection > Manage settings, "
    L"turn off Tamper Protection, then try again.",
    L"有 %d 項設定寫入失敗（可能是竄改防護開啟中）。\n\n"
    L"請到 Windows 安全性 > 病毒與威脅防護 > 管理設定，"
    L"關閉竄改保護後再試。" };
static const wchar_t *const S_ASK_BUILD_IMAGE[NLANG] = {
    L"First-time setup: a boot image needs to be prepared first:\n"
    L"a copy of this PC's built-in recovery system will be made and configured,\n"
    L"takes about 3-5 minutes and uses about 1GB (%s).\n\n"
    L"Prepare it now?",
    L"第一次使用，要先準備重開時用的開機映像：\n"
    L"從這台電腦內建的修復系統複製一份並改好，\n"
    L"約需 3～5 分鐘，會占用約 1GB（%s）。\n\n"
    L"要現在準備嗎？" };
static const wchar_t *const S_BUILD_FAILED[NLANG] = {
    L"Failed to prepare the boot image, services remain unchanged.\n\n"
    L"Details in %s\\build.log.",
    L"開機映像準備失敗，服務維持原狀。\n\n"
    L"詳細原因在 %s\\build.log。" };
static const wchar_t *const S_BUILDING_CLOSE_WARN[NLANG] = {
    L"Preparing the boot image, please don't close before it finishes.",
    L"正在準備開機映像，完成前請不要關閉。" };

/* ---- state ------------------------------------------------------------ */
typedef struct {
    int disabled;   /* 政策已關 */
    int tp_on;      /* 竄改防護開著 */
    int n_off;      /* Start==4 的服務數 */
    int n_def;      /* Start==出廠值 的服務數 */
    int pending;    /* 0=無 1=重開後停用 2=重開後還原 */
    int running;    /* WinDefend 正在跑 */
    int building;   /* 正在準備開機映像 */
} State;
static State g_st;

static int g_policy_fail;   /* 上一次動作的設定寫入失敗數 */
enum { OFF_NOT_NEEDED, OFF_ARMED, OFF_MISSING, OFF_FAILED };
static int g_offline;       /* 上一次動作的離線排程結果 */
static HFONT g_titleFont, g_uiFont;
static HWND  g_hDisable, g_hEnable, g_hTamper, g_hLang;
static volatile LONG g_building;   /* 背景執行緒正在做映像 */
static int g_build_disable;        /* 做完映像後要排的動作 */
#define WM_APP_BUILT (WM_APP + 1)

/* ---- low-level helpers ------------------------------------------------ */

static int set_dword(const char *subkey, const char *name, DWORD val) {
    HKEY h;
    if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, subkey, 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &h, NULL) != ERROR_SUCCESS)
        return 1;
    LONG r = RegSetValueExA(h, name, 0, REG_DWORD, (const BYTE*)&val, sizeof(val));
    RegCloseKey(h);
    return r == ERROR_SUCCESS ? 0 : 1;
}

static int get_dword(const char *subkey, const char *name, DWORD *out) {
    DWORD sz = sizeof(DWORD);
    return RegGetValueA(HKEY_LOCAL_MACHINE, subkey, name, RRF_RT_REG_DWORD,
                        NULL, out, &sz) == ERROR_SUCCESS;
}

static void svc_path(char *path, const char *svc) {
    lstrcpyA(path, SVC_ROOT);
    lstrcatA(path, svc);
}

static void set_service_start(const char *svc, DWORD start) {
    char path[128];
    HKEY h;
    svc_path(path, svc);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, path, 0, KEY_SET_VALUE, &h) != ERROR_SUCCESS)
        return;
    /* Defender 的核心保護在跑時會回拒絕存取，屬預期；沒改到的交給離線映像 */
    RegSetValueExA(h, "Start", 0, REG_DWORD, (const BYTE*)&start, sizeof(start));
    RegCloseKey(h);
}

/* 啟動 System32 內的程式，回傳行程控制代碼，失敗回 NULL */
static HANDLE spawn_sys32(const wchar_t *rel, const wchar_t *args) {
    wchar_t sys[MAX_PATH];
    static wchar_t cmd[1024];
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si)); si.cb = sizeof(si);
    GetSystemDirectoryW(sys, MAX_PATH);
    wsprintfW(cmd, L"\"%s\\%s\" %s", sys, rel, args);
    if (!CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
        return NULL;
    CloseHandle(pi.hThread);
    return pi.hProcess;
}

static int run_sys32(const wchar_t *rel, const wchar_t *args) {
    HANDLE p = spawn_sys32(rel, args);
    if (!p) return -1;
    WaitForSingleObject(p, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(p, &code);
    CloseHandle(p);
    return (int)code;
}

static void set_realtime_pref(int disable) {
    wchar_t args[160];
    wsprintfW(args, L"-NoProfile -ExecutionPolicy Bypass -Command "
                    L"\"Set-MpPreference -DisableRealtimeMonitoring $%s "
                    L"-ErrorAction SilentlyContinue\"", disable ? L"true" : L"false");
    run_sys32(L"WindowsPowerShell\\v1.0\\powershell.exe", args);
}

/* ---- data ------------------------------------------------------------- */

static const wchar_t *TASKS[] = {
    L"Windows Defender Cache Maintenance",
    L"Windows Defender Cleanup",
    L"Windows Defender Scheduled Scan",
    L"Windows Defender Verification",
};
#define NTASKS (int)(sizeof(TASKS)/sizeof(TASKS[0]))

struct svc { const char *name; DWORD def; };
static const struct svc SVCS[] = {
    { "WinDefend", 2 }, { "WdFilter", 0 }, { "WdBoot", 0 },
    { "WdNisDrv", 3 }, { "WdNisSvc", 3 },
    { "MDCoreSvc", 2 }, { "SecurityHealthService", 3 },
};
#define NSVCS (int)(sizeof(SVCS)/sizeof(SVCS[0]))

/* 幾個排程同時改，不用一個等一個 */
static void set_tasks(int enable) {
    HANDLE p[NTASKS];
    int n = 0;
    for (int i = 0; i < NTASKS; i++) {
        wchar_t args[200];
        wsprintfW(args, L"/Change /TN \"\\Microsoft\\Windows\\Windows Defender\\%s\" /%s",
                  TASKS[i], enable ? L"Enable" : L"Disable");
        HANDLE h = spawn_sys32(L"schtasks.exe", args);
        if (h) p[n++] = h;
    }
    if (n) WaitForMultipleObjects(n, p, TRUE, INFINITE);
    for (int i = 0; i < n; i++) CloseHandle(p[i]);
}

/* ---- offline image (C:\DefenderOffline) ------------------------------- */

static int file_exists(const wchar_t *p) {
    return GetFileAttributesW(p) != INVALID_FILE_ATTRIBUTES;
}

static int read_text(const wchar_t *p, char *buf, DWORD n) {
    HANDLE h = CreateFileW(p, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return 0;
    DWORD got = 0;
    BOOL ok = ReadFile(h, buf, n - 1, &got, NULL);
    CloseHandle(h);
    if (!ok) return 0;
    buf[got] = 0;
    return 1;
}

/* 開機項目 GUID，格式不對就回 0 */
static int read_guid(wchar_t *out) {
    char a[64];
    wchar_t gp[MAX_PATH];
    off_path(gp, L"pe\\entry-guid.txt");
    if (!read_text(gp, a, sizeof(a)) || a[0] != '{' || lstrlenA(a) < 38 || a[37] != '}')
        return 0;
    for (int i = 0; i < 38; i++) out[i] = (wchar_t)a[i];
    out[38] = 0;
    return 1;
}

/* 映像檔在、開機選單裡也真的有這個項目（重灌或修復開機後可能被清掉）*/
static int offline_ready(void) {
    wchar_t g[40], k[96], w1[MAX_PATH], w2[MAX_PATH];
    HKEY h;
    off_path(w1, L"pe\\winre.wim");
    off_path(w2, L"pe\\boot.sdi");
    if (!file_exists(w1) || !file_exists(w2) || !read_guid(g))
        return 0;
    wsprintfW(k, L"BCD00000000\\Objects\\%s", g);
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, k, 0, KEY_READ, &h) != ERROR_SUCCESS) return 0;
    RegCloseKey(h);
    return 1;
}

/* 把內嵌的離線腳本寫到 C:\DefenderOffline（每次排程前都寫，確保跟 exe 同版）*/
static const struct { WORD id; const wchar_t *name; } EMBED[] = {
    { 201, L"df_run.cmd" },
    { 202, L"disable_defender_offline.cmd" },
    { 203, L"restore_defender_offline.cmd" },
    { 204, L"build_pe.ps1" },
};

static int write_embedded(void) {
    char dfbuf[4096];
    char rel[MAX_PATH];
    int rl = 0;
    /* 離線資料夾的相對路徑（去掉磁碟代號），例如 C:\DefenderOffline -> \DefenderOffline */
    const wchar_t *rp = (g_off_dir[0] && g_off_dir[1] == L':') ? g_off_dir + 2 : g_off_dir;
    for (; *rp && rl < (int)sizeof(rel) - 1; rp++) rel[rl++] = (char)*rp;
    rel[rl] = 0;

    CreateDirectoryW(g_off_dir, NULL);
    for (int i = 0; i < (int)(sizeof(EMBED)/sizeof(EMBED[0])); i++) {
        HRSRC r = FindResourceW(NULL, MAKEINTRESOURCEW(EMBED[i].id), MAKEINTRESOURCEW(10) /* RT_RCDATA */);
        HGLOBAL g = r ? LoadResource(NULL, r) : NULL;
        const void *p = g ? LockResource(g) : NULL;
        DWORD n = r ? SizeofResource(NULL, r) : 0;
        if (!p || !n) return 0;

        const void *out = p;
        DWORD outn = n;
        /* df_run.cmd 在 WinRE 執行：把 @@RELDIR@@ 代換成離線資料夾的相對路徑 */
        if (EMBED[i].id == 201) {
            const char *src = (const char *)p;
            DWORD si = 0, di = 0;
            while (si < n && di < sizeof(dfbuf)) {
                if (si + 10 <= n && memcmp(src + si, "@@RELDIR@@", 10) == 0) {
                    for (int k = 0; k < rl && di < sizeof(dfbuf); k++) dfbuf[di++] = rel[k];
                    si += 10;
                } else {
                    dfbuf[di++] = src[si++];
                }
            }
            out = dfbuf; outn = di;
        }

        wchar_t path[MAX_PATH];
        off_path(path, EMBED[i].name);
        HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) return 0;
        DWORD w = 0;
        BOOL ok = WriteFile(h, out, outn, &w, NULL) && w == outn;
        CloseHandle(h);
        if (!ok) return 0;
    }
    return 1;
}

/* 下次開機是否排了我們的項目：直接讀 BCD 登錄檔，不用開 bcdedit */
static int boot_armed(void) {
    wchar_t g[40], v[128];
    DWORD sz = sizeof(v) - sizeof(wchar_t);
    if (!read_guid(g)) return 0;
    memset(v, 0, sizeof(v));
    if (RegGetValueW(HKEY_LOCAL_MACHINE, TEXT_W(BOOTSEQ_KEY), L"Element",
                     RRF_RT_REG_MULTI_SZ, NULL, v, &sz) != ERROR_SUCCESS)
        return 0;
    return lstrcmpiW(v, g) == 0;
}

static void cancel_offline(void) {
    wchar_t ap[MAX_PATH];
    off_path(ap, L"action.txt");
    run_sys32(L"bcdedit.exe", L"/deletevalue {bootmgr} bootsequence");
    DeleteFileW(ap);
}

static int arm_offline(int disable) {
    wchar_t g[40], args[80], ap[MAX_PATH];
    if (!read_guid(g)) return 0;
    off_path(ap, L"action.txt");
    const char *txt = disable ? "disable\r\n" : "restore\r\n";
    HANDLE h = CreateFileW(ap, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return 0;
    DWORD w = 0;
    BOOL ok = WriteFile(h, txt, (DWORD)lstrlenA(txt), &w, NULL);
    CloseHandle(h);
    wsprintfW(args, L"/bootsequence %s", g);
    if (!ok || run_sys32(L"bcdedit.exe", args) != 0 || !boot_armed()) {
        DeleteFileW(ap);
        return 0;
    }
    return 1;
}

/* action.txt 與開機設定對不上時（上次沒跑成）清掉，只在啟動時做 */
static void reconcile_pending(void) {
    wchar_t ap[MAX_PATH];
    off_path(ap, L"action.txt");
    int has_action = file_exists(ap);
    int armed = boot_armed();
    if (has_action && !armed) DeleteFileW(ap);
    else if (!has_action && armed) cancel_offline();
}

static int read_pending(void) {
    char a[32];
    wchar_t ap[MAX_PATH];
    off_path(ap, L"action.txt");
    if (!read_text(ap, a, sizeof(a))) return 0;
    if ((a[0] | 0x20) == 'd') return 1;
    if ((a[0] | 0x20) == 'r') return 2;
    return 0;
}

static int defender_running(void) {
    int run = 0;
    SC_HANDLE m = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!m) return 0;
    SC_HANDLE s = OpenServiceW(m, L"WinDefend", SERVICE_QUERY_STATUS);
    if (s) {
        SERVICE_STATUS st;
        if (QueryServiceStatus(s, &st)) run = (st.dwCurrentState != SERVICE_STOPPED);
        CloseServiceHandle(s);
    }
    CloseServiceHandle(m);
    return run;
}

/* 工作列小圖示的開機自動啟動（跟工作管理員停用同一個開關）*/
static int set_tray(int enable) {
    BYTE v[12];
    memset(v, 0, sizeof(v));
    v[0] = enable ? 2 : 3;
    if (!enable) { FILETIME ft; GetSystemTimeAsFileTime(&ft); memcpy(v + 4, &ft, 8); }
    HKEY h;
    if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, TRAY_KEY, 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &h, NULL) != ERROR_SUCCESS)
        return 1;
    LONG r = RegSetValueExA(h, "SecurityHealth", 0, REG_BINARY, v, sizeof(v));
    RegCloseKey(h);
    return r == ERROR_SUCCESS ? 0 : 1;
}

static void kill_tray(void) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    PROCESSENTRY32W e;
    e.dwSize = sizeof(e);
    for (BOOL ok = Process32FirstW(snap, &e); ok; ok = Process32NextW(snap, &e)) {
        if (lstrcmpiW(e.szExeFile, L"SecurityHealthSystray.exe") == 0) {
            HANDLE p = OpenProcess(PROCESS_TERMINATE, FALSE, e.th32ProcessID);
            if (p) { TerminateProcess(p, 1); CloseHandle(p); }
        }
    }
    CloseHandle(snap);
}

/* 服務沒全部到目標值才排離線；都到位就取消殘留的排程 */
static void apply_offline(int disable) {
    int need = 0;
    for (int i = 0; i < NSVCS; i++) {
        char path[128]; DWORD v = 0;
        svc_path(path, SVCS[i].name);
        if (get_dword(path, "Start", &v) && v != (disable ? 4 : SVCS[i].def)) need = 1;
    }
    /* 這個偏好的鑰匙只有 SYSTEM 寫得動；還原時若還是 1，要靠離線映像清掉，
       否則 Defender 回來後即時防護仍是關的 */
    if (!disable) {
        DWORD v = 0;
        if (get_dword(RT_PREF_KEY, "DisableRealtimeMonitoring", &v) && v != 0) need = 1;
    }
    if (!need) {
        wchar_t ap[MAX_PATH];
        off_path(ap, L"action.txt");
        if (file_exists(ap) || boot_armed()) cancel_offline();
        g_offline = OFF_NOT_NEEDED;
    } else if (!offline_ready()) {
        g_offline = OFF_MISSING;
    } else {
        g_offline = (write_embedded() && arm_offline(disable)) ? OFF_ARMED : OFF_FAILED;
    }
}

/* 背景做映像（約幾分鐘），做完直接排離線動作，再通知視窗 */
static DWORD WINAPI build_thread(LPVOID param) {
    wchar_t args[400];
    wsprintfW(args,
        L"-NoProfile -ExecutionPolicy Bypass -File \"%s\\build_pe.ps1\" -Root \"%s\"",
        g_off_dir, g_off_dir);
    int ok = write_embedded()
        && run_sys32(L"WindowsPowerShell\\v1.0\\powershell.exe", args) == 0
        && offline_ready();
    if (ok) apply_offline(g_build_disable);
    InterlockedExchange(&g_building, 0);
    PostMessageW((HWND)param, WM_APP_BUILT, (WPARAM)ok, 0);
    return 0;
}

/* ---- actions ---------------------------------------------------------- */

static void act_disable(void) {
    g_policy_fail = 0;
    g_policy_fail += set_dword(POLICY_BASE, "DisableAntiSpyware", 1);
    g_policy_fail += set_dword(POLICY_BASE "\\Real-Time Protection", "DisableRealtimeMonitoring", 1);
    g_policy_fail += set_dword(POLICY_BASE "\\Real-Time Protection", "DisableBehaviorMonitoring", 1);
    g_policy_fail += set_dword(POLICY_BASE "\\Real-Time Protection", "DisableOnAccessProtection", 1);
    g_policy_fail += set_dword(POLICY_BASE "\\Real-Time Protection", "DisableIOAVProtection", 1);
    g_policy_fail += set_dword(POLICY_BASE "\\Real-Time Protection", "DisableScanOnRealtimeEnable", 1);
    g_policy_fail += set_dword(POLICY_BASE "\\Spynet", "SpynetReporting", 0);
    g_policy_fail += set_dword(POLICY_BASE "\\Spynet", "SubmitSamplesConsent", 0);
    g_policy_fail += set_dword(POLICY_BASE "\\Spynet", "DisableBlockAtFirstSeen", 1);
    for (int i = 0; i < NSVCS; i++) set_service_start(SVCS[i].name, 4);
    set_tasks(0);
    if (defender_running()) set_realtime_pref(1);   /* Defender 沒在跑時政策已足夠，不用開 PowerShell */
    g_policy_fail += set_tray(0);
    kill_tray();
    apply_offline(1);
}

static void act_enable(void) {
    g_policy_fail = 0;
    LONG r = RegDeleteTreeA(HKEY_LOCAL_MACHINE, POLICY_BASE);
    if (r != ERROR_SUCCESS && r != ERROR_FILE_NOT_FOUND) g_policy_fail++;
    for (int i = 0; i < NSVCS; i++) set_service_start(SVCS[i].name, SVCS[i].def);
    set_tasks(1);
    /* Set-MpPreference 要 Defender 在跑才有效；沒在跑時由 apply_offline 排離線映像清偏好 */
    if (defender_running()) set_realtime_pref(0);
    g_policy_fail += set_tray(1);
    apply_offline(0);
}

/* ---- state refresh ---------------------------------------------------- */

static void read_state(void) {
    DWORD v = 0;
    g_st.disabled = (get_dword(POLICY_BASE, "DisableAntiSpyware", &v) && v == 1);
    v = 0;
    g_st.tp_on = (get_dword(TP_KEY, "TamperProtection", &v) && (v & 1));   /* 5=開、4=關（0 也是關）*/

    g_st.n_off = g_st.n_def = 0;
    for (int i = 0; i < NSVCS; i++) {
        char path[128]; DWORD s = 0;
        svc_path(path, SVCS[i].name);
        if (!get_dword(path, "Start", &s)) continue;
        if (s == 4) g_st.n_off++;
        if (s == SVCS[i].def) g_st.n_def++;
    }
    g_st.pending = read_pending();
    g_st.running = defender_running();
    g_st.building = g_building != 0;
}

static int full_off(void) { return g_st.n_off == NSVCS && g_st.disabled; }
static int full_on(void)  { return g_st.n_def == NSVCS && !g_st.disabled; }

/* ---- GUI -------------------------------------------------------------- */

/* 只在狀態有變化時重畫，避免計時器輪詢造成閃爍 */
static void refresh(HWND hwnd) {
    State prev = g_st;
    read_state();
    if (memcmp(&prev, &g_st, sizeof(State)) == 0 && prev.n_off + prev.n_def > 0) return;

    EnableWindow(g_hDisable, !g_st.building && g_st.pending != 1 && !(g_st.pending == 0 && full_off()));
    EnableWindow(g_hEnable,  !g_st.building && g_st.pending != 2 && !(g_st.pending == 0 && full_on()));
    SetWindowTextW(g_hTamper, g_st.tp_on ? TR(S_BTN_TAMPER_ON) : TR(S_BTN_TAMPER_OFF));
    InvalidateRect(hwnd, NULL, TRUE);
}

/* 設定視窗標題與各按鈕文字成目前語言；WM_CREATE 與切換語言時都呼叫 */
static void apply_lang(HWND hwnd) {
    SetWindowTextW(hwnd, TR(S_WINTITLE));
    SetWindowTextW(g_hDisable, TR(S_BTN_DISABLE));
    SetWindowTextW(g_hEnable, TR(S_BTN_ENABLE));
    SetWindowTextW(g_hTamper, g_st.tp_on ? TR(S_BTN_TAMPER_ON) : TR(S_BTN_TAMPER_OFF));
    SetWindowTextW(g_hLang, TR(S_BTN_LANG));
}

static void open_tamper(HWND hwnd) {
    if (!g_st.tp_on) {
        MessageBoxW(hwnd, TR(S_TAMPER_ALREADY_OFF), TR(S_TITLE_TAMPER), MB_ICONINFORMATION);
        return;
    }
    /* 竄改防護無法由程式直接關閉（微軟鎖定），只能跳到設定頁手動切換。
       shell32 很肥（會多載一串 DLL），只有這裡用到才載入 */
    typedef HINSTANCE (WINAPI *ShellExec)(HWND, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, INT);
    HMODULE sh = LoadLibraryW(L"shell32.dll");
    ShellExec se = sh ? (ShellExec)(void *)GetProcAddress(sh, "ShellExecuteW") : NULL;
    if (se) se(NULL, L"open", L"windowsdefender://threatsettings", NULL, NULL, SW_SHOWNORMAL);
    MessageBoxW(hwnd, TR(S_TAMPER_OPENED), TR(S_TITLE_TAMPER), MB_ICONINFORMATION);
    refresh(hwnd);
}

static void paint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd, &ps);
    RECT rc; GetClientRect(hwnd, &rc);

    const wchar_t *title, *sub;
    COLORREF bg;
    if (g_st.building) {
        bg = RGB(205,125,25); title = TR(S_BUILDING_TITLE);
        sub = TR(S_BUILDING_SUB);
    } else if (g_st.pending == 1) {
        bg = RGB(205,125,25); title = TR(S_PEND_DISABLE_TITLE);
        sub = TR(S_PEND_SUB);
    } else if (g_st.pending == 2) {
        bg = RGB(205,125,25); title = TR(S_PEND_RESTORE_TITLE);
        sub = TR(S_PEND_SUB);
    } else if (full_off()) {
        bg = g_st.running ? RGB(205,125,25) : RGB(200,55,55);
        title = g_st.running ? TR(S_PEND_DISABLE_TITLE) : TR(S_FULLOFF_TITLE);
        sub = g_st.running ? TR(S_FULLOFF_SUB_PENDING) : TR(S_FULLOFF_SUB_DONE);
    } else if (full_on()) {
        bg = g_st.running ? RGB(40,160,80) : RGB(205,125,25);
        title = g_st.running ? TR(S_RUNNING_TITLE) : TR(S_PEND_RESTORE_TITLE);
        sub = g_st.running ? TR(S_RUNNING_SUB) : TR(S_FULLON_SUB_PENDING);
    } else {
        bg = RGB(205,125,25); title = TR(S_PARTIAL_TITLE);
        sub = g_st.running ? TR(S_PARTIAL_SUB_RUNNING) : TR(S_PARTIAL_SUB_OTHER);
    }
    HBRUSH b = CreateSolidBrush(bg);
    FillRect(dc, &rc, b);
    DeleteObject(b);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255,255,255));

    /* title */
    RECT t = rc; t.top = S(26); t.bottom = S(90);
    HFONT old = (HFONT)SelectObject(dc, g_titleFont);
    DrawTextW(dc, title, -1, &t, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    SelectObject(dc, old);

    /* sub line */
    old = (HFONT)SelectObject(dc, g_uiFont);
    RECT s = rc; s.top = S(92); s.bottom = S(118);
    DrawTextW(dc, sub, -1, &s, DT_CENTER | DT_SINGLELINE);

    /* tamper warning */
    if (g_st.tp_on) {
        SetTextColor(dc, RGB(255,235,140));
        RECT w = rc; w.top = S(120); w.bottom = S(150);
        DrawTextW(dc, TR(S_TAMPER_WARN), -1, &w, DT_CENTER | DT_SINGLELINE);
    }
    SelectObject(dc, old);

    EndPaint(hwnd, &ps);
}

/* 動作（含做映像）完成後的提示 */
static void finish_action(HWND hwnd, int disable) {
    const wchar_t *verb = TR(disable ? S_VERB_DISABLE : S_VERB_ENABLE);
    wchar_t msg[700];
    refresh(hwnd);
    if (g_offline == OFF_MISSING || g_offline == OFF_FAILED) {
        wsprintfW(msg, TR(S_FMT_SVC_NOT_SCHEDULED), verb,
            TR(g_offline == OFF_MISSING ? S_IMG_NOT_READY : S_SCHED_FAILED));
        MessageBoxW(hwnd, msg, TR(S_TITLE_DEFSWITCH), MB_ICONWARNING);
        return;
    }
    int need_reboot = (g_offline == OFF_ARMED) || (disable ? g_st.running : !g_st.running);
    if (!need_reboot) {
        wsprintfW(msg, TR(S_FMT_DONE_NOREBOOT), verb);
        MessageBoxW(hwnd, msg, TR(S_TITLE_DEFSWITCH), MB_ICONINFORMATION);
        return;
    }
    wsprintfW(msg, TR(S_FMT_REBOOT_ASK), verb,
        TR(g_offline == OFF_ARMED ? S_REST_ARMED : S_REST_DONE));
    if (MessageBoxW(hwnd, msg, TR(S_TITLE_DEFSWITCH),
                    MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
        run_sys32(L"shutdown.exe", L"/r /t 0");
}

static void do_action(HWND hwnd, int disable) {
    HCURSOR oc = SetCursor(LoadCursor(NULL, IDC_WAIT));
    if (disable) act_disable(); else act_enable();
    SetCursor(oc);
    refresh(hwnd);

    if (g_policy_fail) {
        /* 設定寫入失敗才真的可能是竄改防護 */
        wchar_t msg[300];
        wsprintfW(msg, TR(S_FMT_WRITE_FAIL), g_policy_fail);
        if (g_offline == OFF_ARMED) cancel_offline();
        refresh(hwnd);
        MessageBoxW(hwnd, msg, TR(S_TITLE_DEFSWITCH), MB_ICONWARNING);
        return;
    }
    wchar_t ask_msg[600];
    wsprintfW(ask_msg, TR(S_ASK_BUILD_IMAGE), g_off_dir);
    if (g_offline == OFF_MISSING &&
        MessageBoxW(hwnd, ask_msg,
            TR(S_TITLE_DEFSWITCH), MB_YESNO | MB_ICONQUESTION) == IDYES) {
        g_build_disable = disable;
        InterlockedExchange(&g_building, 1);
        HANDLE t = CreateThread(NULL, 0, build_thread, hwnd, 0, NULL);
        if (t) { CloseHandle(t); refresh(hwnd); return; }
        InterlockedExchange(&g_building, 0);
    }
    finish_action(hwnd, disable);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT m, WPARAM wp, LPARAM lp) {
    switch (m) {
    case WM_CREATE: {
        g_dpi = (int)GetDpiForWindow(hwnd);
        if (g_dpi <= 0) g_dpi = 96;

        g_titleFont = CreateFontW(S(34),0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
            DEFAULT_PITCH,L"Microsoft JhengHei UI");
        g_uiFont = CreateFontW(S(17),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,
            DEFAULT_PITCH,L"Microsoft JhengHei UI");

        g_hDisable = CreateWindowW(L"BUTTON", L"",
            WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
            S(40),S(160),S(150),S(44), hwnd,(HMENU)IDC_DISABLE,NULL,NULL);
        g_hEnable = CreateWindowW(L"BUTTON", L"",
            WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
            S(230),S(160),S(150),S(44), hwnd,(HMENU)IDC_ENABLE,NULL,NULL);
        g_hTamper = CreateWindowW(L"BUTTON", L"",
            WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
            S(40),S(216),S(340),S(40), hwnd,(HMENU)IDC_TAMPER,NULL,NULL);
        g_hLang = CreateWindowW(L"BUTTON", L"",
            WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
            S(330),S(8),S(60),S(24), hwnd,(HMENU)IDC_LANG,NULL,NULL);
        SendMessageW(g_hDisable, WM_SETFONT,(WPARAM)g_uiFont,TRUE);
        SendMessageW(g_hEnable,  WM_SETFONT,(WPARAM)g_uiFont,TRUE);
        SendMessageW(g_hTamper,  WM_SETFONT,(WPARAM)g_uiFont,TRUE);
        SendMessageW(g_hLang,    WM_SETFONT,(WPARAM)g_uiFont,TRUE);
        apply_lang(hwnd);
        reconcile_pending();
        /* 即時偵測狀態變化；允許系統把喚醒合併，最小化時不查 */
        SetCoalescableTimer(hwnd, 1, 1000, NULL, 250);
        refresh(hwnd);
        return 0;
    }
    case WM_TIMER:
        if (!IsIconic(hwnd)) refresh(hwnd);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wp) == IDC_DISABLE) do_action(hwnd, 1);
        else if (LOWORD(wp) == IDC_ENABLE) do_action(hwnd, 0);
        else if (LOWORD(wp) == IDC_TAMPER) open_tamper(hwnd);
        else if (LOWORD(wp) == IDC_LANG) {
            g_lang = g_lang ? L_EN : L_ZH;
            apply_lang(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    case WM_APP_BUILT:
        refresh(hwnd);
        if (!wp) {
            {
                wchar_t fm[300];
                wsprintfW(fm, TR(S_BUILD_FAILED), g_off_dir);
                MessageBoxW(hwnd, fm, TR(S_TITLE_DEFSWITCH), MB_ICONWARNING);
            }
            return 0;
        }
        finish_action(hwnd, g_build_disable);
        return 0;
    case WM_CLOSE:
        if (g_building) {
            MessageBoxW(hwnd, TR(S_BUILDING_CLOSE_WARN),
                        TR(S_TITLE_DEFSWITCH), MB_ICONINFORMATION);
            return 0;
        }
        break;
    case WM_PAINT: paint(hwnd); return 0;
    case WM_CTLCOLORBTN: return (LRESULT)GetStockObject(NULL_BRUSH);
    case WM_DESTROY: KillTimer(hwnd, 1); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, m, wp, lp);
}

void start(void) {
    HINSTANCE hInst = GetModuleHandleW(NULL);

    /* 解析離線資料夾（環境變數 DEFENDER_OFFLINE_DIR，否則預設 C:\DefenderOffline）。
       必須在任何離線函式（reconcile_pending 等）之前做。 */
    resolve_off_dir();

    /* 開機自動偵測語言：系統 UI 語言主語言碼是中文就用繁中，否則英文 */
    LANGID uilang = GetUserDefaultUILanguage();
    g_lang = (PRIMARYLANGID(uilang) == LANG_CHINESE) ? L_ZH : L_EN;

    /* 視窗沒有文字輸入；關掉輸入法就不會載入 TSF 那一串 DLL */
    ImmDisableIME((DWORD)-1);

    g_dpi = (int)GetDpiForSystem();
    if (g_dpi <= 0) g_dpi = 96;

    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(1));
    wc.lpszClassName = L"DefenderToggleWnd";
    RegisterClassW(&wc);

    int W = S(420), H = S(310);
    RECT r = {0,0,W,H};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, FALSE);
    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    int ww = r.right-r.left, wh = r.bottom-r.top;

    HWND hwnd = CreateWindowW(L"DefenderToggleWnd", TR(S_WINTITLE),
        (WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX),
        (sw-ww)/2, (sh-wh)/2, ww, wh, NULL, NULL, hInst, NULL);
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    ExitProcess(0);
}
