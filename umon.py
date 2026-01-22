#!/usr/bin/env python3
"""
System Monitor - htop-like utility written in Python
Monitors CPU and memory usage with color support and TUI
"""

"""
CHANGELOG
"""

import argparse
import sys
import time
import os
import platform  # Added platform import
import ctypes  # Added ctypes import
import socket  # Added socket import
from datetime import datetime

# === AUTHOR =================================================
__AUTHOR__ = 'Igor Brzezek'
__AUTHOR_EMAIL__ = 'igor.brzezek@gmail.com'
__AUTHOR_GITHUB__ = 'github.com/igorbrzezek'
__VERSION__ = "0.0.2"
__DATE__ = '21.01.2026'
# ============================================================

# Fix Unicode encoding on Windows
if sys.platform == 'win32':
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

try:
    import psutil
except ImportError:
    print("Error: psutil is required. Install it with: pip install psutil")
    sys.exit(1)

# Enable ANSI colors on Windows
try:
    from colorama import init
    init(autoreset=False)
except ImportError:
    pass


class Colors:
    """ANSI color codes"""
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    WHITE = '\033[97m'
    BOLD = '\033[1m'
    DIM = '\033[2m'
    RESET = '\033[0m'


def get_color_for_percentage(percentage, use_color=True):
    """Return appropriate color based on percentage"""
    if not use_color:
        return ''

    if percentage < 33:
        return Colors.GREEN
    elif percentage < 66:
        return Colors.YELLOW
    else:
        return Colors.RED


def draw_bar_ascii(value, max_val=100, width=20, use_color=True):
    """Draw an ASCII progress bar"""
    percentage = (value / max_val) * 100
    filled = int((percentage / 100) * width)

    color = get_color_for_percentage(percentage, use_color)
    reset = Colors.RESET if use_color else ''
    cyan = Colors.CYAN if use_color else ''
    white = Colors.WHITE if use_color else ''

    filled_part = f"{color}{'#' * filled}{reset}"
    empty_part = f"{white}{'-' * (width - filled)}{reset}"

    return f"{cyan}[{reset}{filled_part}{empty_part}{cyan}]{reset} {percentage:.1f}%"


def format_bytes(bytes_val):
    """Format bytes to human-readable format"""
    for unit in ['B', 'KB', 'MB', 'GB', 'TB']:
        if bytes_val < 1024:
            return f"{bytes_val:.1f}{unit}"
        bytes_val /= 1024
    return f"{bytes_val:.1f}PB"


def display_sysinfo():
    """Displays comprehensive system information and then exits."""
    info_lines = []

    info_lines.append(f"{Colors.BOLD}System Information:{Colors.RESET}")
    info_lines.append(f"  OS Name: {platform.system()}")
    info_lines.append(f"  OS Release: {platform.release()}")
    info_lines.append(f"  OS Version: {platform.version()}")
    info_lines.append(f"  Machine: {platform.machine()}")
    info_lines.append(f"  Processor: {platform.processor()}")
    info_lines.append(f"  Python Version: {platform.python_version()}")
    info_lines.append(f"  Python Build: {platform.python_build()}")
    info_lines.append(f"  Platform: {platform.platform()}")

    if psutil:
        try:
            info_lines.append(
                f"\n{Colors.BOLD}CPU Information (via psutil):{Colors.RESET}")
            info_lines.append(
                f"  Physical Cores: {psutil.cpu_count(logical=False)}")
            info_lines.append(
                f"  Logical Cores: {psutil.cpu_count(logical=True)}")
            cpu_freq = psutil.cpu_freq()
            if cpu_freq:
                info_lines.append(
                    f"  Current Freq: {cpu_freq.current:.2f} Mhz")
                info_lines.append(f"  Min Freq: {cpu_freq.min:.2f} Mhz")
                info_lines.append(f"  Max Freq: {cpu_freq.max:.2f} Mhz")

            info_lines.append(
                f"\n{Colors.BOLD}Memory Information (via psutil):{Colors.RESET}")
            vm = psutil.virtual_memory()
            info_lines.append(f"  Total: {format_bytes(vm.total)}")
            info_lines.append(f"  Available: {format_bytes(vm.available)}")
            info_lines.append(f"  Used: {format_bytes(vm.used)}")
            info_lines.append(f"  Percentage: {vm.percent:.2f}%")

            sm = psutil.swap_memory()
            info_lines.append(
                f"\n{Colors.BOLD}Swap Information (via psutil):{Colors.RESET}")
            info_lines.append(f"  Total: {format_bytes(sm.total)}")
            info_lines.append(f"  Used: {format_bytes(sm.used)}")
            info_lines.append(f"  Free: {format_bytes(sm.free)}")
            info_lines.append(f"  Percentage: {sm.percent:.2f}%")

        except Exception as e:
            info_lines.append(f"  (Error gathering psutil info: {e})")
    else:
        info_lines.append("\n(psutil not available for detailed info)")

    for line in info_lines:
        print(line)
    sys.exit(0)  # Exit after displaying sysinfo


