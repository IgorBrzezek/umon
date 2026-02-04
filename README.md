# MicroMon (umon)

A lightweight, htop-like system monitoring utility available in both **Python** and **C** implementations. This tool provides real-time insights into your system's CPU, memory (RAM and SWAP), disk usage, and network activity directly from your terminal. It's designed to be simple, efficient, portable, and customizable with various command-line options.

## Additional Information

-   **Author**: Igor Brzezek <igor.brzezek@gmail.com>
-   **GitHub**: [github.com/igorbrzezek](https://github.com/igorbrzezek)
-   **Python Version**: 0.0.3 (2026-02-04)
-   **C Version**: 0.0.2 (2026-01-21)

## Features

-   **Real-time CPU Usage**: Monitor overall CPU utilization and individual core usage with stable, averaged readings.
-   **Per-Core CPU List**: Display CPU cores as a vertical list with individual progress bars.
-   **Memory Overview**: Track RAM and SWAP usage with clear percentage indicators.
-   **Disk Usage**: View disk space consumption for all mounted partitions.
-   **Network Monitoring**: Display network interface usage, upload/download speeds, and percentages relative to link speed.
-   **Network Interface List**: View detailed information about all network interfaces.
-   **System Information**: A dedicated option to display detailed system and environment information.
-   **Color-coded Output**: Visual cues for resource utilization (green for <33%, yellow for 34-66%, red for >66%) for easy interpretation.
-   **Customizable Refresh Rate**: Adjust the update frequency to suit your needs (default: 250ms).
-   **Selective Display**: Choose to monitor only CPU, Memory, Disks, or Network.
-   **Monochrome Mode**: Option to disable colors for terminals that don't support ANSI codes.
-   **CSV Logging**: Export monitoring data to CSV files for later analysis.
-   **Cross-Platform**: Python version works on Linux, macOS, and Windows. C version is optimized for Windows (portable executable).

## Python Version (umon.py)

### Installation

To run the Python version, you need Python 3 and the `psutil` library. For color output, the `colorama` library is also recommended, especially on Windows.

1.  **Ensure you have Python 3 installed.** You can download it from [python.org](https://www.python.org/).
2.  **Install required Python packages** using pip:
    ```bash
    pip install psutil colorama
    ```

### Usage

Run the script from your terminal.

```bash
python umon.py [options]
```

### Python Examples

-   **Default run** (show CPU, Memory, Disks with colors, 250ms refresh):
    ```bash
    python umon.py
    ```
-   **Show only CPU information**:
    ```bash
    python umon.py --cpu
    ```
-   **Show only memory** (RAM and SWAP) information:
    ```bash
    python umon.py --mem
    ```
-   **Show only disk usage information**:
    ```bash
    python umon.py --disks
    ```
-   **Show only network usage information**:
    ```bash
    python umon.py --net
    ```
-   **Show network usage for a specific interface** (e.g., Ethernet):
    ```bash
    python umon.py --net Ethernet
    ```
-   **Show CPU cores as a list with individual bars**:
    ```bash
    python umon.py --cpulist
    ```
-   **Run in monochrome mode** (no colors):
    ```bash
    python umon.py --mono
    ```
-   **Set a custom refresh interval** (e.g., update every 100 milliseconds):
    ```bash
    python umon.py --interval 100
    ```
-   **Log data to CSV file**:
    ```bash
    python umon.py --log system_monitoring.csv
    ```
-   **Combine options** (e.g., CPU only with 500ms refresh rate and logging):
    ```bash
    python umon.py --cpu --interval 500 --log cpu_data.csv
    ```
-   **Display detailed system information and exit**:
    ```bash
    python umon.py --sysinfo
    ```
-   **Display network interface list and exit**:
    ```bash
    python umon.py --netlist
    ```
-   **View a brief one-line help message**:
    ```bash
    python umon.py -h
    ```
-   **View detailed help message** with all options and examples:
    ```bash
    python umon.py --help
    ```

## C Version (umon_win.c / umon.exe)

The C version is optimized for **Windows** and provides a portable executable with no external dependencies.

### Features of C Version

-   **Statically Linked**: Single executable file with all libraries built-in
-   **Portable**: No installation required, works on any Windows system
-   **Fast**: Native compiled code for optimal performance
-   **Color Support**: Full ANSI color support on Windows 10+ (version 1511 or later)
-   **All Python Features**: Includes all monitoring capabilities of the Python version

### Compilation

#### Using MinGW-w64 (Recommended)

```bash
# Standard compilation
gcc -O2 -Wall -o umon.exe umon_win.c -liphlpapi -lpsapi -lnetapi32 -ladvapi32 -lws2_32

# Static linking (portable, no DLL dependencies)
gcc -O2 -Wall -static -o umon.exe umon_win.c -liphlpapi -lpsapi -lnetapi32 -ladvapi32 -lws2_32
```

#### Using Visual Studio

```cmd
# Standard compilation
cl /O2 /W3 umon_win.c iphlpapi.lib psapi.lib netapi32.lib advapi32.lib ws2_32.lib

# Static linking
cl /O2 /W3 /MT umon_win.c iphlpapi.lib psapi.lib netapi32.lib advapi32.lib ws2_32.lib
```

#### Cross-compilation from Linux

```bash
# Install MinGW-w64
sudo apt-get install mingw-w64

# Compile for Windows 64-bit
x86_64-w64-mingw32-gcc -O2 -Wall -static -o umon.exe umon_win.c -liphlpapi -lpsapi -lnetapi32 -ladvapi32 -lws2_32

# Compile for Windows 32-bit
i686-w64-mingw32-gcc -O2 -Wall -static -o umon32.exe umon_win.c -liphlpapi -lpsapi -lnetapi32 -ladvapi32 -lws2_32
```

### C Version Usage

```cmd
umon.exe [options]
```

### C Version Examples

-   **Default run** (show CPU, Memory, Disks with colors):
    ```cmd
    umon.exe
    ```
-   **Show only CPU information**:
    ```cmd
    umon.exe --cpu
    ```
-   **Show CPU cores as a list**:
    ```cmd
    umon.exe --cpulist
    ```
-   **Show only memory**:
    ```cmd
    umon.exe --mem
    ```
-   **Show only disk usage**:
    ```cmd
    umon.exe --disks
    ```
-   **Show only network usage**:
    ```cmd
    umon.exe --net
    ```
-   **Show network usage for a specific interface**:
    ```cmd
    umon.exe --net "Wi-Fi"
    ```
-   **Run in monochrome mode** (no colors):
    ```cmd
    umon.exe --mono
    ```
-   **Set a custom refresh interval** (e.g., 1000ms = 1 second):
    ```cmd
    umon.exe --interval 1000
    ```
-   **Log data to CSV file**:
    ```cmd
    umon.exe --cpu --mem --log monitoring.csv
    ```
-   **Display system information and exit**:
    ```cmd
    umon.exe --sysinfo
    ```
-   **Display network interface list and exit**:
    ```cmd
    umon.exe --netlist
    ```
-   **View brief help**:
    ```cmd
    umon.exe -h
    ```
-   **View detailed help**:
    ```cmd
    umon.exe --help
    ```

## Command-line Options

### Common Options (Both Versions)

-   `-h`: Show program info and options in one line.
-   `--help`: Show detailed help message with all options and examples.
-   `--cpu`: Display only CPU usage information.
-   `--mem`: Display only memory (RAM and SWAP) usage information.
-   `--disks`: Display only disk usage information for all mounted partitions.
-   `--net [INTERFACE]`: Display only network usage information. Optionally specify an INTERFACE name to monitor a specific one (e.g., `--net Ethernet`). If no interface is specified, all active interfaces will be shown.
-   `--netlist`: Display network interface list with details and exit.
-   `--cpulist`: Show CPU cores as a list with individual progress bars instead of grid layout.
-   `--mono`: Run in monochrome mode, disabling all ANSI color output.
-   `--interval MS`: Set the refresh interval for updating statistics, in milliseconds (default: 250ms, minimum: 50ms).
-   `--sysinfo`: Display detailed system information and exit.
-   `--log FILENAME`: Log gathered data to a CSV file. Data is written at each refresh interval.

## Color Coding

Both versions use color-coded progress bars for easy visual interpretation:

-   **Green** (`< 33%`): Low usage - System is healthy
-   **Yellow** (`34% - 66%`): Medium usage - Monitor closely
-   **Red** (`> 66%`): High usage - Attention needed

## Logging to CSV

The `--log` option allows you to save monitoring data to a CSV file for later analysis. The log includes:

-   Timestamp for each reading
-   CPU usage percentages (total and per-core)
-   Memory usage (RAM and SWAP)
-   Disk usage for each mounted partition
-   Network speeds (RX/TX) for each interface

Example:
```bash
python umon.py --log system_data.csv
# or
umon.exe --cpu --mem --interval 1000 --log performance.csv
```

## Notes

### Python Version
-   **Color Output**: Color output relies on ANSI escape codes. On Windows, the `colorama` library is used to translate these codes. If you encounter issues with colors, ensure `colorama` is installed and your terminal supports ANSI. Windows Terminal is generally recommended over older Command Prompt or PowerShell versions.
-   **Dependencies**: `psutil` is essential for this script. If it's not installed, the script will exit with an error message.

### C Version (Windows)
-   **Color Support**: Colors require Windows 10 version 1511 or later. Use `--mono` if colors are not displaying correctly on older systems.
-   **No Dependencies**: The statically compiled version has no external DLL dependencies.
-   **Exiting**: Press `Ctrl+C` to stop the monitoring loop.

### General
-   **Network Monitoring**: Network monitoring requires at least two samples to calculate speeds; initial run may show "Waiting for first sample...".
-   **Per-Core CPU**: CPU usage is calculated using averaged samples over multiple readings for stability.

## Changelog

### Version 0.0.3 (2026-02-04) - Python
-   Added `--log` option to export monitoring data to CSV files.
-   Improved data collection structure for better extensibility.
-   Fixed various display formatting issues.
-   Version bump for consistency with feature set.

### Version 0.0.2 (2026-01-21) - C Version Release
-   **Initial C version release** for Windows with full feature parity to Python version.
-   Added statically linked portable executable support.
-   Added `--cpulist` option to display CPU cores as a vertical list with bars.
-   Improved formatting of percentage display in progress bars.
-   Updated program title to include version number in the header.
-   Enhanced network monitoring display: shows download (DN) and upload (UP) rates as separate bars.
-   Added stability improvements for CPU measurements using averaged samples over multiple readings.
-   Fixed network interface filtering logic for better handling of specific interface selection.
-   Added `--log` support for CSV export in C version.
-   Minor code refactoring and improvements in display functions.

### Version 0.0.1 (Earlier)
-   Initial release with basic CPU, memory, and disk monitoring.
-   Color-coded output support.
-   Customizable refresh intervals.
-   Network monitoring capabilities.

## License

This project is open source. Feel free to use, modify, and distribute.

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests on GitHub.
