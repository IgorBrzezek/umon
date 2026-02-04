#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <linux/if_link.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <ctype.h>
#include <mntent.h>
#include <math.h>
#include <signal.h>
#include <netpacket/packet.h>

/* Program Information */
#define __CODEVERSION__ "0.0.3"
#define __CODEAUTHOR__ "Igor Brzezek"
#define __CODEDATE__ "04.02.2026"
#define __CODEGIT__ "github.com/igorbrzezek"

#define RESET   "\033[0m"
#define RED     "\033[91m"
#define GREEN   "\033[92m"
#define YELLOW  "\033[93m"
#define BLUE    "\033[94m"
#define MAGENTA "\033[95m"
#define CYAN    "\033[96m"
#define WHITE   "\033[97m"
#define BOLD    "\033[1m"
#define DIM     "\033[2m"

/* State Structures */
typedef struct {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
} cpu_stats_t;

typedef struct {
    char name[32];
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

/* Helpers */
const char* c_red(void) { return opt_mono ? "" : RED; }
const char* c_green(void) { return opt_mono ? "" : GREEN; }
const char* c_yellow(void) { return opt_mono ? "" : YELLOW; }
const char* c_blue(void) { return opt_mono ? "" : BLUE; }
const char* c_magenta(void) { return opt_mono ? "" : MAGENTA; }
const char* c_cyan(void) { return opt_mono ? "" : CYAN; }
const char* c_white(void) { return opt_mono ? "" : WHITE; }
const char* c_bold(void) { return opt_mono ? "" : BOLD; }
const char* c_dim(void) { return opt_mono ? "" : DIM; }
const char* c_reset(void) { return opt_mono ? "" : RESET; }

void format_bytes(double bytes, char *buffer, size_t size) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int i = 0;
    while (bytes >= 1024 && i < 5) {
        bytes /= 1024;
        i++;
    }
    snprintf(buffer, size, "%.1f%s", bytes, units[i]);
}

const char* get_color_for_percentage(double pct) {
    if (opt_mono) return "";
    if (pct < 33) return GREEN;
    if (pct < 66) return YELLOW;
    return RED;
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
    struct utsname un;
    uname(&un);
    
    printf("%sSystem Information:%s\n", c_bold(), c_reset());
    printf("  OS Name: %s\n", un.sysname);
    printf("  OS Release: %s\n", un.release);
    printf("  OS Version: %s\n", un.version);
    printf("  Machine: %s\n", un.machine);
    
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f) {
        char line[512];
        char model_name[256] = "Unknown";
        int cores = 0;
        double cpu_mhz = 0;
        
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "model name", 10) == 0) {
                char *p = strchr(line, ':');
                if (p) {
                    strncpy(model_name, p + 2, sizeof(model_name) - 1);
                    model_name[strcspn(model_name, "\n")] = 0;
                }
            }
            if (strncmp(line, "processor", 9) == 0) {
                cores++;
            }
            if (strncmp(line, "cpu MHz", 7) == 0) {
                sscanf(line, "cpu MHz : %lf", &cpu_mhz);
            }
        }
        fclose(f);
        
        printf("  Processor: %s\n", model_name);
        printf("\n%sCPU Information (via procfs):%s\n", c_bold(), c_reset());
        printf("  Logical Cores: %d\n", cores);
        if (cpu_mhz > 0)
            printf("  Current Freq: %.2f Mhz\n", cpu_mhz);
    }

    f = fopen("/proc/meminfo", "r");
    if (f) {
        char line[256];
        unsigned long mem_total = 0, mem_free = 0, mem_available = 0;
        unsigned long swap_total = 0, swap_free = 0;
        
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "MemTotal: %lu kB", &mem_total) == 1) {}
            if (sscanf(line, "MemAvailable: %lu kB", &mem_available) == 1) {}
            if (sscanf(line, "MemFree: %lu kB", &mem_free) == 1) {}
            if (sscanf(line, "SwapTotal: %lu kB", &swap_total) == 1) {}
            if (sscanf(line, "SwapFree: %lu kB", &swap_free) == 1) {}
        }
        fclose(f);
        
        if (mem_available == 0) mem_available = mem_free;

        double total_gb = (double)mem_total * 1024;
        double avail_gb = (double)mem_available * 1024;
        double used_gb = total_gb - avail_gb;
        double pct = (used_gb / total_gb) * 100.0;
        
        char buf1[32], buf2[32], buf3[32];

        printf("\n%sMemory Information (via procfs):%s\n", c_bold(), c_reset());
        format_bytes(total_gb, buf1, sizeof(buf1));
        printf("  Total: %s\n", buf1);
        format_bytes(avail_gb, buf2, sizeof(buf2));
        printf("  Available: %s\n", buf2);
        format_bytes(used_gb, buf3, sizeof(buf3));
        printf("  Used: %s\n", buf3);
        printf("  Percentage: %.2f%%\n", pct);

        printf("\n%sSwap Information (via procfs):%s\n", c_bold(), c_reset());
        double sw_total = (double)swap_total * 1024;
        double sw_free = (double)swap_free * 1024;
        double sw_used = sw_total - sw_free;
        double sw_pct = (sw_total > 0) ? (sw_used / sw_total) * 100.0 : 0.0;
        
        format_bytes(sw_total, buf1, sizeof(buf1));
        printf("  Total: %s\n", buf1);
        format_bytes(sw_used, buf2, sizeof(buf2));
        printf("  Used: %s\n", buf2);
        format_bytes(sw_free, buf3, sizeof(buf3));
        printf("  Free: %s\n", buf3);
        printf("  Percentage: %.2f%%\n", sw_pct);
    }

    exit(0);
}

