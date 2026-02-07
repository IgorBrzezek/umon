/*
 * umon_win.c - System Monitor for Windows
 * 
 * COMPILATION INSTRUCTIONS FOR WINDOWS:
 * 
 * Using MinGW-w64 (recommended):
 * --------------------------------
 * 1. Install MinGW-w64 from: https://www.mingw-w64.org/
 *    Or use MSYS2: https://www.msys2.org/
 * 
 * 2. Open MinGW-w64 terminal or MSYS2 terminal
 * 
 * 3. Compile with:
 *    gcc -O2 -Wall -o umon.exe umon_win.c -liphlpapi -lpsapi -lnetapi32 -ladvapi32
 * 
 * 4. Static linking (portable, no DLL dependencies):
 *    gcc -O2 -Wall -static -o umon.exe umon_win.c -liphlpapi -lpsapi -lnetapi32 -ladvapi32
 * 
 * Using Visual Studio:
 * -------------------
 * 1. Open "Developer Command Prompt for VS"
 * 
 * 2. Compile with:
 *    cl /O2 /W3 umon_win.c iphlpapi.lib psapi.lib netapi32.lib advapi32.lib
 * 
 * 3. For static linking:
 *    cl /O2 /W3 /MT umon_win.c iphlpapi.lib psapi.lib netapi32.lib advapi32.lib
 * 
 * Using Clang on Windows:
 * ----------------------
 * clang -O2 -Wall -o umon.exe umon_win.c -liphlpapi -lpsapi -lnetapi32 -ladvapi32
 * 
 * Cross-compilation from Linux:
 * ----------------------------
 * Install mingw-w64:
 *   sudo apt-get install mingw-w64
 * 
 * Compile for Windows 64-bit:
 *   x86_64-w64-mingw32-gcc -O2 -Wall -o umon.exe umon_win.c -liphlpapi -lpsapi -lnetapi32 -ladvapi32
 * 
 * Compile for Windows 32-bit:
 *   i686-w64-mingw32-gcc -O2 -Wall -o umon32.exe umon_win.c -liphlpapi -lpsapi -lnetapi32 -ladvapi32
 * 
 * or
 * gcc -O2 -Wall -o umon.exe umon_win.c -liphlpapi -lpsapi -lnetapi32 -ladvapi32
 * or
 * gcc -O2 -Wall -static -o umon.exe umon_win.c -liphlpapi -lpsapi -lnetapi32 -ladvapi32 -lws2_32
 *
 * USAGE:
 *   umon.exe [options]
 *   umon.exe --cpu --mem --log data.csv
 *   umon.exe --interval 1000 --log monitoring.csv
 * 
 * AUTHOR: Igor Brzezek
 * DATE: 21.01.2026
 */
/*
 * umon_win.c - System Monitor for Windows (No-Flicker & Fixed CLI)
 * * gcc -O2 -Wall -o umon.exe umon_win.c -liphlpapi -lpsapi -lnetapi32 -ladvapi32
 */

#define _CRT_SECURE_NO_WARNINGS
#define _WIN32_WINNT 0x0601
#include <winsock2.h>
#include <ws2tcpip.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iphlpapi.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

/* Linkowanie dla MSVC (GCC zignoruje te pragma, co jest normalne) */
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ws2_32.lib")

#define VERSION "0.0.4"
#define AUTHOR "Igor Brzezek"
#define DATE "04.02.2026"
#define AUTHOR_GITHUB "github.com/igorbrzezek"

/* Struktury i zmienne globalne */
typedef struct { ULONGLONG idle, kernel, user; } cpu_times_t;
typedef struct { char name[128]; unsigned long long bytes_sent, bytes_recv; } net_stats_t;

int opt_cpu = 0, opt_mem = 0, opt_disks = 0, opt_net_all = 0, opt_cpulist = 0, opt_mono = 0;
int opt_interval = 250;
char *opt_net_iface = NULL;
int num_cores = 0, net_prev_count = 0;
cpu_times_t *prev_per_core_times = NULL, prev_total_cpu;
net_stats_t net_prev[32];
LARGE_INTEGER net_prev_time;
HANDLE hConsole;
DWORD originalMode;

/* --- Zarządzanie konsolą --- */

void show_cursor(BOOL show) {
    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(hConsole, &ci);
    ci.bVisible = show;
    SetConsoleCursorInfo(hConsole, &ci);
}

