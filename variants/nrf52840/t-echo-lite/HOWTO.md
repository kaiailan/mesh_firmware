# T-Echo Lite Firmware HOW-TO

## 概述

T-Echo Lite 是一款基于 nRF52840 的 Meshtastic 键盘设备，集成了：
- TCA8418 I2C 矩阵键盘 (16x8, 128 键)
- AW21009QNR 背光驱动 (12-bit 亮度)
- AW86224 线性马达 (震动反馈)
- u-blox L76K GPS
- 192x176 E-Ink 屏幕

---

## 构建固件

### 1. 安装依赖

```bash
# 安装 PlatformIO
pip install platformio

# 安装 nrfutil (用于烧录)
pip install nrfutil
```

### 2. 构建命令

```bash
cd /home/kai_root/meshtastic
/home/kai_root/.local/bin/pio run -e t-echo-lite
```

### 3. 固件输出

构建完成后，固件位于：
```
.pio/build/t-echo-lite/firmware-t-echo-lite-2.8.2.0d3da2d.uf2
```

---

## 烧录固件

### 方法 1: USB 烧录 (推荐)

1. 将 T-Echo Lite 连接到电脑
2. 复制固件到 USB 驱动器：
   ```bash
   cp .pio/build/t-echo-lite/firmware-t-echo-lite-2.8.2.0d3da2d.uf2 /media/usb/
   ```
3. 将 T-Echo Lite 进入 DFU 模式
4. 将固件文件拖入 DFU 目录

### 方法 2: SCP 到 Flash Box

```bash
# 安装 sshpass (一次性)
sudo apt-get install -y sshpass

# 复制固件到 flash box
sshpass -p '[.passwords]' scp -o StrictHostKeyChecking=no \
  .pio/build/t-echo-lite/firmware-t-echo-lite-2.8.2.0d3da2d.uf2 \
  kai_root@192.168.1.12:/home/kai_root/test-rom/firmware.uf2
```

---

## 按键映射

### 键盘布局

T-Echo Lite 使用 5 行 × 4 列的矩阵键盘布局（20 个物理按键）：

```
┌─────────┬─────────┬─────────┬─────────┐
│   UP    │   ESC   │  HOME   │   TAB   │  Row 0
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

### 功能键映射表

| 按键 | 位置 | 单击 | 双击 | 三击 |
|------|------|------|------|------|
| **UP** | (0,0) | UP | UP | 背光开关 |
| **ESC** | (0,1) | ESC | ESC | 通知模式循环 |
| **HOME** | (0,2) | LEFT | BSP | 背光开关 |
| **TAB** | (0,3) | RIGHT | BSP | 按键震动开关 |
| **CENTER** | (1,0) | SELECT | TAB | 背光开关 |
| **DOWN** | (2,0) | DOWN | DOWN | 背光开关 |
| **NO** | (3,0) | ESC | ESC | 背光开关 |
| **YES** | (4,0) | SELECT | TAB | 背光开关 |

### 数字/符号键映射表（多击循环）

| 按键 | 位置 | 第 1 击 | 第 2 击 | 第 3 击 | 第 4 击 | 第 5 击 |
|------|------|---------|---------|---------|---------|---------|
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
| **'#'** | (4,3) | # | SHIFT 切换 | | | |

### 多击规则

- **阈值**: 400ms（大多数键），`*` 键 1200ms（TCA8418 延迟）
- **逻辑**: `tapIndex = (tap_count - 1) % TEchoLiteTapMod[key]`
- **重要**: `TEchoLiteTapMod[key]` 必须 >= 该键的最大连击次数

### 特殊键说明

- **SHIFT 键 (#)**: 双击切换 SHIFT 状态（大写/特殊字符），单击输出 `#` 字符
- **通知模式循环 (ESC 三击)**: 3 → 0 → 1 → 2 → 3（VIB+LIGHT → OFF → VIB ONLY → LIGHT ONLY → VIB+LIGHT）
- **背光开关**: UP/HOME/CENTER/DOWN/NO/YES 三击切换背光
- **按键震动开关 (TAB 三击)**: 切换按键震动反馈

---

## 通知模式系统

### 模式定义

| 模式 | 名称 | 震动 | 闪光 |
|------|------|------|------|
| **0** | OFF | ❌ | ❌ |
| **1** | VIB ONLY | ✅ | ❌ |
| **2** | LIGHT ONLY | ❌ | ✅ |
| **3** | VIB + LIGHT (默认) | ✅ | ✅ |

### 切换方式

**ESC 三击** 循环切换：3 → 0 → 1 → 2 → 3