def display_netlist():
    """Displays information about all network interfaces and then exits."""
    info_lines = []
    info_lines.append(f"{Colors.BOLD}Network Interfaces:{Colors.RESET}")

    interfaces = psutil.net_if_addrs()
    stats = psutil.net_if_stats()

    if not interfaces:
        info_lines.append("  No network interfaces found.")
    else:
        for iface_name, iface_addrs in interfaces.items():
            info_lines.append(
                f"\n{Colors.BLUE}  Interface: {iface_name}{Colors.RESET}")

            # Display status
            if iface_name in stats:
                stat = stats[iface_name]
                info_lines.append(
                    f"    Status: {'Up' if stat.isup else 'Down'}")
                info_lines.append(
                    f"    Duplex: {str(stat.duplex).split('.')[-1].capitalize() if stat.duplex else 'N/A'}")
                info_lines.append(f"    Speed: {stat.speed} Mbps")
                info_lines.append(f"    MTU: {stat.mtu}")
            else:
                info_lines.append("    Status: N/A (No stats available)")

            # Display addresses
            for addr in iface_addrs:
                if addr.family == psutil.AF_LINK:  # MAC address
                    info_lines.append(f"    MAC Address: {addr.address}")
                elif addr.family == socket.AF_INET:  # IPv4
                    info_lines.append(f"    IPv4 Address: {addr.address}")
                    info_lines.append(f"    Netmask: {addr.netmask}")
                    info_lines.append(f"    Broadcast: {addr.broadcast}")
                elif addr.family == socket.AF_INET6:  # IPv6
                    info_lines.append(f"    IPv6 Address: {addr.address}")
                    info_lines.append(f"    IPv6 Netmask: {addr.netmask}")

    for line in info_lines:
        print(line)
    sys.exit(0)


def get_stable_cpu_percent(samples=3, delay=0.1):
    """Get averaged CPU percent for stability"""
    percents = []
    for _ in range(samples):
        percents.append(psutil.cpu_percent(interval=None))
        time.sleep(delay)
    return sum(percents) / len(percents) if percents else 0


def get_stable_per_core(samples=3, delay=0.1):
    """Get averaged per-core CPU percent"""
    cores_list = []
    for _ in range(samples):
        cores = psutil.cpu_percent(interval=None, percpu=True)
        if not cores_list:
            cores_list = [[] for _ in cores]
        for i, p in enumerate(cores):
            cores_list[i].append(p)
        time.sleep(delay)
    return [sum(core) / len(core) for core in cores_list]


def get_cpu_info(use_color=True, bar_width=20, mode='normal'):
    """Get CPU information - minimal"""
    # Use averaged readings for stability
    cpu_percent = get_stable_cpu_percent()
    cpu_count = psutil.cpu_count(logical=True)

    blue = Colors.BLUE if use_color else ''
    white = Colors.WHITE if use_color else ''
    reset = Colors.RESET if use_color else ''

    if mode == 'list':
        lines = []
        per_core = get_stable_per_core()
        for i, percent in enumerate(per_core):
            lines.append(f"CPU {i:2d}: {draw_bar_ascii(percent, 100, bar_width, use_color)}")
        return lines
    else:
        lines = []
        lines.append(
            f"{blue}CPU{reset} ({cpu_count} cores): {draw_bar_ascii(cpu_percent, 100, bar_width, use_color)}")

        # Per-core usage - show all cores with individual bars
        per_core = get_stable_per_core()
        cores_per_row = 3

        for row_start in range(0, len(per_core), cores_per_row):
            row_end = min(row_start + cores_per_row, len(per_core))
            cores_line = ""

            for i in range(row_start, row_end):
                core_percent = per_core[i]
                bar = draw_bar_ascii(core_percent, 100, bar_width, use_color)
                cores_line += f"{white}#{i:2d}:{reset}{bar}  "

            lines.append(cores_line.rstrip())
        
        return lines


