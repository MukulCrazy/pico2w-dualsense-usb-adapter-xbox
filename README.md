# Pico2W DualSense 5 Bridge (Xbox Mode & Rumble Edition)

> Turn a Raspberry Pi Pico2W into a high-performance wireless adapter for the DualSense (DS5) controller.

## Overview
This project enables the Raspberry Pi Pico2W to function as an elite Bluetooth bridge for the DualSense and DualSense Edge controllers, allowing wireless connectivity with enhanced haptics support. 

This customized repository introduces **native XInput conversion** to allow full, real-time haptic rumble translation when connected to an Xbox-compatible PC environment.

---

## 👑 The Main Feature: My Xbox Mode Integration
This fork re-architects the core USB layer to add a dedicated **Xbox Emulation Mode with native, real-time Rumble Translation**:
* **XInput Layout Mapping:** Converts standard DualSense buttons, directional pads, triggers, and analog sticks into a native wired Xbox 360 controller profile recognized instantly by Windows.
* **TinyUSB Vendor Class Receiver:** Leverages TinyUSB's high-level Vendor class APIs (`tud_vendor_available` / `tud_vendor_read`) to safely intercept downstream multi-motor vibration packets directly from the PC host.
* **Haptic Rumble Translation Layer:** Intercepts traditional low-frequency (heavy) and high-frequency (light) game vibration events and maps them directly onto the DualSense haptic voice coils over Bluetooth instantly.

---

## 🚀 Additional Features

- 🎮 **Full DualSense Connectivity:** Smooth wireless pairing via the Pico2W onboard Bluetooth stack.
- 🔊 **Supports HD Haptics:** Advanced audio-based vibration feedback mirroring a raw native wired connection.
- 📡 **Wireless Bluetooth Bridging:** Low-latency input parsing with an event-driven polling pipeline.
- 🎙️ **4-Channel Audio Routing:** Exposes full USB Audio device configurations recognized natively by compatible AAA PC games (PlayStation Mode only).
- 🎧 **Wireless Headset Streaming:** Channels 1 & 2 compress via an Opus encoder to stream sound directly to the controller's headphone jack (PlayStation Mode only).

---

## 📦 Getting Started

