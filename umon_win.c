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
 * USAGE:
 *   umon.exe [options]
 *   umon.exe --cpu --mem --log data.csv
 *   umon.exe --interval 1000 --log monitoring.csv
 * 
 * AUTHOR: Igor Brzezek
 * DATE: 21.01.2026
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

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <conio.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "advapi32.lib")

/* Constants and Macros */
#define VERSION "0.0.3"
#define AUTHOR "Igor Brzezek"
#define AUTHOR_GITHUB "github.com/igorbrzezek" 
#define DATE "04.02.2026"

#define RESET   ""
#define RED     ""
#define GREEN   ""
#define YELLOW  ""
#define BLUE    ""
#define MAGENTA ""
#define CYAN    ""
#define WHITE   ""
#define BOLD    ""
#define DIM     ""

/* State Structures */
typedef struct {
    ULONGLONG idle;
    ULONGLONG kernel;
    ULONGLONG user;
} cpu_times_t;

typedef struct {
    char name[128];
    unsigned long long bytes_sent;
    unsigned long long bytes_recv;
} net_stats_t;

/* Global Options */
int opt_cpu = 0;
int opt_mem = 0;
int opt_disks = 0;
char *opt_net_iface = NULL;
int opt_net_all = 0;
int opt_cpulist = 0;
int opt_mono = 0;
int opt_interval = 250;
char *opt_log = NULL;
FILE *log_fp = NULL;
int log_header_written = 0;

/* CPU tracking */
int num_cores = 0;
cpu_times_t *prev_cpu_times = NULL;
cpu_times_t prev_total_cpu;

/* Network tracking */
#define MAX_IFACES 32
net_stats_t net_prev[MAX_IFACES];
int net_prev_count = 0;
LARGE_INTEGER net_prev_time;

/* Console handling */
HANDLE hConsole;
DWORD originalMode;

void enableAnsiOnWindows(void) {
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) return;
    
    GetConsoleMode(hConsole, &originalMode);
    DWORD newMode = originalMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, newMode);
}

void restoreConsole(void) {
    if (hConsole != INVALID_HANDLE_VALUE) {
        SetConsoleMode(hConsole, originalMode);
    }
}

const char* c_red(void) { return opt_mono ? "" : "\033[91m"; }
const char* c_green(void) { return opt_mono ? "" : "\033[92m"; }
const char* c_yellow(void) { return opt_mono ? "" : "\033[93m"; }
const char* c_blue(void) { return opt_mono ? "" : "\033[94m"; }
const char* c_magenta(void) { return opt_mono ? "" : "\033[95m"; }
const char* c_cyan(void) { return opt_mono ? "" : "\033[96m"; }
const char* c_white(void) { return opt_mono ? "" : "\033[97m"; }
const char* c_bold(void) { return opt_mono ? "" : "\033[1m"; }
const char* c_dim(void) { return opt_mono ? "" : "\033[2m"; }
const char* c_reset(void) { return opt_mono ? "" : "\033[0m"; }

void format_bytes(double bytes, char *buffer, size_t size) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int i = 0;
    while (bytes >= 1024 && i < 5) {
        bytes /= 1024;
        i++;
    }
    snprintf(buffer, size, "%.1f %s", bytes, units[i]);
}

const char* get_color_for_percentage(double pct) {
    if (opt_mono) return "";
    if (pct < 33.0) return c_green();
    if (pct < 66.0) return c_yellow();
    return c_red();
}

void draw_bar_ascii(double value, double max_val, int width, char *buffer, size_t size) {
    if (max_val == 0) max_val = 1;
    double pct = (value / max_val) * 100.0;
    if (pct > 100.0) pct = 100.0;
    if (pct < 0) pct = 0;
    
    int filled = (int)((pct / 100.0) * width);
    if (filled > width) filled = width;
    
    char bar[256];
    char *p = bar;
    
    const char *col = get_color_for_percentage(pct);
    strcpy(p, col); p += strlen(col);
    for (int i = 0; i < filled; i++) *p++ = '#';
    strcpy(p, c_reset()); p += strlen(c_reset());
    
    strcpy(p, c_white()); p += strlen(c_white());
    for (int i = 0; i < width - filled; i++) *p++ = '-';
    strcpy(p, c_reset()); p += strlen(c_reset());
    *p = '\0';
    
    snprintf(buffer, size, "%s[%s%s%s]%s %.1f%%", 
             c_cyan(), c_reset(), bar, c_cyan(), c_reset(), pct);
}