void cleanup(void) {
    show_cursor(TRUE);
    if (hConsole != INVALID_HANDLE_VALUE) SetConsoleMode(hConsole, originalMode);
}

BOOL WINAPI ConsoleHandler(DWORD dwType) {
    cleanup();
    return FALSE;
}

void enableAnsi(void) {
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) return;
    GetConsoleMode(hConsole, &originalMode);
    SetConsoleMode(hConsole, originalMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    atexit(cleanup);
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
}

void set_cursor_position(int x, int y) {
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(hConsole, coord);
}

const char* c_red()     { return opt_mono ? "" : "\033[91m"; }
const char* c_green()   { return opt_mono ? "" : "\033[92m"; }
const char* c_yellow()  { return opt_mono ? "" : "\033[93m"; }
const char* c_blue()    { return opt_mono ? "" : "\033[94m"; }
const char* c_magenta() { return opt_mono ? "" : "\033[95m"; }
const char* c_cyan()    { return opt_mono ? "" : "\033[96m"; }
const char* c_white()   { return opt_mono ? "" : "\033[97m"; }
const char* c_bold()    { return opt_mono ? "" : "\033[1m"; }
const char* c_dim()     { return opt_mono ? "" : "\033[2m"; }
const char* c_reset()   { return opt_mono ? "" : "\033[0m"; }

/* --- Pomoc --- */

void print_h(void) {
    printf("Info: umon v%s by %s (%s). [%s]\n", VERSION, AUTHOR, DATE, AUTHOR_GITHUB);
    printf("Usage: umon [--cpu] [--mem] [--disks] [--net [IFACE]] [--mono] [--interval MS] [--sysinfo] | -h | --help\n");
}

void print_help(void) {
    print_h();
    printf("\nDetailed Options:\n");
    printf("  --cpu                Display CPU usage (total and per-core grid)\n");
    printf("  --mem                Display RAM and Swap (Pagefile) usage\n");
    printf("  --disks              Display fixed disk drives usage\n");
    printf("  --net [IFACE]        Display network speed (optional: filter by name)\n");
    printf("  --cpulist            Show CPU cores as a vertical list\n");
    printf("  --interval MS        Refresh interval in milliseconds (default 250)\n");
    printf("  --sysinfo            Display system hardware info and exit\n");
    printf("  --mono               Disable ANSI colors\n");
    printf("  -h                   Show short info and one-line usage\n");
    printf("  --help               Show this full help message\n");
}

/* --- Logika zbierania danych --- */

void format_bytes(double bytes, char *buffer, size_t size) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0; while (bytes >= 1024 && i < 4) { bytes /= 1024; i++; }
    snprintf(buffer, size, "%.1f %s", bytes, units[i]);
}

void draw_bar_ascii(double val, double max, int w, char *buf, size_t sz) {
    double pct = (val/max)*100.0; if(pct>100) pct=100; if(pct<0) pct=0;
    int fill = (int)((pct/100.0)*w);
    const char *col = (pct < 33) ? c_green() : (pct < 66 ? c_yellow() : c_red());
    char bar[128]; char *p = bar;
    p += sprintf(p, "%s", col); for (int i=0; i<fill; i++) *p++ = '#';
    p += sprintf(p, "%s%s", c_reset(), c_white()); for (int i=0; i<w-fill; i++) *p++ = '-';
    p += sprintf(p, "%s", c_reset()); *p = '\0';
    snprintf(buf, sz, "%s[%s]%s %5.1f%%", c_cyan(), bar, c_reset(), pct);
}

typedef struct { LARGE_INTEGER i, k, u, d, it; ULONG ic; } SPI;
typedef NTSTATUS (WINAPI *PNQSI)(ULONG, PVOID, ULONG, PULONG);
static PNQSI nqsi = NULL;

void init_cpu_stats(void) {
    SYSTEM_INFO si; GetSystemInfo(&si); num_cores = si.dwNumberOfProcessors;
    prev_per_core_times = calloc(num_cores, sizeof(cpu_times_t));
    nqsi = (PNQSI)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQuerySystemInformation");
    FILETIME id, ke, us; GetSystemTimes(&id, &ke, &us);
    prev_total_cpu.idle = ((ULONGLONG)id.dwHighDateTime << 32) | id.dwLowDateTime;
    prev_total_cpu.kernel = ((ULONGLONG)ke.dwHighDateTime << 32) | ke.dwLowDateTime;
    prev_total_cpu.user = ((ULONGLONG)us.dwHighDateTime << 32) | us.dwLowDateTime;
}

