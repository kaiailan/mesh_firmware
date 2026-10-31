# T-Echo Lite Firmware HOW-TO

## Overview

T-Echo Lite is an nRF52840-based Meshtastic keyboard device with:
- TCA8418 I2C matrix keyboard (16x8, 128 keys)
- AW21009QNR backlight driver (12-bit brightness)
- AW86224 linear motor (vibration feedback)
- u-blox L76K GPS
- 192x176 E-Ink display

---

## Building the Firmware

### 1. Install Dependencies

```bash
# Install PlatformIO
pip install platformio

# Install nrfutil (for flashing)
pip install nrfutil
```

### 2. Build Command

```bash
cd /home/kai_root/meshtastic
/home/kai_root/.local/bin/pio run -e t-echo-lite
```

### 3. Firmware Output

After building, the firmware is located at:
```
.pio/build/t-echo-lite/firmware-t-echo-lite-2.8.2.0d3da2d.uf2
```

---

## Flashing the Firmware

### Method 1: USB Flash (Recommended)

1. Connect T-Echo Lite to computer
2. Copy firmware to USB drive:
   ```bash
   cp .pio/build/t-echo-lite/firmware-t-echo-lite-2.8.2.0d3da2d.uf2 /media/usb/
   ```
3. Put T-Echo Lite into DFU mode
4. Drag firmware file into DFU directory

### Method 2: SCP to Flash Box

```bash
# Install sshpass (one-time)
sudo apt-get install -y sshpass

# Copy firmware to flash box
sshpass -p '[.passwords]' scp -o StrictHostKeyChecking=no \
  .pio/build/t-echo-lite/firmware-t-echo-lite-2.8.2.0d3da2d.uf2 \
  kai_root@192.168.1.12:/home/kai_root/test-rom/firmware.uf2
```

---

## Key Mapping

### Keyboard Layout

T-Echo Lite uses a 5-row × 4-column matrix keyboard layout (20 physical keys):

```
┌─────────┬─────────┬─────────┬─────────┐
│   UP    │   ESC   │  HOME   │  MAIL   │  Row 0
├─────────┼─────────┼─────────┼─────────┤
│  CENTER │   '1'   │   '2'   │   '3'   │  Row 1
├─────────┼─────────┼─────────┼─────────┤
│   DOWN  │   '4'   │   '5'   │   '6'   │  Row 2
├─────────┼─────────┼─────────┼─────────┤
│   NO    │   '7'   │   '8'   │   '9'   │  Row 3
├─────────┼─────────┼─────────┼─────────┤
│   YES   │   '*'   │   '0'   │   '#'   │  Row 4
└─────────┴─────────┴─────────┴─────────┘
```

### Function Key Mapping

| Key | Position | Single | Double | Triple |
|-----|----------|--------|--------|--------|
| **UP** | (0,0) | UP | UP | Backlight Toggle |
| **ESC** | (0,1) | ESC | ESC | Notification Mode Cycle |
| **HOME** | (0,2) | LEFT | BSP | Backlight Toggle |
| **MAIL** | (0,3) | RIGHT | BSP | Key Vibration Toggle |
| **CENTER** | (1,0) | SELECT | TAB | Backlight Toggle |
| **DOWN** | (2,0) | DOWN | DOWN | Backlight Toggle |
| **NO** | (3,0) | ESC | ESC | Backlight Toggle |
| **YES** | (4,0) | SELECT | TAB | Backlight Toggle |

### Number/Symbol Key Mapping (Multi-Tap Cycle)

| Key | Position | Tap 1 | Tap 2 | Tap 3 | Tap 4 | Tap 5 |
|-----|----------|-------|-------|-------|-------|-------|
| **'1'** | (1,1) | . | , | 1 | | |
| **'2'** | (1,2) | a | b | c | 2 | @ |
| **'3'** | (1,3) | d | e | f | 3 | |
| **'4'** | (2,1) | g | h | i | 4 | |
| **'5'** | (2,2) | j | k | l | 5 | |
| **'6'** | (2,3) | m | n | o | 6 | |
| **'7'** | (3,1) | p | q | r | 7 | s |
| **'8'** | (3,2) | t | u | v | 8 | |
| **'9'** | (3,3) | w | x | y | 9 | z |
| **'*'** | (4,1) | * | BSP | ENTER | | |
| **'0'** | (4,2) | SPACE | 0 | | | |
| **'#'** | (4,3) | # | SHIFT Toggle | | | |

### Multi-Tap Rules

- **Threshold**: 400ms (most keys), `*` key 1200ms (TCA8418 latency)
- **Logic**: `tapIndex = (tap_count - 1) % TEchoLiteTapMod[key]`
- **Important**: `TEchoLiteTapMod[key]` must be >= max consecutive taps for that key

### Special Keys