/* Network List */
void display_netlist(void) {
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];
    
    printf("%sNetwork Interfaces:%s\n", c_bold(), c_reset());

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(1);
    }

    FILE *fp = fopen("/proc/net/dev", "r");
    if (fp) {
        char line[256];
        fgets(line, sizeof(line), fp);
        fgets(line, sizeof(line), fp);
        
        while (fgets(line, sizeof(line), fp)) {
            char *p = line;
            while (*p == ' ') p++;
            char *name_end = strchr(p, ':');
            if (name_end) {
                *name_end = '\0';
                char *ifname = p;
                
                printf("\n%s  Interface: %s%s\n", c_blue(), ifname, c_reset());
                
                char path[256];
                char buf[64];
                
                snprintf(path, sizeof(path), "/sys/class/net/%s/operstate", ifname);
                FILE *fs = fopen(path, "r");
                if (fs) {
                    if (fgets(buf, sizeof(buf), fs)) {
                        buf[strcspn(buf, "\n")] = 0;
                        printf("    Status: %s\n", (strcmp(buf, "up") == 0) ? "Up" : "Down");
                    } else {
                        printf("    Status: N/A\n");
                    }
                    fclose(fs);
                } else {
                    printf("    Status: N/A\n");
                }
                
                snprintf(path, sizeof(path), "/sys/class/net/%s/duplex", ifname);
                fs = fopen(path, "r");
                if (fs) {
                    if (fgets(buf, sizeof(buf), fs)) {
                        buf[strcspn(buf, "\n")] = 0;
                        buf[0] = toupper(buf[0]);
                        printf("    Duplex: %s\n", buf);
                    } else {
                        printf("    Duplex: N/A\n");
                    }
                    fclose(fs);
                }

                snprintf(path, sizeof(path), "/sys/class/net/%s/speed", ifname);
                fs = fopen(path, "r");
                if (fs) {
                     int speed = 0;
                     if (fscanf(fs, "%d", &speed) == 1) {
                         printf("    Speed: %d Mbps\n", speed);
                     }
                     fclose(fs);
                }
                
                snprintf(path, sizeof(path), "/sys/class/net/%s/mtu", ifname);
                fs = fopen(path, "r");
                if (fs) {
                     int mtu = 0;
                     if (fscanf(fs, "%d", &mtu) == 1) {
                         printf("    MTU: %d\n", mtu);
                     }
                     fclose(fs);
                }

                for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                    if (ifa->ifa_addr == NULL) continue;
                    if (strcmp(ifa->ifa_name, ifname) != 0) continue;
                    
                    family = ifa->ifa_addr->sa_family;
                    
                    if (family == AF_PACKET) {
                         struct sockaddr_ll *s = (struct sockaddr_ll*)ifa->ifa_addr;
                         printf("    MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            s->sll_addr[0], s->sll_addr[1], s->sll_addr[2],
                            s->sll_addr[3], s->sll_addr[4], s->sll_addr[5]);
                    } else if (family == AF_INET) {
                        s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                                        host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                        if (s == 0) printf("    IPv4 Address: %s\n", host);
                        
                        if (ifa->ifa_netmask) {
                            s = getnameinfo(ifa->ifa_netmask, sizeof(struct sockaddr_in),
                                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                             if (s == 0) printf("    Netmask: %s\n", host);
                        }
                         if (ifa->ifa_ifu.ifu_broadaddr) {
                            s = getnameinfo(ifa->ifa_ifu.ifu_broadaddr, sizeof(struct sockaddr_in),
                                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                             if (s == 0) printf("    Broadcast: %s\n", host);
                        }

                    } else if (family == AF_INET6) {
                         s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6),
                                        host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                        if (s == 0) printf("    IPv6 Address: %s\n", host);
                    }
                }
            }
        }
        fclose(fp);
    }
    
    freeifaddrs(ifaddr);
    exit(0);
}

