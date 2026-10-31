# T-Echo Lite

基于 nRF52840 的 Meshtastic 键盘设备。

nRF52840-based Meshtastic keyboard device.

## 硬件规格 / Hardware Specs

- **MCU**: nRF52840
- **键盘 / Keyboard**: TCA8418 I2C 矩阵键盘 / matrix keyboard (16x8, 128 键 / keys)
- **背光 / Backlight**: AW21009QNR (12-bit 亮度控制 / brightness control)
- **震动 / Vibration**: AW86224 线性马达 / linear motor (206.3Hz 正弦波 / sine wave)
- **GPS**: u-blox L76K
- **屏幕 / Display**: 192x176 E-Ink

## 文档 / Documentation

### 中文
- [CHANGELOG.md](./CHANGELOG.md) — 版本历史和更新日志
- [HOWTO.md](./HOWTO.md) — 构建、烧录、配置指南

### English
- [CHANGELOG.en.md](./CHANGELOG.en.md) — Version history and changelog
- [HOWTO.en.md](./HOWTO.en.md) — Build, flash, and configuration guide

## 构建

```bash
pio run -e t-echo-lite
```

## 特性 / Features

- **4 模式通知系统 / 4-Mode Notification System** — ESC 三击切换 / triple-tap to cycle: OFF / VIB ONLY / LIGHT ONLY / VIB+LIGHT
- **按键震动 / Key Vibration** — TAB 三击切换 / triple-tap to toggle
- **背光亮度循环 / Backlight Brightness Cycling** — HOME 三击切换 / triple-tap to cycle
- **背光自动关闭 / Backlight Auto-Off** — 30 秒无按键 / no keypress for 30 seconds
- **消息通知 / Message Notification** — 收到消息时震动 + 背光闪烁 / vibration + backlight flash on message receipt