- **SHIFT Key (#)**: Double-tap toggles SHIFT state (uppercase/special characters), single-tap outputs `#` character
- **Notification Mode Cycle (ESC Triple-Tap)**: 3 → 0 → 1 → 2 → 3 (VIB+LIGHT → OFF → VIB ONLY → LIGHT ONLY → VIB+LIGHT)
- **Backlight Toggle**: UP/HOME/CENTER/DOWN/NO/YES triple-tap toggles backlight
- **Key Vibration Toggle (MAIL Triple-Tap)**: Toggles key vibration feedback

---

## Notification Mode System

### Mode Definitions

| Mode | Name | Vibration | Flash |
|------|------|-----------|-------|
| **0** | OFF | ❌ | ❌ |
| **1** | VIB ONLY | ✅ | ❌ |
| **2** | LIGHT ONLY | ❌ | ✅ |
| **3** | VIB + LIGHT (Default) | ✅ | ✅ |

### How to Switch

**ESC Triple-Tap** cycles: 3 → 0 → 1 → 2 → 3

### Log Output

```
Notification mode: VIB + LIGHT -> OFF (mode=3 -> 0)
Notification mode: OFF -> VIB ONLY (mode=0 -> 1)
Notification mode: VIB ONLY -> LIGHT ONLY (mode=1 -> 2)
Notification mode: LIGHT ONLY -> VIB + LIGHT (mode=2 -> 3)
```

---

## Backlight System

### Brightness Cycling

**HOME Triple-Tap** cycles: OFF → 30% (38/127) → 65% (82/127) → 100% (127/127) → OFF

### Auto-Off

- **Trigger**: 30 seconds of no keypress
- **Resume**: Any keypress restores original brightness

### Backlight Flash (Message Notification)

- **Pattern**: 3 flashes
- **Timing**: ON(100ms) → OFF(80ms) × 3
- **Total Duration**: ~460ms
- **Restore**: Restores original backlight state (on/off) after flashing

---

## Vibration System

### Key Vibration

- **Trigger**: MAIL triple-tap toggle
- **Check**: `key_vibrate_enabled`
- **Purpose**: Key feedback

### Message Vibration

- **Trigger**: Message received + notification mode allows
- **Check**: `isNotificationVibrationEnabled()` (doesn't check `key_vibrate_enabled`)
- **Purpose**: Message reminder

### Double-Pulse Pattern

```
30ms vibration → 50ms interval → 30ms vibration
```

---

## Message Notification Integration

### Trigger Points

- **File**: `src/modules/ExternalNotificationModule.cpp`
- **Function**: `triggerVibraOutput()`
- **Condition**: Message received + External Notification Module enabled

### Execution Logic

```cpp
bool doVib = kb->isNotificationVibrationEnabled();
bool doLight = kb->isNotificationLightEnabled();

if (doVib && doLight) {
    kb->vibrateNotification(30);  // Doesn't check key_vibrate_enabled
    kb->flashBacklightNotification();
} else if (doVib) {
    kb->vibrateNotification(30);
    delay(50);
    kb->vibrateNotification(30);
} else if (doLight) {
    kb->flashBacklightNotification();
}
```

---

## Logging Conventions

### Kept Logs

- Chip initialization (AW21009, AW86224)
- Key vibration toggle (MAIL triple-tap)
- Notification mode switch (ESC triple-tap)

### Removed Logs

- Key detection (every keypress)
- Multi-tap progression (every consecutive tap)
- Backlight brightness setting (every flash)
- SHIFT toggle
- Forced screen refresh

---

## Common Issues

### 1. Triple-Tap Not Triggering

**Cause**: `TEchoLiteTapMod[key]` too small

**Fix**: Change to >= max consecutive taps

**Example**: ESC triple-tap requires `TEchoLiteTapMod[1] = 3`

### 2. Message Vibration Affected by Key Vibration Toggle

**Cause**: Used `vibrateKey()` instead of `vibrateNotification()`

**Fix**: Message vibration should use `vibrateNotification()`

### 3. Backlight Flash Changes Backlight State

**Cause**: Didn't restore original state after flashing

**Fix**: `flashBacklightNotification()` checks `backlight_on` at the end and restores

### 4. Compilation Error: `setAW21009Brightness` Not Defined

**Cause**: Function is lambda, scope issue

**Fix**: Change to regular member function

---

## Hardware Specifications

### AW86224 Linear Motor

- I2C Address: 0x58
- Frequency: 206.3 Hz (F0)
- Waveform: Sine wave, 12kHz sampling rate
- Storage: SRAM @ 0x3800
- Registers:
  - `0x08`=0 (RAM mode)
  - `0x0A`=1 (Wave ID 1)
  - `0x07`=15 (Gain 100%)
  - `0x09` bit0=1 (GO)
- Power: Always on

### AW21009QNR Backlight Driver

- I2C Address: 0x20
- GCR: 0x20
- Reset: 0x70
- Global Current: 0x58
- Channel SL: 0x46-0x4E
- Brightness 12bit: 0x21-0x2E
- UPDATE: 0x45
- GCR Value: 0xB1

### TCA8418 Matrix Keyboard

- I2C Address: 0x34
- Matrix: 16x8 (128 keys)
- Polling Interval: 50ms (TCA8418 specific)

---

## Version History

### 2026-10-30 (v1.0.0)
- Fixed ESC triple-tap not triggering (TEchoLiteTapMod[1] = 3)
- Separated key vibration and message vibration (vibrateNotification)
- Cleaned up debug logs (kept important logs)
- Final firmware MD5: `996244727a41b19186b87e079fc9ce98`

### 2026-10-29 (v0.9.0)
- AW86224 linear motor driver
- AW21009QNR backlight driver
- TCA8418 matrix keyboard

### 2026-10-27 (v0.8.0)
- Initial version
- Basic keyboard support
- Backlight control
- Multi-tap detection