/* System Info */
void display_sysinfo(void) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    GetComputerNameA(computerName, &size);
    
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    
    printf("%sSystem Information:%s\n", c_bold(), c_reset());
    printf("  Computer Name: %s\n", computerName);
    printf("  OS Version: Windows %lu.%lu\n", osvi.dwMajorVersion, osvi.dwMinorVersion);
    printf("  Architecture: %s\n", 
           si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "x64" :
           si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ? "x86" : "Other");
    
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    
    printf("\n%sCPU Information:%s\n", c_bold(), c_reset());
    printf("  Number of Processors: %lu\n", si.dwNumberOfProcessors);
    
    SYSTEM_INFO sysInfo;
    GetNativeSystemInfo(&sysInfo);
    
    printf("\n%sMemory Information:%s\n", c_bold(), c_reset());
    
    double totalGB = (double)memInfo.ullTotalPhys / (1024 * 1024 * 1024);
    double availGB = (double)memInfo.ullAvailPhys / (1024 * 1024 * 1024);
    double usedGB = totalGB - availGB;
    double pct = (usedGB / totalGB) * 100.0;
    
    char buf1[32], buf2[32], buf3[32];
    format_bytes(memInfo.ullTotalPhys, buf1, sizeof(buf1));
    format_bytes(memInfo.ullAvailPhys, buf2, sizeof(buf2));
    format_bytes(memInfo.ullTotalPhys - memInfo.ullAvailPhys, buf3, sizeof(buf3));
    
    printf("  Total: %s\n", buf1);
    printf("  Available: %s\n", buf2);
    printf("  Used: %s\n", buf3);
    printf("  Percentage: %.2f%%\n", pct);
    
    printf("\n%sMemory Load:%s %lu%%\n", c_bold(), c_reset(), memInfo.dwMemoryLoad);
    
    exit(0);
}

/* Network List */
void display_netlist(void) {
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
    ULONG family = AF_UNSPEC;
    
    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    ULONG outBufLen = 0;
    
    dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
    
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);
        if (pAddresses == NULL) {
            printf("Memory allocation failed\n");
            exit(1);
        }
        dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
    }
    
    if (dwRetVal == NO_ERROR) {
        PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
        
        printf("%sNetwork Interfaces:%s\n", c_bold(), c_reset());
        
        while (pCurrAddresses) {
            printf("\n%s  Interface: %s%ls%s\n", c_blue(), c_reset(), 
                   pCurrAddresses->FriendlyName, c_reset());
            printf("    Description: %ls\n", pCurrAddresses->Description);
            printf("    MAC Address: ");
            
            if (pCurrAddresses->PhysicalAddressLength != 0) {
                for (int i = 0; i < (int)pCurrAddresses->PhysicalAddressLength; i++) {
                    if (i == (pCurrAddresses->PhysicalAddressLength - 1))
                        printf("%.2X\n", (int)pCurrAddresses->PhysicalAddress[i]);
                    else
                        printf("%.2X:", (int)pCurrAddresses->PhysicalAddress[i]);
                }
            } else {
                printf("N/A\n");
            }
            
            printf("    Status: %s\n", 
                   pCurrAddresses->OperStatus == IfOperStatusUp ? "Up" : "Down");
            printf("    Type: ");
            switch (pCurrAddresses->IfType) {
                case IF_TYPE_ETHERNET_CSMACD: printf("Ethernet\n"); break;
                case IF_TYPE_ISO88025_TOKENRING: printf("Token Ring\n"); break;
                case IF_TYPE_PPP: printf("PPP\n"); break;
                case IF_TYPE_SOFTWARE_LOOPBACK: printf("Loopback\n"); break;
                case IF_TYPE_ATM: printf("ATM\n"); break;
                case IF_TYPE_IEEE80211: printf("Wireless\n"); break;
                case IF_TYPE_TUNNEL: printf("Tunnel\n"); break;
                case IF_TYPE_IEEE1394: printf("Firewire\n"); break;
                default: printf("Other (%lu)\n", pCurrAddresses->IfType);
            }
            printf("    MTU: %lu\n", pCurrAddresses->Mtu);
            printf("    Speed: %I64u Mbps\n", pCurrAddresses->TransmitLinkSpeed / 1000000);
            
            PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
            if (pUnicast) {
                for (int i = 0; pUnicast != NULL; i++) {
                    char addrStr[INET6_ADDRSTRLEN];
                    DWORD addrStrLen = INET6_ADDRSTRLEN;
                    
                    struct sockaddr *sa = pUnicast->Address.lpSockaddr;
                    if (sa->sa_family == AF_INET) {
                        struct sockaddr_in *sin = (struct sockaddr_in *)sa;
                        InetNtopA(AF_INET, &sin->sin_addr, addrStr, addrStrLen);
                        printf("    IPv4 Address: %s\n", addrStr);
                    } else if (sa->sa_family == AF_INET6) {
                        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
                        InetNtopA(AF_INET6, &sin6->sin6_addr, addrStr, addrStrLen);
                        printf("    IPv6 Address: %s\n", addrStr);
                    }
                    pUnicast = pUnicast->Next;
                }
            }
            
            pCurrAddresses = pCurrAddresses->Next;
        }
    } else {
        printf("GetAdaptersAddresses failed with error: %lu\n", dwRetVal);
    }
    
    if (pAddresses) {
        free(pAddresses);
    }
    
    exit(0);
}