def get_memory_info(use_color=True, bar_width=40):
    """Get memory information - minimal"""
    ram = psutil.virtual_memory()
    swap = psutil.swap_memory()

    green = Colors.GREEN if use_color else ''
    yellow = Colors.YELLOW if use_color else ''
    white = Colors.WHITE if use_color else ''
    reset = Colors.RESET if use_color else ''

    lines = []
    lines.append(
        f'{"RAM:  ":<8}{draw_bar_ascii(ram.percent, 100, bar_width, use_color)} {white}{format_bytes(ram.used)}/{format_bytes(ram.total)}{reset}')
    lines.append(f'{"SWAP: ":<8}{draw_bar_ascii(swap.percent, 100, bar_width, use_color)} {white}{format_bytes(swap.used)}/{format_bytes(swap.total)}{reset}')

    return lines


def get_disk_info(use_color=True, bar_width=40):
    """Get disk usage information - minimal"""
    partitions = psutil.disk_partitions()
    cyan = Colors.CYAN if use_color else ''
    white = Colors.WHITE if use_color else ''
    reset = Colors.RESET if use_color else ''

    lines = []
    for partition in partitions:
        try:
            usage = psutil.disk_usage(partition.mountpoint)
            percent = usage.percent
            lines.append(f"{cyan}{partition.device}{reset} ({partition.mountpoint}): {draw_bar_ascii(percent, 100, bar_width, use_color)} {white}{format_bytes(usage.used)}/{format_bytes(usage.total)}{reset}")
        except PermissionError:
            # Skip partitions we can't access
            continue

    return lines


def get_net_info(use_color=True, bar_width=40, interface_filter=None):
    """Get network usage information - minimal"""
    global _LAST_NET_IO_COUNTERS
    global _LAST_NET_TIME

    current_net_io = psutil.net_io_counters(pernic=True)
    current_time = time.time()

    lines = []
    magenta = Colors.MAGENTA if use_color else ''
    white = Colors.WHITE if use_color else ''
    reset = Colors.RESET if use_color else ''

    if _LAST_NET_IO_COUNTERS is None or _LAST_NET_TIME is None:
        _LAST_NET_IO_COUNTERS = current_net_io
        _LAST_NET_TIME = current_time
        lines.append(f"{magenta}NET{reset}:  Waiting for first sample...")
        return lines

    time_diff = current_time - _LAST_NET_TIME
    if time_diff == 0:
        lines.append(
            f"{magenta}NET{reset}:  Time difference is zero, cannot calculate speed.")
        _LAST_NET_IO_COUNTERS = current_net_io  # Update for next iteration
        _LAST_NET_TIME = current_time  # Update for next iteration
        return lines

    interfaces_to_show = []
    if interface_filter and interface_filter is not True:  # If a specific interface name is provided
        if interface_filter in current_net_io:
            interfaces_to_show.append(interface_filter)
        else:
            lines.append(
                f"{Colors.RED}Error: Interface '{interface_filter}' not found.{reset}")
    else:  # Show all interfaces
        interfaces_to_show = current_net_io.keys()

    for interface in interfaces_to_show:
        if interface in _LAST_NET_IO_COUNTERS and interface in current_net_io:
            current_stats = current_net_io[interface]
            last_stats = _LAST_NET_IO_COUNTERS[interface]

            # Bytes transferred
            bytes_sent_diff = current_stats.bytes_sent - last_stats.bytes_sent
            bytes_recv_diff = current_stats.bytes_recv - last_stats.bytes_recv

            # Speed in Bytes/second
            upload_speed_bps = bytes_sent_diff / time_diff
            download_speed_bps = bytes_recv_diff / time_diff

            # Get interface speed for percentage calculation
            net_stats = psutil.net_if_stats()
            # Get interface speed and determine label suffix
            link_speed_mbps = 0
            link_speed_bps = 0
            link_speed_label_suffix = ""

            if interface in net_stats and net_stats[interface].speed:
                    link_speed_mbps = net_stats[interface].speed  # in Mbps
                    link_speed_bps = link_speed_mbps * 125000
                    link_speed_label_suffix = f" ({link_speed_mbps} Mbps)"
            else:
                link_speed_label_suffix = " (Unknown Speed)"

            upload_percent = 0
            download_percent = 0
            if link_speed_bps > 0:
                 upload_percent = (upload_speed_bps / link_speed_bps) * 100
                 download_percent = (download_speed_bps / link_speed_bps) * 100
                 upload_percent = min(upload_percent, 100.0)
                 download_percent = min(download_percent, 100.0)

                 lines.append(f'{"DN:":<8}{draw_bar_ascii(download_percent, 100, bar_width, use_color)} {white}{format_bytes(download_speed_bps)}/s{reset}')
                 lines.append(f'{"UP:":<8}{draw_bar_ascii(upload_percent, 100, bar_width, use_color)} {white}{format_bytes(upload_speed_bps)}/s{reset}')

    _LAST_NET_IO_COUNTERS = current_net_io
    _LAST_NET_TIME = current_time
    return lines