double calc_cpu(cpu_times_t *c, cpu_times_t *p) {
    ULONGLONG tD = (c->kernel + c->user) - (p->kernel + p->user);
    ULONGLONG iD = c->idle - p->idle;
    return tD > 0 ? (double)(tD - iD) / tD * 100.0 : 0.0;
}

void get_cpu_info(int bw, int lst) {
    FILETIME id, ke, us; GetSystemTimes(&id, &ke, &us);
    cpu_times_t curT = { (((ULONGLONG)id.dwHighDateTime << 32) | id.dwLowDateTime),
                         (((ULONGLONG)ke.dwHighDateTime << 32) | ke.dwLowDateTime),
                         (((ULONGLONG)us.dwHighDateTime << 32) | us.dwLowDateTime) };
    char b[256]; draw_bar_ascii(calc_cpu(&curT, &prev_total_cpu), 100, bw, b, 256);
    printf("%sCPU%s (%d cores): %s\n", c_blue(), c_reset(), num_cores, b);
    if (nqsi && prev_per_core_times) {
        SPI *spi = (SPI*)calloc(num_cores, sizeof(SPI));
        if (nqsi(8, spi, num_cores * sizeof(SPI), NULL) == 0) {
            for (int i=0; i<num_cores; i++) {
                cpu_times_t c = { (ULONGLONG)spi[i].i.QuadPart, (ULONGLONG)spi[i].k.QuadPart, (ULONGLONG)spi[i].u.QuadPart };
                double p = calc_cpu(&c, &prev_per_core_times[i]); prev_per_core_times[i] = c;
                draw_bar_ascii(p, 100, bw, b, 256);
                if (lst) printf("  Core %2d: %s\n", i, b);
                else { printf("  #%2d:%s ", i, b); if((i+1)%2==0) printf("\n"); }
            }
        } free(spi);
    } prev_total_cpu = curT;
}

void get_memory_info(int bw) {
    MEMORYSTATUSEX m; m.dwLength = sizeof(m); GlobalMemoryStatusEx(&m);
    char b[256], b1[32], b2[32]; 
    
    // RAM
    double uR = (double)(m.ullTotalPhys - m.ullAvailPhys);
    draw_bar_ascii(uR, (double)m.ullTotalPhys, bw, b, 256);
    format_bytes(uR, b1, 32); format_bytes((double)m.ullTotalPhys, b2, 32);
    printf("RAM:   %s %s/%s\n", b, b1, b2);
    
    // Swap (Pagefile)
    double uS = (double)(m.ullTotalPageFile - m.ullAvailPageFile);
    draw_bar_ascii(uS, (double)m.ullTotalPageFile, bw, b, 256);
    format_bytes(uS, b1, 32); format_bytes((double)m.ullTotalPageFile, b2, 32);
    printf("Swap:  %s %s/%s\n", b, b1, b2);
}

void get_disk_info(int bw) {
    DWORD d = GetLogicalDrives();
    for (int i=0; i<26; i++) {
        if (d & (1 << i)) {
            char p[4] = {(char)('A'+i), ':', '\\', '\0'};
            if (GetDriveTypeA(p) == DRIVE_FIXED) {
                ULARGE_INTEGER f, t; if (GetDiskFreeSpaceExA(p, &f, &t, NULL)) {
                    char b[256], b1[32], b2[32];
                    draw_bar_ascii((double)(t.QuadPart - f.QuadPart), (double)t.QuadPart, bw, b, 256);
                    format_bytes((double)(t.QuadPart - f.QuadPart), b1, 32); format_bytes((double)t.QuadPart, b2, 32);
                    printf("%s%c:%s   %s %s/%s\n", c_cyan(), p[0], c_reset(), b, b1, b2);
                }
            }
        }
    }
}