/* Per-core CPU tracking */
typedef struct {
    ULONGLONG idle;
    ULONGLONG kernel;
    ULONGLONG user;
} per_core_times_t;

per_core_times_t *prev_per_core_times = NULL;

/* NtQuerySystemInformation for per-core stats */
typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

#define SystemProcessorPerformanceInformation 8

typedef NTSTATUS (WINAPI *PNtQuerySystemInformation)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
);

static PNtQuerySystemInformation NtQuerySystemInformation = NULL;

/* CPU Functions */
void init_cpu_stats(void) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    num_cores = si.dwNumberOfProcessors;
    
    prev_cpu_times = (cpu_times_t *)calloc(num_cores, sizeof(cpu_times_t));
    prev_per_core_times = (per_core_times_t *)calloc(num_cores, sizeof(per_core_times_t));
    
    /* Load NtQuerySystemInformation dynamically */
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (hNtdll) {
        NtQuerySystemInformation = (PNtQuerySystemInformation)GetProcAddress(hNtdll, "NtQuerySystemInformation");
    }
    
    FILETIME idleTime, kernelTime, userTime;
    GetSystemTimes(&idleTime, &kernelTime, &userTime);
    
    prev_total_cpu.idle = ((ULONGLONG)idleTime.dwHighDateTime << 32) | idleTime.dwLowDateTime;
    prev_total_cpu.kernel = ((ULONGLONG)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;
    prev_total_cpu.user = ((ULONGLONG)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;
    
    /* Initialize per-core times */
    if (NtQuerySystemInformation && prev_per_core_times) {
        SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *spi = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *)calloc(num_cores, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));
        if (spi) {
            ULONG returnLength = 0;
            NTSTATUS status = NtQuerySystemInformation(SystemProcessorPerformanceInformation, spi, 
                num_cores * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION), &returnLength);
            
            if (status == 0) {
                for (int i = 0; i < num_cores; i++) {
                    prev_per_core_times[i].idle = (ULONGLONG)spi[i].IdleTime.QuadPart;
                    prev_per_core_times[i].kernel = (ULONGLONG)spi[i].KernelTime.QuadPart;
                    prev_per_core_times[i].user = (ULONGLONG)spi[i].UserTime.QuadPart;
                }
            }
            free(spi);
        }
    }
}

double calculate_cpu_percent(cpu_times_t *curr, cpu_times_t *prev) {
    ULONGLONG currTotal = curr->kernel + curr->user;
    ULONGLONG prevTotal = prev->kernel + prev->user;
    ULONGLONG totalDiff = currTotal - prevTotal;
    ULONGLONG idleDiff = curr->idle - prev->idle;
    
    if (totalDiff == 0) return 0.0;
    
    return ((double)(totalDiff - idleDiff) / totalDiff) * 100.0;
}

