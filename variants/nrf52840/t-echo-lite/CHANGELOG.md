# T-Echo Lite Firmware Changelog

## v1.0.0 (2026-10-30)

### 新增功能
- **4 模式通知系统** — ESC 三击切换：OFF / VIB ONLY / LIGHT ONLY / VIB+LIGHT
- **按键震动** — TAB 三击切换按键震动开关
- **背光亮度循环** — HOME 三击切换：OFF → 30% → 65% → 100% → OFF
- **背光自动关闭** — 30 秒无按键自动关闭背光
- **消息通知集成** — 收到消息时触发震动 + 背光闪烁

### 修复
- **ESC 三击不触发** — 修复 `TEchoLiteTapMod[1]` 从 1 改为 3
- **震动耦合** — 分离按键震动和消息震动（`vibrateNotification()` 不检查 `key_vibrate_enabled`）
- **背光闪烁状态** — `flashBacklightNotification()` 闪烁后恢复原背光状态

### 性能优化
- **键盘轮询** — TCA8418 轮询间隔从 300ms 降到 50ms，减少按键延迟
- **调试日志清理** — 移除频繁的 LOG_DEBUG 和 LOG_INFO，保留关键状态日志

### 震动模式
- **双脉冲模式** — 30ms 震动 + 50ms 间隔 + 30ms 震动
- **背光闪烁** — 3 次闪烁，每次 ON(100ms) → OFF(80ms)，总时长 ~460ms

### 通知模式
- **Mode 0**: OFF（无震动，无闪光）
- **Mode 1**: VIB ONLY（双脉冲震动）
- **Mode 2**: LIGHT ONLY（3 次背光闪烁）
- **Mode 3**: VIB + LIGHT（双脉冲震动 + 3 次背光闪烁）**默认**

### 按键映射
- **TAB (Key 3)**: 单击=RIGHT, 双击=BSP, 三击=按键震动开关
- **ESC (Key 1)**: 单击=ESC, 双击=ESC, 三击=通知模式循环
- **SHIFT (Key 19)**: 单击=#, 双击=SHIFT 切换
- **其他功能键**: 三击=背光开关

---

## v0.9.0 (2026-10-29)

### 新增功能
- **AW86224 线性马达驱动** — I2C 0x58, 正弦波 206.3Hz
- **AW21009QNR 背光驱动** — I2C 0x20, 12-bit 亮度控制
- **TCA8418 矩阵键盘** — 16x8, 128 键

### 修复
- **多击计数 bug** — 修复连击计数逻辑
- **SHIFT 清除 bug** — 修复 SHIFT 状态清除问题

### 硬件适配
- **nRF52840** — 支持 T-Echo Lite 硬件
- **u-blox L76K GPS** — GPS 模块集成
- **192x176 E-Ink** — 电子墨水屏支持

---

## v0.8.0 (2026-10-27)

### 初始版本
- **基础键盘支持** — TCA8418 矩阵键盘驱动
- **背光控制** — AW21009QNR 基础亮度控制
- **多击检测** — 基础的多击计数和映射

---

## 技术细节

### AW86224 配置
- I2C 地址: 0x58
- 频率: 206.3 Hz (F0)
- 波形: 正弦波, 12kHz 采样率
- 存储: SRAM @ 0x3800
- 寄存器:
  - `0x08`=0 (RAM 模式)
  - `0x0A`=1 (Wave ID 1)
  - `0x07`=15 (Gain 100%)
  - `0x09` bit0=1 (GO)

### AW21009 寄存器
- GCR: 0x20
- 复位: 0x70
- 全局电流: 0x58
- 通道 SL: 0x46-0x4E
- 亮度 12bit: 0x21-0x2E
- UPDATE: 0x45
- GCR 值: 0xB1

### 亮度控制
- 范围: 0-4095 (12-bit)
- 用户刻度: 0-127 (软件层)
- 换算: `level = (brightness * 4095) / 127`

---

## 构建与部署

### 构建命令
```bash
cd /home/kai_root/meshtastic
/home/kai_root/.local/bin/pio run -e t-echo-lite
```

### 固件输出
- 路径: `.pio/build/t-echo-lite/firmware-t-echo-lite-2.8.2.0d3da2d.uf2`
- 复制到: `/home/kai_root/test-rom/firmware.uf2`
- SCP 到: `kai_root@192.168.1.12:/home/kai_root/test-rom/firmware.uf2`

### 烧录
```bash
cp /home/kai_root/test-rom/firmware.uf2 /media/usb/
```

---

## 文件清单

### 核心文件
- `src/input/TEchoLiteKeyboard.h` — 头文件
- `src/input/TEchoLiteKeyboard.cpp` — 实现 (32KB)
- `src/modules/ExternalNotificationModule.cpp` — 消息通知

### 关键函数
- `TEchoLiteKeyboard::init()` — 初始化
- `TEchoLiteKeyboard::pressed()` — 按键处理
- `TEchoLiteKeyboard::trigger()` — 多击提交
- `TEchoLiteKeyboard::flashBacklightNotification()` — 背光闪烁
- `TEchoLiteKeyboard::vibrateNotification()` — 消息震动
- `TEchoLiteKeyboard::vibrateKey()` — 按键震动
- `ExternalNotificationModule::triggerVibraOutput()` — 消息震动入口