void get_net_info(int bw) {
    MIB_IF_TABLE2 *tbl = NULL;
    if (GetIfTable2(&tbl) == NO_ERROR) {
        LARGE_INTEGER cur, frq; QueryPerformanceCounter(&cur); QueryPerformanceFrequency(&frq);
        double dt = net_prev_time.QuadPart > 0 ? (double)(cur.QuadPart - net_prev_time.QuadPart) / frq.QuadPart : 0;
        for (DWORD i = 0; i < tbl->NumEntries; i++) {
            MIB_IF_ROW2 *r = &tbl->Table[i];
            if (r->Type == IF_TYPE_SOFTWARE_LOOPBACK || r->OperStatus != IfOperStatusUp) continue;
            char n[128]; WideCharToMultiByte(CP_UTF8, 0, r->Description, -1, n, 128, NULL, NULL);
            if (opt_net_iface && !strstr(n, opt_net_iface)) continue;
            if (dt > 0) {
                for (int j = 0; j < net_prev_count; j++) {
                    if (strcmp(net_prev[j].name, n) == 0) {
                        char b1[32], b2[32];
                        double rx = (double)(r->InOctets - net_prev[j].bytes_recv) / dt;
                        double tx = (double)(r->OutOctets - net_prev[j].bytes_sent) / dt;
                        format_bytes(rx, b1, 32); format_bytes(tx, b2, 32);
                        printf("%sNET:%s  %-20s %sDN: %s/s %sUP: %s/s\n", c_magenta(), c_reset(), n, c_dim(), b1, c_dim(), b2);
                        break;
                    }
                }
            }
            int k; for (k = 0; k < net_prev_count; k++) if (strcmp(net_prev[k].name, n) == 0) break;
            if (k == net_prev_count && net_prev_count < 32) { strncpy(net_prev[k].name, n, 127); net_prev_count++; }
            net_prev[k].bytes_recv = r->InOctets; net_prev[k].bytes_sent = r->OutOctets;
        }
        net_prev_time = cur; FreeMibTable(tbl);
    }
}

/* --- Main --- */

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) { print_h(); return 0; }
        else if (strcmp(argv[i], "--help") == 0) { print_help(); return 0; }
        else if (strcmp(argv[i], "--cpu") == 0) opt_cpu = 1;
        else if (strcmp(argv[i], "--mem") == 0) opt_mem = 1;
        else if (strcmp(argv[i], "--disks") == 0) opt_disks = 1;
        else if (strcmp(argv[i], "--cpulist") == 0) opt_cpulist = 1;
        else if (strcmp(argv[i], "--mono") == 0) opt_mono = 1;
        else if (strcmp(argv[i], "--sysinfo") == 0) { 
            enableAnsi(); 
            printf("%sSystem Information Mode...%s\n", c_bold(), c_reset()); 
            /* Tutaj mozna dodac dodatkowe wyswietlanie sysinfo */
            return 0; 
        }
        else if (strcmp(argv[i], "--interval") == 0 && i+1 < argc) opt_interval = atoi(argv[++i]);
        else if (strcmp(argv[i], "--net") == 0) {
             opt_net_all = 1; 
             if (i+1 < argc && argv[i+1][0] != '-') opt_net_iface = argv[++i];
        }
        else {
            fprintf(stderr, "\033[91mError: Unknown option '%s'\033[0m\n\n", argv[i]);
            print_h(); return 1;
        }
    }

    enableAnsi();
    show_cursor(FALSE);
    init_cpu_stats();
    
    int any_opt = opt_cpu || opt_mem || opt_disks || opt_cpulist || opt_net_all;
    int show_cpu_stats = opt_cpu || opt_cpulist || !any_opt;
    int show_mem_stats = opt_mem || !any_opt;
    int show_disk_stats = opt_disks || !any_opt;
    int show_net_stats = opt_net_all; // Pokazujemy siec tylko jesli jawnie wywolana

    system("cls");

    while (1) {
        set_cursor_position(0, 0);
        time_t t = time(NULL); char ts[16]; strftime(ts, 16, "%H:%M:%S", localtime(&t));

        printf("%s============================================================%s\n", c_magenta(), c_reset());
        printf("%s%s       SYSTEM MONITOR v%s          %s%s%s\n", c_bold(), c_cyan(), VERSION, c_white(), ts, c_reset());
        printf("%s============================================================%s\n", c_magenta(), c_reset());

        if (show_cpu_stats)   { printf("\n"); get_cpu_info(30, opt_cpulist); }
        if (show_mem_stats)   { printf("\n"); get_memory_info(30); }
        if (show_disk_stats)  { printf("\n"); get_disk_info(30); }
        if (show_net_stats)   { printf("\n"); get_net_info(30); }
        
        printf("\n%sPress Ctrl+C to exit...%s\n", c_dim(), c_reset());
        fflush(stdout);

        if (_kbhit()) { int ch = _getch(); if (ch == 3) break; }
        Sleep(opt_interval);
    }

    return 0;
}
