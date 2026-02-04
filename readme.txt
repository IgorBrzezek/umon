MicroMon (umon)

A lightweight, htop-like system monitoring utility available in both Python and C implementations. This tool provides real-time insights into your system's CPU, memory (RAM and SWAP), disk usage, and network activity directly from your terminal. It's designed to be simple, efficient, portable, and customizable with various command-line options.

Additional Information:
  Author: Igor Brzezek <igor.brzezek@gmail.com>
  GitHub: github.com/igorbrzezek
  Python Version: 0.0.3 (2026-02-04)
  C Version: 0.0.3 (2026-02-04)

Features:
- Real-time CPU Usage: Monitor overall CPU utilization and individual core usage.
- Per-Core CPU List: Display CPU cores as a vertical list with individual progress bars.
- Memory Overview: Track RAM and SWAP usage with clear percentage indicators.
- Disk Usage: View disk space consumption for all mounted partitions.
- Network Monitoring: Display network interface usage, upload/download speeds.
- Network Interface List: View detailed information about all network interfaces.
- System Information: Display detailed system and environment information.
- Color-coded Output: Visual cues (green <33%, yellow 34-66%, red >66%).
- Customizable Refresh Rate: Adjust update frequency (default: 250ms).
- Selective Display: Monitor only CPU, Memory, Disks, or Network.
- Monochrome Mode: Disable colors for terminals without ANSI support.
- CSV Logging: Export monitoring data to CSV files for analysis.
- Cross-Platform: Python works on Linux, macOS, Windows. C version for Windows.

PYTHON VERSION (umon.py)

Installation:

To run the Python version, you need Python 3 and the 'psutil' library. For color output, the 'colorama' library is also recommended, especially on Windows.

1. Ensure you have Python 3 installed from python.org
2. Install required Python packages:
   pip install psutil colorama

Usage:
  python umon.py [options]

Python Examples:

Default run (show CPU, Memory, Disks with colors, 250ms refresh):
  python umon.py

Show only CPU information:
  python umon.py --cpu

Show only memory (RAM and SWAP) information:
  python umon.py --mem

Show only disk usage information:
  python umon.py --disks

Show only network usage information:
  python umon.py --net

Show network usage for a specific interface (e.g., Ethernet):
  python umon.py --net Ethernet

Show CPU cores as a list with individual bars:
  python umon.py --cpulist

Run in monochrome mode (no colors):
  python umon.py --mono

Set a custom refresh interval (milliseconds):
  python umon.py --interval 100

Log data to CSV file:
  python umon.py --log system_monitoring.csv

Combine options (CPU only with 500ms refresh and logging):
  python umon.py --cpu --interval 500 --log cpu_data.csv

Display detailed system information and exit:
  python umon.py --sysinfo

Display network interface list and exit:
  python umon.py --netlist

View brief one-line help:
  python umon.py -h

View detailed help:
  python umon.py --help

C VERSION (umon_win.c / umon.exe)

The C version is optimized for Windows and provides a portable executable with no external dependencies.

Features of C Version:
- Statically Linked: Single executable file with all libraries built-in
- Portable: No installation required, works on any Windows system
- Fast: Native compiled code for optimal performance
- Color Support: Full ANSI color support on Windows 10+ (version 1511 or later)
- All Python Features: Includes all monitoring capabilities of the Python version

Compilation:

Using MinGW-w64 (Recommended):

Standard compilation:
  gcc -O2 -Wall -o umon.exe umon_win.c -liphlpapi -lpsapi -lnetapi32 -ladvapi32 -lws2_32

Static linking (portable, no DLL dependencies):
  gcc -O2 -Wall -static -o umon.exe umon_win.c -liphlpapi -lpsapi -lnetapi32 -ladvapi32 -lws2_32

Using Visual Studio:

Standard compilation:
  cl /O2 /W3 umon_win.c iphlpapi.lib psapi.lib netapi32.lib advapi32.lib ws2_32.lib

Static linking:
  cl /O2 /W3 /MT umon_win.c iphlpapi.lib psapi.lib netapi32.lib advapi32.lib ws2_32.lib

Cross-compilation from Linux:

Install MinGW-w64:
  sudo apt-get install mingw-w64

Compile for Windows 64-bit:
  x86_64-w64-mingw32-gcc -O2 -Wall -static -o umon.exe umon_win.c -liphlpapi -lpsapi -lnetapi32 -ladvapi32 -lws2_32