void get_per_core_percentages(double *percentages, int num_cores) {
    if (!NtQuerySystemInformation || !prev_per_core_times || !percentages) {
        for (int i = 0; i < num_cores; i++) {
            percentages[i] = 0.0;
        }
        return;
    }
    
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *spi = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *)calloc(num_cores, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION));
    if (!spi) {
        for (int i = 0; i < num_cores; i++) {
            percentages[i] = 0.0;
        }
        return;
    }
    
    ULONG returnLength = 0;
    NTSTATUS status = NtQuerySystemInformation(SystemProcessorPerformanceInformation, spi, 
        num_cores * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION), &returnLength);
    
    if (status == 0) {
        for (int i = 0; i < num_cores; i++) {
            per_core_times_t curr;
            curr.idle = (ULONGLONG)spi[i].IdleTime.QuadPart;
            curr.kernel = (ULONGLONG)spi[i].KernelTime.QuadPart;
            curr.user = (ULONGLONG)spi[i].UserTime.QuadPart;
            
            ULONGLONG currTotal = curr.kernel + curr.user;
            ULONGLONG prevTotal = prev_per_core_times[i].kernel + prev_per_core_times[i].user;
            ULONGLONG totalDiff = currTotal - prevTotal;
            ULONGLONG idleDiff = curr.idle - prev_per_core_times[i].idle;
            
            if (totalDiff == 0) {
                percentages[i] = 0.0;
            } else {
                percentages[i] = ((double)(totalDiff - idleDiff) / totalDiff) * 100.0;
            }
            
            prev_per_core_times[i] = curr;
        }
    } else {
        for (int i = 0; i < num_cores; i++) {
            percentages[i] = 0.0;
        }
    }
    
    free(spi);
}