### 日志输出

```
Notification mode: VIB + LIGHT -> OFF (mode=3 -> 0)
Notification mode: OFF -> VIB ONLY (mode=0 -> 1)
Notification mode: VIB ONLY -> LIGHT ONLY (mode=1 -> 2)
Notification mode: LIGHT ONLY -> VIB + LIGHT (mode=2 -> 3)
```

---

## 背光系统

### 亮度循环

**HOME 三击** 切换：OFF → 30% (38/127) → 65% (82/127) → 100% (127/127) → OFF

### 自动关闭

- **触发**: 30 秒无按键
- **恢复**: 任意按键恢复原亮度

### 背光闪烁 (消息通知)

- **模式**: 3 次闪烁
- **时序**: ON(100ms) → OFF(80ms) × 3
- **总时长**: ~460ms
- **恢复**: 闪烁后恢复原背光状态 (开/关)

---

## 震动系统

### 按键震动

- **触发**: TAB 三击切换
- **检查**: `key_vibrate_enabled`
- **用途**: 按键反馈

### 消息震动

- **触发**: 收到消息 + 通知模式允许
- **检查**: `isNotificationVibrationEnabled()` (不检查 `key_vibrate_enabled`)
- **用途**: 消息提醒

### 双脉冲模式

```
30ms 震动 → 50ms 间隔 → 30ms 震动
```

---

## 消息通知集成

### 触发点

- **文件**: `src/modules/ExternalNotificationModule.cpp`
- **函数**: `triggerVibraOutput()`
- **条件**: 收到消息 + External Notification Module 启用

### 执行逻辑

```cpp
bool doVib = kb->isNotificationVibrationEnabled();
bool doLight = kb->isNotificationLightEnabled();

if (doVib && doLight) {
    kb->vibrateNotification(30);  // 不检查 key_vibrate_enabled
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

## 日志规范

### 保留的日志

- 芯片初始化 (AW21009, AW86224)
- 按键震动开关 (TAB 三击)
- 通知模式切换 (ESC 三击)

### 关闭的日志

- 按键检测 (每次按键)
- 多击推进 (每次连击)
- 背光亮度设置 (每次闪烁)
- SHIFT 切换
- 强制刷新屏幕

---

## 常见问题

### 1. 三击不触发

**原因**: `TEchoLiteTapMod[key]` 太小

**修复**: 改成 >= 最大连击次数

**示例**: ESC 三击需要 `TEchoLiteTapMod[1] = 3`

### 2. 消息震动被按键震动开关影响

**原因**: 用了 `vibrateKey()` 而不是 `vibrateNotification()`

**修复**: 消息震动改用 `vibrateNotification()`

### 3. 背光闪烁改变背光状态

**原因**: 闪烁后没恢复原状态

**修复**: `flashBacklightNotification()` 末尾检查 `backlight_on` 并恢复

### 4. 编译错误: `setAW21009Brightness` 未定义

**原因**: 函数是 lambda, 作用域问题

**修复**: 改成普通成员函数

---

## 硬件规格

### AW86224 线性马达

- I2C 地址: 0x58
- 频率: 206.3 Hz (F0)
- 波形: 正弦波, 12kHz 采样率
- 存储: SRAM @ 0x3800
- 寄存器:
  - `0x08`=0 (RAM 模式)
  - `0x0A`=1 (Wave ID 1)
  - `0x07`=15 (Gain 100%)
  - `0x09` bit0=1 (GO)
- 电源: 常开

### AW21009QNR 背光驱动

- I2C 地址: 0x20
- GCR: 0x20
- 复位: 0x70
- 全局电流: 0x58
- 通道 SL: 0x46-0x4E
- 亮度 12bit: 0x21-0x2E
- UPDATE: 0x45
- GCR 值: 0xB1

### TCA8418 矩阵键盘

- I2C 地址: 0x34
- 矩阵: 16x8 (128 键)
- 轮询间隔: 50ms (TCA8418 专用)

---

## 版本历史

### 2026-10-30 (v1.0.0)
- 修复 ESC 三击不触发 (TEchoLiteTapMod[1] = 3)
- 分离按键震动和消息震动 (vibrateNotification)
- 清理调试日志 (保留重要日志)
- 最终固件 MD5: `996244727a41b19186b87e079fc9ce98`

### 2026-10-29 (v0.9.0)
- AW86224 线性马达驱动
- AW21009QNR 背光驱动
- TCA8418 矩阵键盘

### 2026-10-27 (v0.8.0)
- 初始版本
- 基础键盘支持
- 背光控制
- 多击检测