/* Core Monitoring Functions */

void parse_cpu_line(const char *line, cpu_stats_t *stats) {
    sscanf(line, "%*s %llu %llu %llu %llu %llu %llu %llu %llu",
           &stats->user, &stats->nice, &stats->system,
           &stats->idle, &stats->iowait, &stats->irq,
           &stats->softirq, &stats->steal);
}

double calculate_cpu_percent(cpu_stats_t *curr, cpu_stats_t *prev) {
    unsigned long long prev_idle = prev->idle + prev->iowait;
    unsigned long long curr_idle = curr->idle + curr->iowait;
    
    unsigned long long prev_non_idle = prev->user + prev->nice + prev->system + prev->irq + prev->softirq + prev->steal;
    unsigned long long curr_non_idle = curr->user + curr->nice + curr->system + curr->irq + curr->softirq + curr->steal;
    
    unsigned long long prev_total = prev_idle + prev_non_idle;
    unsigned long long curr_total = curr_idle + curr_non_idle;
    
    unsigned long long total_diff = curr_total - prev_total;
    unsigned long long idle_diff = curr_idle - prev_idle;
    
    if (total_diff == 0) return 0.0;
    
    return ((double)(total_diff - idle_diff) / total_diff) * 100.0;
}

cpu_stats_t cpu_prev_total;
cpu_stats_t *cpu_prev_cores = NULL;
int num_cores = 0;

void init_cpu_stats() {
    num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    cpu_prev_cores = calloc(num_cores, sizeof(cpu_stats_t));
    
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return;
    char line[512];
    int core_idx = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "cpu ", 4) == 0) {
            parse_cpu_line(line, &cpu_prev_total);
        } else if (strncmp(line, "cpu", 3) == 0 && isdigit(line[3])) {
            if (core_idx < num_cores) {
                parse_cpu_line(line, &cpu_prev_cores[core_idx]);
                core_idx++;
            }
        }
    }
    fclose(f);
}

void get_cpu_info(int bar_width) {
    cpu_stats_t curr_total;
    cpu_stats_t *curr_cores = calloc(num_cores, sizeof(cpu_stats_t));
    char line[512];
    
    FILE *f = fopen("/proc/stat", "r");
    if (!f) { free(curr_cores); return; }
    
    int core_idx = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "cpu ", 4) == 0) {
            parse_cpu_line(line, &curr_total);
        } else if (strncmp(line, "cpu", 3) == 0 && isdigit(line[3])) {
            if (core_idx < num_cores) {
                parse_cpu_line(line, &curr_cores[core_idx]);
                core_idx++;
            }
        }
    }
    fclose(f);
    
    char buf[256];
    
    if (opt_cpulist) {
        for (int i = 0; i < num_cores; i++) {
            double pct = calculate_cpu_percent(&curr_cores[i], &cpu_prev_cores[i]);
            draw_bar_ascii(pct, 100, bar_width, buf, sizeof(buf));
            printf("CPU %2d: %s\n", i, buf);
            cpu_prev_cores[i] = curr_cores[i];
        }
    } else {
        double total_pct = calculate_cpu_percent(&curr_total, &cpu_prev_total);
        draw_bar_ascii(total_pct, 100, bar_width, buf, sizeof(buf));
        printf("%sCPU%s (%d cores): %s\n", c_blue(), c_reset(), num_cores, buf);
        cpu_prev_total = curr_total;
        
        int cores_per_row = 3;
        for (int i = 0; i < num_cores; i += cores_per_row) {
            for (int j = i; j < i + cores_per_row && j < num_cores; j++) {
                double pct = calculate_cpu_percent(&curr_cores[j], &cpu_prev_cores[j]);
                draw_bar_ascii(pct, 100, bar_width, buf, sizeof(buf));
                printf("%s#%2d:%s%s  ", c_white(), j, c_reset(), buf);
                cpu_prev_cores[j] = curr_cores[j];
            }
            printf("\n");
        }
    }
    
    free(curr_cores);
}