void get_cpu_info(int bar_width, int cpulist_mode) {
    FILETIME idleTime, kernelTime, userTime;
    GetSystemTimes(&idleTime, &kernelTime, &userTime);
    
    cpu_times_t curr_total;
    curr_total.idle = ((ULONGLONG)idleTime.dwHighDateTime << 32) | idleTime.dwLowDateTime;
    curr_total.kernel = ((ULONGLONG)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;
    curr_total.user = ((ULONGLONG)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;
    
    char buf[256];
    
    double total_pct = calculate_cpu_percent(&curr_total, &prev_total_cpu);
    
    if (cpulist_mode) {
        /* Show each core as a list item */
        double *core_percentages = (double *)calloc(num_cores, sizeof(double));
        if (core_percentages) {
            get_per_core_percentages(core_percentages, num_cores);
            
            for (int i = 0; i < num_cores; i++) {
                draw_bar_ascii(core_percentages[i], 100, bar_width, buf, sizeof(buf));
                printf("%sCPU%s %2d: %s\n", c_blue(), c_reset(), i, buf);
            }
            
            free(core_percentages);
        }
    } else {
        /* Normal mode - show total and per-core grid */
        draw_bar_ascii(total_pct, 100, bar_width, buf, sizeof(buf));
        printf("%sCPU%s (%d cores): %s\n", c_blue(), c_reset(), num_cores, buf);
        
        /* Per-core grid display */
        double *core_percentages = (double *)calloc(num_cores, sizeof(double));
        if (core_percentages) {
            get_per_core_percentages(core_percentages, num_cores);
            
            int cores_per_row = 3;
            for (int row_start = 0; row_start < num_cores; row_start += cores_per_row) {
                int row_end = row_start + cores_per_row;
                if (row_end > num_cores) row_end = num_cores;
                
                for (int i = row_start; i < row_end; i++) {
                    draw_bar_ascii(core_percentages[i], 100, bar_width, buf, sizeof(buf));
                    printf("%s#%2d:%s%s  ", c_white(), i, c_reset(), buf);
                }
                printf("\n");
            }
            
            free(core_percentages);
        }
    }
    
    prev_total_cpu = curr_total;
}

/* Memory Functions */
void get_memory_info(int bar_width) {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    
    double total = (double)memInfo.ullTotalPhys;
    double used = total - (double)memInfo.ullAvailPhys;
    
    double total_page = (double)memInfo.ullTotalPageFile;
    double used_page = total_page - (double)memInfo.ullAvailPageFile;
    
    char bar[256], b1[32], b2[32];
    
    draw_bar_ascii(used, total, bar_width, bar, sizeof(bar));
    format_bytes(used, b1, sizeof(b1));
    format_bytes(total, b2, sizeof(b2));
    printf("RAM:    %s %s%s/%s%s\n", bar, c_white(), b1, b2, c_reset());
    
    draw_bar_ascii(used_page, total_page, bar_width, bar, sizeof(bar));
    format_bytes(used_page, b1, sizeof(b1));
    format_bytes(total_page, b2, sizeof(b2));
    printf("Page:   %s %s%s/%s%s\n", bar, c_white(), b1, b2, c_reset());
}

/* Disk Functions */
void get_disk_info(int bar_width) {
    DWORD drives = GetLogicalDrives();
    char bar[256], b1[32], b2[32];
    
    for (int i = 0; i < 26; i++) {
        if (drives & (1 << i)) {
            char drive[4];
            snprintf(drive, sizeof(drive), "%c:\\", 'A' + i);
            
            UINT driveType = GetDriveTypeA(drive);
            if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOTE) {
                ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;
                
                if (GetDiskFreeSpaceExA(drive, &freeBytes, &totalBytes, &totalFreeBytes)) {
                    unsigned long long total = totalBytes.QuadPart;
                    unsigned long long free = freeBytes.QuadPart;
                    unsigned long long used = total - free;
                    
                    char label[MAX_PATH] = "";
                    GetVolumeInformationA(drive, label, MAX_PATH, NULL, NULL, NULL, NULL, 0);
                    
                    draw_bar_ascii((double)used, (double)total, bar_width, bar, sizeof(bar));
                    format_bytes((double)used, b1, sizeof(b1));
                    format_bytes((double)total, b2, sizeof(b2));
                    
                    if (strlen(label) > 0) {
                        printf("%s%s%s [%s]: %s %s%s/%s%s\n", 
                            c_cyan(), drive, c_reset(), label,
                            bar, c_white(), b1, b2, c_reset());
                    } else {
                        printf("%s%s%s: %s %s%s/%s%s\n", 
                            c_cyan(), drive, c_reset(),
                            bar, c_white(), b1, b2, c_reset());
                    }
                }
            }
        }
    }
}

/* Network Functions */
void get_net_info(int bar_width) {
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    MIB_IF_TABLE2 *pIfTable = NULL;
    
    dwRetVal = GetIfTable2(&pIfTable);
    
    if (dwRetVal == NO_ERROR && pIfTable != NULL) {
        LARGE_INTEGER currTime;
        QueryPerformanceCounter(&currTime);
        
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        
        double dt = 0;
        int first_run = 0;
        
        if (net_prev_time.QuadPart == 0) {
            first_run = 1;
        } else {
            dt = (double)(currTime.QuadPart - net_prev_time.QuadPart) / freq.QuadPart;
        }
        
        char bar[256], b1[32];
        int found = 0;
        
        for (DWORD i = 0; i < pIfTable->NumEntries; i++) {
            MIB_IF_ROW2 *pIfRow = &pIfTable->Table[i];
            
            if (pIfRow->Type == IF_TYPE_SOFTWARE_LOOPBACK) continue;
            if (pIfRow->OperStatus != IfOperStatusUp) continue;
            
            char ifName[128];
            WideCharToMultiByte(CP_UTF8, 0, pIfRow->Description, -1, 
                               ifName, sizeof(ifName), NULL, NULL);
            
            if (opt_net_iface && strstr(ifName, opt_net_iface) == NULL) continue;
            
            unsigned long long recv = pIfRow->InOctets;
            unsigned long long sent = pIfRow->OutOctets;
            
            if (!first_run && dt > 0) {
                for (int j = 0; j < net_prev_count; j++) {
                    if (strcmp(net_prev[j].name, ifName) == 0) {
                        double rx_spd = (double)(recv - net_prev[j].bytes_recv) / dt;
                        double tx_spd = (double)(sent - net_prev[j].bytes_sent) / dt;
                        
                        double link_speed = (double)pIfRow->TransmitLinkSpeed / 8.0;
                        if (link_speed == 0) link_speed = 100 * 1024 * 1024;
                        
                        double rx_pct = (rx_spd / link_speed) * 100.0;
                        double tx_pct = (tx_spd / link_speed) * 100.0;
                        
                        draw_bar_ascii(rx_pct, 100, bar_width, bar, sizeof(bar));
                        format_bytes(rx_spd, b1, sizeof(b1));
                        printf("%sDN:%s    %s %s%s/s%s\n", c_magenta(), c_reset(), bar, c_white(), b1, c_reset());
                        
                        draw_bar_ascii(tx_pct, 100, bar_width, bar, sizeof(bar));
                        format_bytes(tx_spd, b1, sizeof(b1));
                        printf("%sUP:%s    %s %s%s/s%s\n", c_magenta(), c_reset(), bar, c_white(), b1, c_reset());
                        
                        found = 1;
                        break;
                    }
                }
            } else {
                if (!found) {
                    printf("%sNET%s:  Waiting for first sample...\n", c_magenta(), c_reset());
                    found = 1;
                }
            }
            
            int k;
            for (k = 0; k < net_prev_count; k++) {
                if (strcmp(net_prev[k].name, ifName) == 0) {
                    net_prev[k].bytes_recv = recv;
                    net_prev[k].bytes_sent = sent;
                    break;
                }
            }
            if (k == net_prev_count && net_prev_count < MAX_IFACES) {
                strncpy(net_prev[net_prev_count].name, ifName, sizeof(net_prev[0].name) - 1);
                net_prev[net_prev_count].bytes_recv = recv;
                net_prev[net_prev_count].bytes_sent = sent;
                net_prev_count++;
            }
        }
        
        net_prev_time = currTime;
        FreeMibTable(pIfTable);
    }
}

/* Logging Functions */
void write_log_header(int show_cpu, int show_mem, int show_disks, int show_net) {
    if (!log_fp || log_header_written) return;
    
    fprintf(log_fp, "Timestamp");
    
    if (show_cpu) {
        fprintf(log_fp, ",CPU_Total_Percent");
    }
    
    if (show_mem) {
        fprintf(log_fp, ",RAM_Used_Bytes,RAM_Total_Bytes,RAM_Percent,Page_Used_Bytes,Page_Total_Bytes,Page_Percent");
    }
    
    if (show_disks) {
        DWORD drives = GetLogicalDrives();
        for (int i = 0; i < 26; i++) {
            if (drives & (1 << i)) {
                char drive[4];
                snprintf(drive, sizeof(drive), "%c:", 'A' + i);
                UINT driveType = GetDriveTypeA(drive);
                if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOTE) {
                    fprintf(log_fp, ",Disk_%s_Used_Bytes,Disk_%s_Total_Bytes,Disk_%s_Percent", drive, drive, drive);
                }
            }
        }
    }
    
    if (show_net) {
        DWORD dwSize = 0;
        MIB_IF_TABLE2 *pIfTable = NULL;
        
        if (GetIfTable2(&pIfTable) == NO_ERROR && pIfTable != NULL) {
            for (DWORD i = 0; i < pIfTable->NumEntries; i++) {
                if (pIfTable->Table[i].Type != IF_TYPE_SOFTWARE_LOOPBACK &&
                    pIfTable->Table[i].OperStatus == IfOperStatusUp) {
                    char ifName[128];
                    WideCharToMultiByte(CP_UTF8, 0, pIfTable->Table[i].Description, -1, 
                                       ifName, sizeof(ifName), NULL, NULL);
                    fprintf(log_fp, ",Net_%s_RX_Bps,Net_%s_TX_Bps", ifName, ifName);
                }
            }
            FreeMibTable(pIfTable);
        }
    }
    
    fprintf(log_fp, "\n");
    fflush(log_fp);
    log_header_written = 1;
}