### Get the Firmware
You have two options to obtain the binary file:
* **Download a pre-built `.uf2`** — Grab the newest release build (`ds5-bridge-*.uf2`). No tools needed.
* **Build it yourself** — Follow the [Automated Windows Build Guide](#windows-11-one-command-standalone-setup) below using the customized single-command script.

### Flashing Firmware
1. Hold the **BOOTSEL** button on the Pico2W.
2. Connect the Pico2W to your computer via USB.
3. The device will mount as a USB storage device.
4. Drag and drop the `.uf2` firmware file onto the device folder.

### Pairing the Controller
1. Put the DualSense controller into Bluetooth pairing mode (*Hold PS button + Share button*).
2. Wait for the Pico2W to detect and connect.
3. Once connected, the device will appear on the host system.

***You may need to replug the Pico when the controller is in pairing mode.***

---

## 🎮 Operational Instructions

### Switching Layout Modes
* **To activate Xbox Mode:** Hold **Create + Options** (Share + Options) simultaneously for **5 seconds**. The adapter will safely reset its USB properties and reconnect as an XInput gamepad.
* **To return to PS5 Mode:** Hold the same combination for **5 seconds** again.

### Polling Rate Behavior
* **Xbox Mode:** Permanently locked to a high-performance **1000 Hz (1ms)** update interval.
* **PS5 Mode:** Features an optimized polling configuration to deliver seamless inputs while balancing the chip's heavy real-time haptic audio DSP pipeline.

---

## ⚙️ Configuration

You can modify the Pico internal settings via the web config utility.
* **For release configurations:** https://ds5.awalol.eu.org
* **For development configurations:** https://ds5-dev.awalol.eu.org

ℹ️ *Note: The Pico device will only be visible to your computer operating system after the controller is successfully connected. Some configuration behaviors depend on reconnection cycles to take effect.*

### Low-battery LED Indicator
When the connected DualSense reports its battery at or below 10% (and it is not charging), the Pico onboard LED switches from solid-on to a 1 Hz blink so you can see the warning at a glance. The LED returns to solid-on as soon as the controller is plugged in or its reported level rises again. 

The blink also fires when `disable_pico_led` is set — the warning is treated as critical and overrides the LED-off preference; the LED returns to its disabled (off) state once the battery recovers or the controller starts charging. To opt out at build time, configure with `-DENABLE_BATT_LED=OFF` (Default is ON).

### Pico W Version Properties
Pico W only has haptics support, no speaker. You can enable Pico W firmware compilation with `-DPICO_W_BUILD=ON`, or download precompiled firmware from GitHub Actions.

### USB Wake Feature
This feature is experimental. If you need this functionality, please check out the `feat/usb-wake` branch to compile it, or use the precompiled firmware from GitHub Actions under that branch. The `ds5-bridge-wake.uf2` is the firmware with this feature enabled. It is highly recommended to read issues #60 and #61 before using this feature.

### Community Forks
* Original OLED Mod 1: https://github.com/MarcelineVPQ/DS5Dongle-OLED-Edition
* Original OLED Mod 2: https://github.com/zurce/DS5Dongle-OLED

---

## ⚠️ Performance / Overclocking & Known Issues

Due to real-time Opus encoding and Bluetooth routing requirements, the Pico2W must be overclocked:
* **Voltage:** 1.2V
* **Frequency:** 320 MHz

*If your specific hardware piece fails to boot up under these conditions, you may need to increase the voltage slightly or reduce the target CPU frequency manually.*

* ⚠️ Audio may experience slight stuttering depending on signal congestion.
* ⚠️ Overclocking values are mandatory for smooth execution loop processing.

---

## 🛠️ Build Instructions

### Windows 11 (One Command Standalone Setup)
You don't even need to manually clone this repository to build it on a new computer. Simply download the [`tools/build-windows.ps1`](tools/build-windows.ps1) script to any folder and run it inside **PowerShell**:

```powershell
powershell -ExecutionPolicy Bypass -File .\build-windows.ps1
```

*(If you already have a checkout clone on your drive, run `tools\build-windows.ps1` from your repo root instead — it automatically detects and uses your local workspace checkout).*

#### How it works:
1. **Environment Setup:** The script automatically scans your PC for Python, Git, CMake, Ninja, and the ARM toolchain. If any are missing, it installs them via `winget` (falling back to portable downloads if `winget` is unavailable).
2. **Auto-Cloning:** The script reaches out directly to this customized fork (`MukulCrazy/...`) and clones the source code tree alongside the pinned Pico SDK components into `%USERPROFILE%\.ds5-build`.
3. **Compilation:** It automatically initializes submodules, executes the CMake + Ninja compilation routines, and places your finished flashable **`ds5-bridge.uf2`** file right onto your Desktop!

### Manual/Other Platforms
To build from source manually on alternative environments:
1. Install the **Pico SDK 2.2.0** and verify its internal TinyUSB submodule is explicitly switched to tag **0.20.0**.
2. Initialize this repository's submodules: `git submodule update --init --recursive`
3. Configure and compile using the standard toolchain commands:
   ```bash
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DPICO_SDK_PATH=<path_to_sdk>
   cmake --build build --target ds5-bridge
   ```

---

## 📜 Credits & References


### Attributions
* **Core Bluetooth Architecture & Config Framework:** Developed using the baseline open-source stack created by **awalol**.
* **USB Descriptors Structural Mapping:** Core layout descriptors derived from configurations by **HiFiPhile**.
* **Xbox Mode Layouts, Vendor Class Endpoints, & Rumble Translation Engine:** Re-architected, coded, and officially modified by **MukulCrazy**.

### Foundational Inclusions
* [rafaelvaloto/Pico_W-Dualsense](https://github.com/rafaelvaloto/Pico_W-Dualsense) — Project concept inspiration.
* [egormanga/SAxense](https://github.com/egormanga/SAxense) — Bluetooth Haptics POC structural mapping.
* [Sony DualSense Wiki Documentation](https://controllers.fandom.com/wiki/Sony_DualSense) — DualSense data report packet structures.
* [Paliverse/DualSenseX](https://github.com/Paliverse/DualSenseX) — Speaker report packet layout handling pointers.