Compile for Windows 32-bit:
  i686-w64-mingw32-gcc -O2 -Wall -static -o umon32.exe umon_win.c -liphlpapi -lpsapi -lnetapi32 -ladvapi32 -lws2_32

C Version Usage:
  umon.exe [options]

C Version Examples:

Default run (show CPU, Memory, Disks with colors):
  umon.exe

Show only CPU information:
  umon.exe --cpu

Show CPU cores as a list:
  umon.exe --cpulist

Show only memory:
  umon.exe --mem

Show only disk usage:
  umon.exe --disks

Show only network usage:
  umon.exe --net

Show network usage for a specific interface:
  umon.exe --net "Wi-Fi"

Run in monochrome mode (no colors):
  umon.exe --mono

Set a custom refresh interval (milliseconds):
  umon.exe --interval 1000

Log data to CSV file:
  umon.exe --cpu --mem --log monitoring.csv

Display system information and exit:
  umon.exe --sysinfo

Display network interface list and exit:
  umon.exe --netlist

View brief help:
  umon.exe -h

View detailed help:
  umon.exe --help

Command-line Options (Both Versions):

  -h                     Show program info and options in one line.
  --help                 Show detailed help message with all options and examples.
  --cpu                  Display only CPU usage information.
  --mem                  Display only memory (RAM and SWAP) usage information.
  --disks                Display only disk usage information for all mounted partitions.
  --net [INTERFACE]      Display only network usage information. Optionally specify an INTERFACE name.
  --netlist              Display network interface list and exit.
  --cpulist              Show CPU cores as a list with individual progress bars.
  --mono                 Run in monochrome mode, disabling all ANSI color output.
  --interval MS          Set the refresh interval for updating statistics, in milliseconds (default: 250ms, minimum: 50ms).
  --sysinfo              Display detailed system information and exit.
  --log FILENAME         Log gathered data to a CSV file. Data is written at each refresh interval.

Color Coding:

Both versions use color-coded progress bars:
- Green (< 33%): Low usage - System is healthy
- Yellow (34% - 66%): Medium usage - Monitor closely
- Red (> 66%): High usage - Attention needed

Logging to CSV:

The --log option allows you to save monitoring data to a CSV file for later analysis. The log includes:
- Timestamp for each reading
- CPU usage percentages (total and per-core)
- Memory usage (RAM and SWAP)
- Disk usage for each mounted partition
- Network speeds (RX/TX) for each interface

Example:
  python umon.py --log system_data.csv
  umon.exe --cpu --mem --interval 1000 --log performance.csv

Notes:

Python Version:
- Color output relies on ANSI escape codes. On Windows, the 'colorama' library translates these codes.
- Windows Terminal is recommended over older Command Prompt or PowerShell versions.
- 'psutil' is essential. The script will exit with an error if not installed.

C Version (Windows):
- Colors require Windows 10 version 1511 or later. Use --mono if colors don't display correctly.
- The statically compiled version has no external DLL dependencies.
- Press Ctrl+C to stop the monitoring loop.

General:
- Network monitoring requires at least two samples to calculate speeds; initial run may show "Waiting for first sample...".
- CPU usage is calculated using averaged samples over multiple readings for stability.

Changelog:

Version 0.0.3 (2026-02-04) - Python:
- Added --log option to export monitoring data to CSV files.
- Improved data collection structure for better extensibility.
- Fixed various display formatting issues.
- Version bump for consistency with feature set.

Version 0.0.2 (2026-01-21) - C Version Release:
- Initial C version release for Windows with full feature parity to Python version.
- Added statically linked portable executable support.
- Added --cpulist option to display CPU cores as a vertical list with bars.
- Improved formatting of percentage display in progress bars.
- Updated program title to include version number in the header.
- Enhanced network monitoring display: shows download (DN) and upload (UP) rates.
- Added stability improvements for CPU measurements using averaged samples.
- Fixed network interface filtering logic.
- Added --log support for CSV export in C version.

Version 0.0.1 (Earlier):
- Initial release with basic CPU, memory, and disk monitoring.
- Color-coded output support.
- Customizable refresh intervals.
- Network monitoring capabilities.

License:
This project is open source. Feel free to use, modify, and distribute.

Contributing:
Contributions are welcome! Please feel free to submit issues or pull requests on GitHub.