void log_data(int show_cpu, int show_mem, int show_disks, int show_net) {
    if (!log_fp) return;
    
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);
    fprintf(log_fp, "%s", timestamp);
    
    if (show_cpu) {
        FILETIME idleTime, kernelTime, userTime;
        GetSystemTimes(&idleTime, &kernelTime, &userTime);
        
        cpu_times_t curr_total;
        curr_total.idle = ((ULONGLONG)idleTime.dwHighDateTime << 32) | idleTime.dwLowDateTime;
        curr_total.kernel = ((ULONGLONG)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;
        curr_total.user = ((ULONGLONG)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;
        
        double total_pct = calculate_cpu_percent(&curr_total, &prev_total_cpu);
        fprintf(log_fp, ",%.2f", total_pct);
    }
    
    if (show_mem) {
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        
        double total = (double)memInfo.ullTotalPhys;
        double used = total - (double)memInfo.ullAvailPhys;
        double pct = (used / total) * 100.0;
        
        double total_page = (double)memInfo.ullTotalPageFile;
        double used_page = total_page - (double)memInfo.ullAvailPageFile;
        double pct_page = (total_page > 0) ? (used_page / total_page) * 100.0 : 0.0;
        
        fprintf(log_fp, ",%.0f,%.0f,%.2f,%.0f,%.0f,%.2f", used, total, pct, used_page, total_page, pct_page);
    }
    
    if (show_disks) {
        DWORD drives = GetLogicalDrives();
        for (int i = 0; i < 26; i++) {
            if (drives & (1 << i)) {
                char drive[4];
                snprintf(drive, sizeof(drive), "%c:\\", 'A' + i);
                UINT driveType = GetDriveTypeA(drive);
                if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOTE) {
                    ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;
                    if (GetDiskFreeSpaceExA(drive, &freeBytes, &totalBytes, &totalFreeBytes)) {
                        unsigned long long total = totalBytes.QuadPart;
                        unsigned long long used = total - freeBytes.QuadPart;
                        double pct = (total > 0) ? ((double)used / total) * 100.0 : 0.0;
                        fprintf(log_fp, ",%I64u,%I64u,%.2f", used, total, pct);
                    } else {
                        fprintf(log_fp, ",0,0,0");
                    }
                }
            }
        }
    }
    
    if (show_net) {
        MIB_IF_TABLE2 *pIfTable = NULL;
        if (GetIfTable2(&pIfTable) == NO_ERROR && pIfTable != NULL) {
            LARGE_INTEGER currTime;
            QueryPerformanceCounter(&currTime);
            
            LARGE_INTEGER freq;
            QueryPerformanceFrequency(&freq);
            
            double dt = (net_prev_time.QuadPart > 0) ? 
                       (double)(currTime.QuadPart - net_prev_time.QuadPart) / freq.QuadPart : 0;
            
            for (DWORD i = 0; i < pIfTable->NumEntries; i++) {
                if (pIfTable->Table[i].Type != IF_TYPE_SOFTWARE_LOOPBACK &&
                    pIfTable->Table[i].OperStatus == IfOperStatusUp) {
                    char ifName[128];
                    WideCharToMultiByte(CP_UTF8, 0, pIfTable->Table[i].Description, -1, 
                                       ifName, sizeof(ifName), NULL, NULL);
                    
                    unsigned long long recv = pIfTable->Table[i].InOctets;
                    unsigned long long sent = pIfTable->Table[i].OutOctets;
                    
                    if (dt > 0) {
                        for (int j = 0; j < net_prev_count; j++) {
                            if (strcmp(net_prev[j].name, ifName) == 0) {
                                double rx_spd = (double)(recv - net_prev[j].bytes_recv) / dt;
                                double tx_spd = (double)(sent - net_prev[j].bytes_sent) / dt;
                                fprintf(log_fp, ",%.2f,%.2f", rx_spd, tx_spd);
                                break;
                            }
                        }
                    } else {
                        fprintf(log_fp, ",0,0");
                    }
                }
            }
            FreeMibTable(pIfTable);
        }
    }
    
    fprintf(log_fp, "\n");
    fflush(log_fp);
}