void get_memory_info(int bar_width) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return;
    
    char line[256];
    unsigned long mem_total = 0, mem_free = 0, mem_available = 0;
    unsigned long swap_total = 0, swap_free = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "MemTotal: %lu kB", &mem_total) == 1) {}
        if (sscanf(line, "MemAvailable: %lu kB", &mem_available) == 1) {}
        if (sscanf(line, "MemFree: %lu kB", &mem_free) == 1) {}
        if (sscanf(line, "SwapTotal: %lu kB", &swap_total) == 1) {}
        if (sscanf(line, "SwapFree: %lu kB", &swap_free) == 1) {}
    }
    fclose(f);
    
    if (mem_available == 0) mem_available = mem_free;
    
    double total_bytes = (double)mem_total * 1024;
    double used_bytes = total_bytes - ((double)mem_available * 1024);
    
    double sw_total = (double)swap_total * 1024;
    double sw_used = sw_total - ((double)swap_free * 1024);
    
    char bar[256], b1[32], b2[32];
    
    draw_bar_ascii(used_bytes, total_bytes, bar_width, bar, sizeof(bar));
    format_bytes(used_bytes, b1, sizeof(b1));
    format_bytes(total_bytes, b2, sizeof(b2));
    printf("RAM:    %s %s%s/%s%s\n", bar, c_white(), b1, b2, c_reset());
    
    draw_bar_ascii(sw_used, sw_total, bar_width, bar, sizeof(bar));
    format_bytes(sw_used, b1, sizeof(b1));
    format_bytes(sw_total, b2, sizeof(b2));
    printf("SWAP:   %s %s%s/%s%s\n", bar, c_white(), b1, b2, c_reset());
}

void get_disk_info(int bar_width) {
    FILE *mtab = setmntent("/proc/mounts", "r");
    if (!mtab) return;
    
    struct mntent *ent;
    char bar[256], b1[32], b2[32];
    
    while ((ent = getmntent(mtab)) != NULL) {
        if (strncmp(ent->mnt_fsname, "/dev/", 5) == 0 && strstr(ent->mnt_fsname, "loop") == NULL) {
             struct statvfs s;
             if (statvfs(ent->mnt_dir, &s) == 0) {
                 unsigned long long total = s.f_blocks * s.f_frsize;
                 unsigned long long free = s.f_bfree * s.f_frsize;
                 unsigned long long used = total - free;
                 
                 draw_bar_ascii((double)used, (double)total, bar_width, bar, sizeof(bar));
                 format_bytes((double)used, b1, sizeof(b1));
                 format_bytes((double)total, b2, sizeof(b2));
                 
                 printf("%s%s%s (%s): %s %s%s/%s%s\n", 
                    c_cyan(), ent->mnt_fsname, c_reset(), ent->mnt_dir,
                    bar, c_white(), b1, b2, c_reset());
             }
        }
    }
    endmntent(mtab);
}

/* Net state */
#define MAX_IFACES 32
net_stats_t net_prev[MAX_IFACES];
int net_prev_count = 0;
double last_net_time = 0;

double get_time_sec() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
}