def display_monitor_minimal(show_cpu=True, show_mem=True, show_disks=True, show_net=False, interval=250, use_color=True, show_cpulist=False):
    """Main monitoring loop - minimal version"""
    interval_sec = interval / 1000.0
    first_run = True

    try:
        # Enter alternate screen buffer to avoid scrolling main terminal
        sys.stdout.write('\033[?1049h')
        # Hide cursor
        sys.stdout.write('\033[?25l')
        sys.stdout.flush()

        while True:
            lines = []

            # Title
            magenta = Colors.MAGENTA if use_color else ''
            white = Colors.WHITE if use_color else ''
            bold = Colors.BOLD if use_color else ''
            dim = Colors.DIM if use_color else ''
            reset = Colors.RESET if use_color else ''

            program_title = f"System Monitor (v{VERSION})"
            current_time = datetime.now().strftime('%H:%M:%S')

            # Calculate padding for centering
            total_width = 60 # width of the '=' bar
            title_time_str = f"{program_title} {current_time}"
            padding_needed = total_width - len(title_time_str)
            left_padding = padding_needed // 2
            right_padding = total_width - len(title_time_str) - left_padding

            lines.append(f"{magenta}{'=' * total_width}{reset}")
            lines.append(f"{bold}{Colors.CYAN}{' ' * left_padding}{program_title}{reset} {dim}{white}{current_time}{reset}{' ' * right_padding}{bold}{Colors.CYAN}{reset}")
            lines.append(f"{magenta}{'=' * total_width}{reset}")

            # CPU info
            if show_cpu:
                lines.append("")
                mode = 'list' if show_cpulist else 'normal'
                lines.extend(get_cpu_info(use_color, bar_width=30, mode=mode))

            # Memory info
            if show_mem:
                lines.append("")
                lines.extend(get_memory_info(use_color, bar_width=30))

            # Disk info
            if show_disks:
                lines.append("")
                lines.extend(get_disk_info(use_color, bar_width=30))

            # Network info
            if show_net: # show_net can be True or an interface name
                lines.append("")
                lines.extend(get_net_info(use_color, bar_width=30, interface_filter=show_net))

            lines.append("")
            lines.append("Press Ctrl+C to quit.")

            # Build output
            output = '\n'.join(lines)

            # Always clear screen and position to home, then write output
            sys.stdout.write('\033[H\033[2J')  # Home and clear entire screen
            sys.stdout.write(output + '\n')

            if first_run:
                first_run = False

            sys.stdout.flush()
            time.sleep(interval_sec)

    except KeyboardInterrupt:
        # Show cursor again
        sys.stdout.write('\033[?25h')
        # Exit alternate screen buffer
        sys.stdout.write('\033[?1049l')
        sys.stdout.flush()
        print("\n\nMonitoring stopped. Press Ctrl-C again to terminate forcefully.")
        sys.exit(0)


