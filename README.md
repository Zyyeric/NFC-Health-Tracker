# nRF52 Health Monitor

Embedded system designed for real-time health monitoring using the **nRF52** microcontroller. It collects biometric data from a **PulseSensor** and a **MAX30102 temperature module**, displaying results on a **TFT LCD touchscreen**.

## Prerequisites

* Install the ARM GCC toolchain:
  ```
  # For Ubuntu/Debian
  sudo apt-get install gcc-arm-none-eabi
  
  # For macOS
  brew install --cask gcc-arm-embedded
  
  # For Windows
  # Download from https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads
  ```

* Install OpenOCD for JTAG debugging and flashing:
  ```
  # For Ubuntu/Debian
  sudo apt-get install openocd
  
  # For macOS
  brew install openocd
  
  # For Windows
  # Download from https://openocd.org/pages/getting-openocd.html
  ```

## Building and Flashing

Build and flash the firmware in a single step using:
```
make flash
```