# MicroMon (umon)

A lightweight, htop-like system monitoring utility written in Python. This tool provides real-time insights into your system's CPU, memory (RAM and SWAP), and disk usage directly from your terminal. It's designed to be simple, efficient, and customizable with various command-line options.

## Additional Information

-   **Author**: Igor Brzezek <igor.brzezek@gmail.com>
-   **GitHub**: [github.com/igorbrzezek](https://github.com/igorbrzezek)
-   **Version**: 0.0.1
-   **Date**: 21.01.2026

## Features

-   **Real-time CPU Usage**: Monitor overall CPU utilization and individual core usage.
-   **Memory Overview**: Track RAM and SWAP usage with clear percentage indicators.
-   **Disk Usage**: View disk space consumption for all mounted partitions.
-   **Color-coded Output**: Visual cues for resource utilization (green, yellow, red) for easy interpretation.
-   **Customizable Refresh Rate**: Adjust the update frequency to suit your needs.
-   **Selective Display**: Choose to monitor only CPU, Memory, or Disks.
-   **System Information**: A dedicated option to display detailed system and Python environment information.

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

-   **Default run** (show CPU, Memory, and Disks with colors, 250ms refresh):
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
-   **Run in monochrome mode** (no colors):
    ```bash
    python umon.py --mono
    ```
-   **Set a custom refresh interval** (e.g., update every 100 milliseconds):
    ```bash
    python umon.py --interval 100
    ```
-   **Combine options** (e.g., CPU only with 500ms refresh rate):
    ```bash
    python umon.py --cpu --interval 500
    ```
-   **Display detailed system information and exit**:
    ```bash
    python umon.py --sysinfo
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
-   `--mono`: Run in monochrome mode, disabling all ANSI color output.
-   `--interval INTERVAL`: Set the refresh interval for updating statistics, in milliseconds (default: 250ms).
-   `--sysinfo`: Display detailed system information and exit.


## Notes

-   **Color Output**: Color output relies on ANSI escape codes. On Windows, the `colorama` library is used to translate these codes. If you encounter issues with colors, ensure `colorama` is installed and your terminal supports ANSI. Windows Terminal is generally recommended over older Command Prompt or PowerShell versions.
-   **Exiting**: Press `Ctrl+C` to stop the monitoring loop. You might need to press it twice to forcefully terminate if the first press is caught by the script's graceful exit.
-   **Dependencies**: `psutil` is essential for this script. If it's not installed, the script will exit with an error message.