void get_net_info(int bar_width) {
    FILE *f = fopen("/proc/net/dev", "r");
    if (!f) return;
    
    double curr_time = get_time_sec();
    double dt = curr_time - last_net_time;
    
    int first_run = (last_net_time == 0);
    
    char line[512];
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    
    net_stats_t curr[MAX_IFACES];
    int curr_count = 0;
    
    char bar[256], b1[32];
    
    int printed_wait_msg = 0;
    
    while (fgets(line, sizeof(line), f) && curr_count < MAX_IFACES) {
        char *p = line;
        while (*p == ' ') p++;
        char *end = strchr(p, ':');
        if (!end) continue;
        *end = '\0';
        strncpy(curr[curr_count].name, p, 31);
        
        p = end + 1;
        unsigned long long r_bytes, r_packets, r_errs, r_drop, r_fifo, r_frame, r_compressed, r_multicast;
        unsigned long long s_bytes, s_packets, s_errs, s_drop, s_fifo, s_colls, s_carrier, s_compressed;
        
        sscanf(p, "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
            &r_bytes, &r_packets, &r_errs, &r_drop, &r_fifo, &r_frame, &r_compressed, &r_multicast,
            &s_bytes, &s_packets, &s_errs, &s_drop, &s_fifo, &s_colls, &s_carrier, &s_compressed);
            
        curr[curr_count].bytes_recv = r_bytes;
        curr[curr_count].bytes_sent = s_bytes;
        
        if (!first_run && dt > 0) {
            for (int i = 0; i < net_prev_count; i++) {
                if (strcmp(net_prev[i].name, curr[curr_count].name) == 0) {
                    
                    if (opt_net_iface && strcmp(opt_net_iface, curr[curr_count].name) != 0) break;
                    
                    double rx_spd = (curr[curr_count].bytes_recv - net_prev[i].bytes_recv) / dt;
                    double tx_spd = (curr[curr_count].bytes_sent - net_prev[i].bytes_sent) / dt;
                    
                    double link_speed_mbps = 0;
                    char path[256];
                    snprintf(path, sizeof(path), "/sys/class/net/%s/speed", curr[curr_count].name);
                    FILE *fs = fopen(path, "r");
                    if (fs) {
                        if (fscanf(fs, "%lf", &link_speed_mbps) != 1) link_speed_mbps = 0;
                        fclose(fs);
                    }
                    
                    double link_speed_bps = link_speed_mbps * 125000.0;
                    
                    if (link_speed_bps <= 0) link_speed_bps = 100 * 1024 * 1024;
                    
                    double rx_pct = 0, tx_pct = 0;
                    if (link_speed_bps > 0) {
                        rx_pct = (rx_spd / link_speed_bps) * 100.0;
                        tx_pct = (tx_spd / link_speed_bps) * 100.0;
                    }
                    
                    if (opt_net_iface == NULL || strcmp(opt_net_iface, curr[curr_count].name) == 0) {
                        draw_bar_ascii(rx_pct, 100, bar_width, bar, sizeof(bar));
                        format_bytes(rx_spd, b1, sizeof(b1));
                        printf("DN:     %s %s%s/s%s\n", bar, c_white(), b1, c_reset());
                        
                        draw_bar_ascii(tx_pct, 100, bar_width, bar, sizeof(bar));
                        format_bytes(tx_spd, b1, sizeof(b1));
                        printf("UP:     %s %s%s/s%s\n", bar, c_white(), b1, c_reset());
                    }
                }
            }
        } else {
            if (!printed_wait_msg) {
                printf("%sNET%s:  Waiting for first sample...\n", c_magenta(), c_reset());
                printed_wait_msg = 1;
            }
        }
        
        curr_count++;
    }
    fclose(f);
    
    memcpy(net_prev, curr, sizeof(curr));
    net_prev_count = curr_count;
    last_net_time = curr_time;
}