void print_help(void) {
	printf("Info: umon v%s by %s (%s) [%s].\n\n", VERSION, AUTHOR, DATE, AUTHOR_GITHUB);
    printf("Usage: umon [options]\n\n");
    printf("Options:\n");
    printf("  -h, --help           Show this help message\n");
    printf("  --cpu                Display only CPU usage information\n");
    printf("  --mem                Display only memory usage\n");
    printf("  --disks              Display only disk usage\n");
    printf("  --net [IFACE]        Display only network usage (optional: specific interface)\n");
    printf("  --netlist            List network interfaces and exit\n");
    printf("  --cpulist            Show CPU cores as a list\n");
    printf("  --mono               Disable colors\n");
    printf("  --interval MS        Refresh interval in milliseconds (default 250)\n");
    printf("  --sysinfo            Display system info and exit\n");
    printf("  --log FILENAME       Log data to CSV file with the same interval\n");
    printf("\nLogging:\n");
    printf("  Use --log to save monitoring data to a CSV file.\n");
    printf("  The log includes all enabled metrics (CPU, memory, disks, network)\n");
    printf("  with timestamps. Data is written at each refresh interval.\n");
    printf("  Example: umon --cpu --mem --log system.csv\n");
    printf("\nWindows Notes:\n");
    printf("  On Windows, colors require Windows 10 version 1511 or later.\n");
    printf("  Use --mono if colors are not displaying correctly.\n");
}

void print_oneline_help(void) {
    printf("Info: umon v%s by %s (%s). For detailed help, use '--help'.\n\n", VERSION, AUTHOR, DATE);
	printf("Usage: umon [--cpu] [--mem] [--disks] [--net [IFACE]] [--mono] [--interval MS] [--log FILE] [--sysinfo] | -h | --help\n");
    
}

/* Cleanup */
void cleanup(void) {
    restoreConsole();
    if (log_fp) {
        fclose(log_fp);
        log_fp = NULL;
    }
    if (prev_cpu_times) {
        free(prev_cpu_times);
        prev_cpu_times = NULL;
    }
    if (prev_per_core_times) {
        free(prev_per_core_times);
        prev_per_core_times = NULL;
    }
}