def main():
    # Custom action to handle -h with short help
    class OneLineHelpAction(argparse.Action):
        def __init__(self, option_strings, dest, **kwargs):
            super().__init__(option_strings, dest, nargs=0, **kwargs)
        
        def __call__(self, parser, namespace, values, option_string=None):
            version_info = f"umon.py v{__VERSION__} by {__AUTHOR__} ({__DATE__})"
            options_summary = "[--cpu] [--mem] [--disks] [--mono] [--interval MS] [--sysinfo]"
            print(f"Usage: python umon.py {options_summary} | -h | --help")
            print(f"Info: {version_info}. For detailed help, use 'python umon.py --help'.")
            parser.exit()
    
    parser = argparse.ArgumentParser(
        prog='umon.py',
        description='System Monitor - A htop-like utility written in Python for monitoring CPU and memory usage with color support and TUI.',
        formatter_class=argparse.RawTextHelpFormatter,
        add_help=False,
        epilog=f"""
Examples:
  python umon.py                   Show CPU, memory, and disks (default, colorful)
  python umon.py --cpu             Show only CPU information
  python umon.py --mem             Show only memory (RAM and SWAP) information
  python umon.py --disks           Show only disk usage information
  python umon.py --net             Show only network usage information
  python umon.py --netlist         List all network interfaces and their details, then exit
  python umon.py --interval 100    Update every 100 milliseconds
  python umon.py --mono            Monochrome mode (no colors)
  python umon.py --cpu --interval 500  CPU only with 500ms refresh rate
  python umon.py --sysinfo         Display detailed system information and exit

Additional Information:
  Author: {__AUTHOR__} <{__AUTHOR_EMAIL__}>
  GitHub: {__AUTHOR_GITHUB__}
  Version: {__VERSION__}
  Date: {__DATE__}
        """
    )

    parser.add_argument(
        '-h',
        action=OneLineHelpAction,
        help='Show program info and options in one line.'
    )

    parser.add_argument(
        '--help',
        action='help',
        default=argparse.SUPPRESS,
        help='Show this detailed help message, including descriptions of all options and examples.'
    )

    parser.add_argument(
        '--cpu',
        action='store_true',
        help='Display only CPU usage information.'
    )

    parser.add_argument(
        '--mem',
        action='store_true',
        help='Display only memory (RAM and SWAP) usage information.'
    )

    parser.add_argument(
        '--disks',
        action='store_true',
        help='Display only disk usage information for all mounted partitions.'
    )

    parser.add_argument(
        '--net',
        nargs='?', # 0 or 1 argument
        const=True, # Value if --net is present without argument
        default=False, # Value if --net is not present
        metavar='INTERFACE',
        help='Display only network usage information. Optionally specify an INTERFACE name to monitor a specific one (e.g., --net Ethernet). If no interface is specified, all will be shown.'
    )

    parser.add_argument(
        '--netlist',
        action='store_true',
        help='Display network interface list and exit.'
    )
    parser.add_argument(
        '--cpulist',
        action='store_true',
        help='Show CPU cores as a list with bars.'
    )

    parser.add_argument(
        '--mono',
        action='store_true',
        help='Run in monochrome mode, disabling all ANSI color output.'
    )

    parser.add_argument(
        '--interval',
        type=int,
        default=250,
        help='Set the refresh interval for updating statistics, in milliseconds (default: 250ms). Minimum 50ms.'
    )

    parser.add_argument(
        '--sysinfo',
        action='store_true',
        help='Display detailed system information and exit.'
    )

    args = parser.parse_args()
    
    if args.sysinfo:
        display_sysinfo()
    
    if args.netlist:
        display_netlist()

    # Determine what to show
    # If no specific display options are selected, show all (CPU, MEM, DISKS, NET)
    any_specific_display_selected = args.cpu or args.mem or args.disks or args.net is not False or args.cpulist
    
    show_cpu = args.cpu or args.cpulist or not any_specific_display_selected
    show_mem = args.mem or not any_specific_display_selected
    show_disks = args.disks or not any_specific_display_selected
    show_net = args.net or (not any_specific_display_selected and args.net is False) # show_net will be True or an interface name, or False

    # Determine display options
    use_color = not args.mono

    # Validate interval
    if args.interval < 50:
        print("Warning: interval should be at least 50ms. Setting to 50ms.")
        args.interval = 50

    # Start monitoring
    display_monitor_minimal(show_cpu, show_mem, show_disks, show_net, args.interval, use_color, show_cpulist=args.cpulist)


_LAST_NET_IO_COUNTERS = None
_LAST_NET_TIME = None



if __name__ == '__main__':
    _LAST_NET_IO_COUNTERS = None
    _LAST_NET_TIME = None
    main()