/* Logging Functions */
void write_log_header(int show_cpu, int show_mem, int show_disks, int show_net) {
    if (!log_fp || log_header_written) return;
    
    fprintf(log_fp, "Timestamp");
    
    if (show_cpu) {
        fprintf(log_fp, ",CPU_Total_Percent");
        for (int i = 0; i < num_cores; i++) {
            fprintf(log_fp, ",CPU_Core_%d_Percent", i);
        }
    }
    
    if (show_mem) {
        fprintf(log_fp, ",RAM_Used_Bytes,RAM_Total_Bytes,RAM_Percent,Swap_Used_Bytes,Swap_Total_Bytes,Swap_Percent");
    }
    
    if (show_disks) {
        FILE *mtab = setmntent("/proc/mounts", "r");
        if (mtab) {
            struct mntent *ent;
            while ((ent = getmntent(mtab)) != NULL) {
                if (strncmp(ent->mnt_fsname, "/dev/", 5) == 0 && strstr(ent->mnt_fsname, "loop") == NULL) {
                    fprintf(log_fp, ",Disk_%s_Used_Bytes,Disk_%s_Total_Bytes,Disk_%s_Percent", ent->mnt_fsname, ent->mnt_fsname, ent->mnt_fsname);
                }
            }
            endmntent(mtab);
        }
    }
    
    if (show_net) {
        FILE *f = fopen("/proc/net/dev", "r");
        if (f) {
            char line[512];
            fgets(line, sizeof(line), f);
            fgets(line, sizeof(line), f);
            while (fgets(line, sizeof(line), f)) {
                char *p = line;
                while (*p == ' ') p++;
                char *end = strchr(p, ':');
                if (end) {
                    *end = '\0';
                    char *ifname = p;
                    if (opt_net_iface == NULL || strcmp(opt_net_iface, ifname) == 0) {
                        fprintf(log_fp, ",Net_%s_RX_Bps,Net_%s_TX_Bps", ifname, ifname);
                    }
                }
            }
            fclose(f);
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
    
    /* Log CPU */
    if (show_cpu) {
        cpu_stats_t curr_total;
        cpu_stats_t *curr_cores = calloc(num_cores, sizeof(cpu_stats_t));
        char line[512];
        
        FILE *f = fopen("/proc/stat", "r");
        if (f) {
            int core_idx = 0;
            while (fgets(line, sizeof(line), f)) {
                if (strncmp(line, "cpu ", 4) == 0) {
                    parse_cpu_line(line, &curr_total);
                } else if (strncmp(line, "cpu", 3) == 0 && isdigit(line[3])) {
                    if (core_idx < num_cores) {
                        parse_cpu_line(line, &curr_cores[core_idx]);
                        core_idx++;
                    }
                }
            }
            fclose(f);
            
            double total_pct = calculate_cpu_percent(&curr_total, &cpu_prev_total);
            fprintf(log_fp, ",%.2f", total_pct);
            
            for (int i = 0; i < num_cores; i++) {
                double pct = calculate_cpu_percent(&curr_cores[i], &cpu_prev_cores[i]);
                fprintf(log_fp, ",%.2f", pct);
            }
        }
        free(curr_cores);
    }
    
    /* Log Memory */
    if (show_mem) {
        FILE *f = fopen("/proc/meminfo", "r");
        if (f) {
            char line[256];
            unsigned long mem_total = 0, mem_available = 0;
            unsigned long swap_total = 0, swap_free = 0;
            
            while (fgets(line, sizeof(line), f)) {
                if (sscanf(line, "MemTotal: %lu kB", &mem_total) == 1) {}
                if (sscanf(line, "MemAvailable: %lu kB", &mem_available) == 1) {}
                if (sscanf(line, "SwapTotal: %lu kB", &swap_total) == 1) {}
                if (sscanf(line, "SwapFree: %lu kB", &swap_free) == 1) {}
            }
            fclose(f);
            
            if (mem_available == 0) mem_available = 0;
            
            double total_bytes = (double)mem_total * 1024;
            double used_bytes = total_bytes - ((double)mem_available * 1024);
            double pct = (total_bytes > 0) ? (used_bytes / total_bytes) * 100.0 : 0.0;
            
            double sw_total = (double)swap_total * 1024;
            double sw_used = sw_total - ((double)swap_free * 1024);
            double sw_pct = (sw_total > 0) ? (sw_used / sw_total) * 100.0 : 0.0;
            
            fprintf(log_fp, ",%.0f,%.0f,%.2f,%.0f,%.0f,%.2f", used_bytes, total_bytes, pct, sw_used, sw_total, sw_pct);
        }
    }
    
    /* Log Disks */
    if (show_disks) {
        FILE *mtab = setmntent("/proc/mounts", "r");
        if (mtab) {
            struct mntent *ent;
            while ((ent = getmntent(mtab)) != NULL) {
                if (strncmp(ent->mnt_fsname, "/dev/", 5) == 0 && strstr(ent->mnt_fsname, "loop") == NULL) {
                    struct statvfs s;
                    if (statvfs(ent->mnt_dir, &s) == 0) {
                        unsigned long long total = s.f_blocks * s.f_frsize;
                        unsigned long long free = s.f_bfree * s.f_frsize;
                        unsigned long long used = total - free;
                        double pct = (total > 0) ? ((double)used / total) * 100.0 : 0.0;
                        fprintf(log_fp, ",%llu,%llu,%.2f", used, total, pct);
                    } else {
                        fprintf(log_fp, ",0,0,0");
                    }
                }
            }
            endmntent(mtab);
        }
    }
    
    /* Log Network */
    if (show_net) {
        double curr_time = get_time_sec();
        double dt = curr_time - last_net_time;
        int first_run = (last_net_time == 0);
        
        FILE *f = fopen("/proc/net/dev", "r");
        if (f) {
            char line[512];
            fgets(line, sizeof(line), f);
            fgets(line, sizeof(line), f);
            
            while (fgets(line, sizeof(line), f)) {
                char *p = line;
                while (*p == ' ') p++;
                char *end = strchr(p, ':');
                if (end) {
                    *end = '\0';
                    char *ifname = p;
                    
                    if (opt_net_iface == NULL || strcmp(opt_net_iface, ifname) == 0) {
                        p = end + 1;
                        unsigned long long r_bytes, s_bytes;
                        sscanf(p, "%llu %*s %*s %*s %*s %*s %*s %*s %llu", &r_bytes, &s_bytes);
                        
                        if (!first_run && dt > 0) {
                            for (int i = 0; i < net_prev_count; i++) {
                                if (strcmp(net_prev[i].name, ifname) == 0) {
                                    double rx_spd = (r_bytes - net_prev[i].bytes_recv) / dt;
                                    double tx_spd = (s_bytes - net_prev[i].bytes_sent) / dt;
                                    fprintf(log_fp, ",%.2f,%.2f", rx_spd, tx_spd);
                                    break;
                                }
                            }
                        } else {
                            fprintf(log_fp, ",0,0");
                        }
                    }
                }
            }
            fclose(f);
        }
    }
    
    fprintf(log_fp, "\n");
    fflush(log_fp);
}

void print_help(void) {
    printf("umon - System Resource Monitor for Linux\n");
    printf("Version: %s\n", __CODEVERSION__);
    printf("Author: %s\n", __CODEAUTHOR__);
    printf("Date: %s\n", __CODEDATE__);
    printf("Repository: %s\n\n", __CODEGIT__);
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
}

void print_oneline_help(void) {
    printf("umon v%s - System Resource Monitor by %s\n", __CODEVERSION__, __CODEAUTHOR__);
    printf("Usage: umon [--cpu] [--mem] [--disks] [--net [IFACE]] [--mono] [--interval MS] [--log FILE] [--sysinfo] | -h | --help\n");
    printf("Use '--help' for detailed information.\n");
}

/* Signal Handling */
void cleanup(void) {
    if (!opt_mono) {
        printf("\033[?25h");
        printf("\033[?1049l");
        fflush(stdout);
    }
    if (log_fp) {
        fclose(log_fp);
        log_fp = NULL;
    }
}

void handle_signal(int sig) {
    (void)sig;
    cleanup();
    printf("\n\nMonitoring stopped.\n");
    if (opt_log) {
        printf("Log saved to: %s\n", opt_log);
    }
    exit(0);
}

int main(int argc, char **argv) {
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
    
    if (!opt_mono) {
        printf("\033[?1049h");
        printf("\033[?25l");
        fflush(stdout);
    }
    
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    atexit(cleanup);
    
    init_cpu_stats();
    
    int first_run = 1;
    
    while (1) {
        printf("\033[H\033[2J");
        
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        char time_str[10];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);
        
        char title[128];
        snprintf(title, sizeof(title), "System Monitor (v%s)", __CODEVERSION__);
        
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
            get_cpu_info(30);
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
        
        usleep(opt_interval * 1000);
        first_run = 0;
    }
    
    return 0;
}
