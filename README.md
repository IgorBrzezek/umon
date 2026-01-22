# MicroMon (umon)

A lightweight, htop-like system monitoring utility written in Python. This tool provides real-time insights into your system's CPU, memory (RAM and SWAP), disk usage, and network activity directly from your terminal. It's designed to be simple, efficient, and customizable with various command-line options.

## Additional Information

-   **Author**: Igor Brzezek <igor.brzezek@gmail.com>
-   **GitHub**: [github.com/igorbrzezek](https://github.com/igorbrzezek)
-   **Version**: 0.0.2
-   **Date**: 21.01.2026

## Features

-   **Real-time CPU Usage**: Monitor overall CPU utilization and individual core usage with stable, averaged readings.
-   **Memory Overview**: Track RAM and SWAP usage with clear percentage indicators.
-   **Disk Usage**: View disk space consumption for all mounted partitions.
-   **Network Monitoring**: Display network interface usage, upload/download speeds, and percentages relative to link speed.
-   **Network Interface List**: View detailed information about all network interfaces.
-   **System Information**: A dedicated option to display detailed system and Python environment information.
-   **Color-coded Output**: Visual cues for resource utilization (green, yellow, red) for easy interpretation.
-   **Customizable Refresh Rate**: Adjust the update frequency to suit your needs.
-   **Selective Display**: Choose to monitor only CPU, Memory, Disks, or Network.
-   **Monochrome Mode**: Option to disable colors for terminals that don't support ANSI codes.

## Installation

To run this program, you need Python 3 and the `psutil` library. For color output, the `colorama` library is also recommended, especially on Windows.

1.  **Ensure you have Python 3 installed.** You can download it from [python.org](https://www.python.org/).
2.  **Install required Python packages** using pip:
    ```bash
    pip install psutil colorama
    ```

## Usage

Run the script from your terminal.

### Basic Commands

-   **Default run** (show CPU, Memory, Disks, and Network with colors, 250ms refresh):
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
-   **Run in monochrome mode** (no colors):
    ```bash
    python umon.py --mono
    ```
-   **Set a custom refresh interval** (e.g., update every 100 milliseconds):
    ```bash
    python umon.py --interval 100
    ```
-   **Show CPU cores as a list with individual bars**:
    ```bash
    python umon.py --cpulist
    ```
-   **Combine options** (e.g., CPU only with 500ms refresh rate):
    ```bash
    python umon.py --cpu --interval 500
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

## Command-line Options

-   `-h`: Show program info and options in one line.
-   `--help`: Show this detailed help message, including descriptions of all options and examples.
-   `--cpu`: Display only CPU usage information.
-   `--mem`: Display only memory (RAM and SWAP) usage information.
-   `--disks`: Display only disk usage information for all mounted partitions.
-   `--net [INTERFACE]`: Display only network usage information. Optionally specify an INTERFACE name to monitor a specific one (e.g., --net Ethernet). If no interface is specified, all will be shown.
-   `--netlist`: Display network interface list and exit.
-   `--cpulist`: Show CPU cores as a list with individual bars.
-   `--mono`: Run in monochrome mode, disabling all ANSI color output.
-   `--interval INTERVAL`: Set the refresh interval for updating statistics, in milliseconds (default: 250ms).
-   `--sysinfo`: Display detailed system information and exit.


## Notes

-   **Color Output**: Color output relies on ANSI escape codes. On Windows, the `colorama` library is used to translate these codes. If you encounter issues with colors, ensure `colorama` is installed and your terminal supports ANSI. Windows Terminal is generally recommended over older Command Prompt or PowerShell versions.
-   **Exiting**: Press `Ctrl+C` to stop the monitoring loop. You might need to press it twice to forcefully terminate if the first press is caught by the script's graceful exit.
-   **Dependencies**: `psutil` is essential for this script. If it's not installed, the script will exit with an error message.
-   **Network Monitoring**: Network monitoring requires at least two samples to calculate speeds; initial run may show "Waiting for first sample...".

## Changelog

Version 0.0.2 (2026-01-21)
- Added --cpulist option to display CPU cores as a vertical list with bars instead of individual per-core bars.
- Improved formatting of percentage display in progress bars (added padding for better alignment).
- Updated program title to include version number in the header.
- Enhanced network monitoring display: now shows download (DN) and upload (UP) rates as separate bars without interface names, and only displays them if link speed information is available.
- Added stability improvements for CPU measurements using averaged samples over multiple readings.
- Fixed network interface filtering logic for better handling of specific interface selection.
- Minor code refactoring and improvements in display functions.