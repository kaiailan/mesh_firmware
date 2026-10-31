# T-Echo Lite Firmware Changelog

## v1.0.0 (2026-10-30)

### New Features
- **4-Mode Notification System** — ESC triple-tap to cycle: OFF / VIB ONLY / LIGHT ONLY / VIB+LIGHT
- **Key Vibration** — MAIL triple-tap to toggle key vibration
- **Backlight Brightness Cycling** — HOME triple-tap: OFF → 30% → 65% → 100% → OFF
- **Backlight Auto-Off** — Automatically turns off after 30 seconds of no keypress
- **Message Notification Integration** — Triggers vibration + backlight flash on message receipt

### Fixes
- **ESC Triple-Tap Not Triggering** — Fixed `TEchoLiteTapMod[1]` from 1 to 3
- **Vibration Coupling** — Separated key vibration and message vibration (`vibrateNotification()` doesn't check `key_vibrate_enabled`)
- **Backlight Flash State** — `flashBacklightNotification()` restores original backlight state after flashing

### Performance Optimizations
- **Keyboard Polling** — Reduced TCA8418 polling interval from 300ms to 50ms to reduce key latency
- **Debug Log Cleanup** — Removed frequent LOG_DEBUG and LOG_INFO, kept critical state logs

### Vibration Pattern
- **Double-Pulse Mode** — 30ms vibration + 50ms interval + 30ms vibration
- **Backlight Flash** — 3 flashes, each ON(100ms) → OFF(80ms), total duration ~460ms

### Notification Modes
- **Mode 0**: OFF (no vibration, no flash)
- **Mode 1**: VIB ONLY (double-pulse vibration)
- **Mode 2**: LIGHT ONLY (3 backlight flashes)
- **Mode 3**: VIB + LIGHT (double-pulse vibration + 3 backlight flashes) **Default**

### Key Mapping
- **MAIL (Key 3)**: Single=RIGHT, Double=BSP, Triple=Key Vibration Toggle
- **ESC (Key 1)**: Single=ESC, Double=ESC, Triple=Notification Mode Cycle
- **SHIFT (Key 19)**: Single=#, Double=SHIFT Toggle
- **Other Function Keys**: Triple=Backlight Toggle

---

## v0.9.0 (2026-10-29)

### New Features
- **AW86224 Linear Motor Driver** — I2C 0x58, sine wave 206.3Hz
- **AW21009QNR Backlight Driver** — I2C 0x20, 12-bit brightness control
- **TCA8418 Matrix Keyboard** — 16x8, 128 keys

### Fixes
- **Multi-Tap Count Bug** — Fixed consecutive tap counting logic
- **SHIFT Clear Bug** — Fixed SHIFT state clearing issue

### Hardware Adaptation
- **nRF52840** — T-Echo Lite hardware support
- **u-blox L76K GPS** — GPS module integration
- **192x176 E-Ink** — E-paper display support

---

## v0.8.0 (2026-10-27)

### Initial Version
- **Basic Keyboard Support** — TCA8418 matrix keyboard driver
- **Backlight Control** — AW21009QNR basic brightness control
- **Multi-Tap Detection** — Basic multi-tap counting and mapping

---

## Technical Details

### AW86224 Configuration
- I2C Address: 0x58
- Frequency: 206.3 Hz (F0)
- Waveform: Sine wave, 12kHz sampling rate
- Storage: SRAM @ 0x3800
- Registers:
  - `0x08`=0 (RAM mode)
  - `0x0A`=1 (Wave ID 1)
  - `0x07`=15 (Gain 100%)
  - `0x09` bit0=1 (GO)

### AW21009 Registers
- GCR: 0x20
- Reset: 0x70
- Global Current: 0x58
- Channel SL: 0x46-0x4E
- Brightness 12bit: 0x21-0x2E
- UPDATE: 0x45
- GCR Value: 0xB1

### Brightness Control
- Range: 0-4095 (12-bit)
- User Scale: 0-127 (software layer)
- Conversion: `level = (brightness * 4095) / 127`

---

## Build & Deploy

### Build Command
```bash
cd /home/kai_root/meshtastic
/home/kai_root/.local/bin/pio run -e t-echo-lite
```

### Firmware Output
- Path: `.pio/build/t-echo-lite/firmware-t-echo-lite-2.8.2.0d3da2d.uf2`
- Copy to: `/home/kai_root/test-rom/firmware.uf2`
- SCP to: `kai_root@192.168.1.12:/home/kai_root/test-rom/firmware.uf2`

### Flash
```bash
cp /home/kai_root/test-rom/firmware.uf2 /media/usb/
```

---

## File List

### Core Files
- `src/input/TEchoLiteKeyboard.h` — Header file
- `src/input/TEchoLiteKeyboard.cpp` — Implementation (32KB)
- `src/modules/ExternalNotificationModule.cpp` — Message notification

### Key Functions
- `TEchoLiteKeyboard::init()` — Initialization
- `TEchoLiteKeyboard::pressed()` — Key press handling
- `TEchoLiteKeyboard::trigger()` — Multi-tap commit
- `TEchoLiteKeyboard::flashBacklightNotification()` — Backlight flash
- `TEchoLiteKeyboard::vibrateNotification()` — Message vibration
- `TEchoLiteKeyboard::vibrateKey()` — Key vibration
- `ExternalNotificationModule::triggerVibraOutput()` — Message vibration entry point