void handle_exit(void) {
    cleanup();
    printf("\n\nMonitoring stopped.\n");
    if (opt_log) {
        printf("Log saved to: %s\n", opt_log);
    }
}

int main(int argc, char **argv) {
    enableAnsiOnWindows();
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            print_oneline_help();
            return 0;
        }
        else if (strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        }
        else if (strcmp(argv[i], "--cpu") == 0) opt_cpu = 1;
        else if (strcmp(argv[i], "--mem") == 0) opt_mem = 1;
        else if (strcmp(argv[i], "--disks") == 0) opt_disks = 1;
        else if (strcmp(argv[i], "--cpulist") == 0) opt_cpulist = 1;
        else if (strcmp(argv[i], "--mono") == 0) opt_mono = 1;
        else if (strcmp(argv[i], "--sysinfo") == 0) {
             display_sysinfo();
             return 0;
        }
        else if (strcmp(argv[i], "--netlist") == 0) {
             display_netlist();
             return 0;
        }
        else if (strcmp(argv[i], "--interval") == 0) {
            if (i + 1 < argc) {
                opt_interval = atoi(argv[++i]);
                if (opt_interval < 50) {
                    printf("Warning: interval should be at least 50ms. Setting to 50ms.\n");
                    opt_interval = 50;
                }
            }
        }
        else if (strcmp(argv[i], "--log") == 0) {
            if (i + 1 < argc) {
                opt_log = argv[++i];
                log_fp = fopen(opt_log, "w");
                if (!log_fp) {
                    perror("Failed to open log file");
                    return 1;
                }
            } else {
                printf("Error: --log requires a filename\n");
                return 1;
            }
        }
        else if (strcmp(argv[i], "--net") == 0) {
            opt_net_all = 1;
            if (i + 1 < argc && argv[i+1][0] != '-') {
                opt_net_iface = argv[++i];
            }
        }
    }
    
    int any_specific = opt_cpu || opt_mem || opt_disks || opt_net_all || opt_cpulist;
    int show_cpu = opt_cpu || opt_cpulist || !any_specific;
    int show_mem = opt_mem || !any_specific;
    int show_disks = opt_disks || !any_specific;
    int show_net = opt_net_all || (!any_specific);
    
    if (!any_specific) show_net = 0;
    
    init_cpu_stats();
    
    int first_run = 1;
    
    while (1) {
        system("cls");
        
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        char time_str[10];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);
        
        char title[128];
        snprintf(title, sizeof(title), "System Monitor (v%s)", VERSION);
        
        int total_width = 60;
        int len_txt = strlen(title) + 1 + strlen(time_str);
        int pad = total_width - len_txt;
        int pad_l = pad / 2;
        int pad_r = pad - pad_l;
        
        printf("%s", c_magenta());
        for(int k=0; k<total_width; k++) putchar('=');
        printf("%s\n", c_reset());
        
        printf("%s%s", c_bold(), c_cyan());
        for(int k=0; k<pad_l; k++) putchar(' ');
        printf("%s%s %s%s%s%s", title, c_reset(), c_dim(), c_white(), time_str, c_reset());
        for(int k=0; k<pad_r; k++) putchar(' ');
        printf("%s\n", c_reset());
        
        printf("%s", c_magenta());
        for(int k=0; k<total_width; k++) putchar('=');
        printf("%s\n", c_reset());
        
        if (show_cpu) {
            printf("\n");
            get_cpu_info(30, opt_cpulist);
        }
        
        if (show_mem) {
            printf("\n");
            get_memory_info(30);
        }
        
        if (show_disks) {
            printf("\n");
            get_disk_info(30);
        }
        
        if (show_net) {
            printf("\n");
            get_net_info(30);
        }
        
        if (log_fp && !first_run) {
            log_data(show_cpu, show_mem, show_disks, show_net);
        } else if (log_fp && first_run) {
            write_log_header(show_cpu, show_mem, show_disks, show_net);
        }
        
        printf("\nPress Ctrl+C to quit.");
        if (opt_log) {
            printf(" Logging to: %s", opt_log);
        }
        printf("\n");
        fflush(stdout);
        
        Sleep(opt_interval);
        
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 3) {
                handle_exit();
                return 0;
            }
        }
        
        first_run = 0;
    }
    
    return 0;
}
