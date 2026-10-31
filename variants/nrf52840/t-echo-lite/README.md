# T-Echo Lite

基于 nRF52840 的 Meshtastic 键盘设备。

## 硬件规格

- **MCU**: nRF52840
- **键盘**: TCA8418 I2C 矩阵键盘 (16x8, 128 键)
- **背光**: AW21009QNR (12-bit 亮度控制)
- **震动**: AW86224 线性马达 (206.3Hz 正弦波)
- **GPS**: u-blox L76K
- **屏幕**: 192x176 E-Ink

## 文档

- [CHANGELOG.md](./CHANGELOG.md) — 版本历史和更新日志
- [HOWTO.md](./HOWTO.md) — 构建、烧录、配置指南

## 构建

```bash
pio run -e t-echo-lite
```

## 特性

- **4 模式通知系统** — ESC 三击切换：OFF / VIB ONLY / LIGHT ONLY / VIB+LIGHT
- **按键震动** — TAB 三击切换
- **背光亮度循环** — HOME 三击切换
- **背光自动关闭** — 30 秒无按键
- **消息通知** — 收到消息时震动 + 背光闪烁
