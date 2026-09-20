# esp32-p4-10.1-mipi-lvgl9

基于 **ESP32-P4** 与 **10.1 寸 IPS 屏 (JD9366, MIPI DSI)** 的 LVGL 9 显示与触摸示例工程，适配 **WTDKP4C5-S1** 开发板。

## ✨ 特性

- 🖥️ **10.1 寸 800×1280 IPS 屏**，通过 **MIPI DSI** 2-lane 接口驱动
- 🎨 显示控制器 **JD9366**（含厂商初始化序列）
- 👆 触摸：JD9366 触摸方案，通过 **I2C**（BackDoor 协议）读取坐标
- 📊 **LVGL 9.2** 图形库，集成乐鑫 **esp_lvgl_port** 适配层
- ⚡ **PPA 硬件加速** + PSRAM (XIP) 显示缓冲
- 🧩 支持 LVGL 官方 **widgets / benchmark** 演示
- 🗂️ 自定义 **OTA** 分区表（16MB Flash）

## 🛠️ 硬件

| 项目 | 参数 |
|------|------|
| 主控 | ESP32-P4 (Eco2 预量产版, rev v1.3) |
| Flash | 16MB (N16R32) |
| PSRAM | 32MB Octal (HEX 模式, 200MHz, XIP) |
| 屏幕 | 10.1 寸 IPS, 800×1280, MIPI DSI 2-lane |
| LCD 控制器 | JD9366 |
| 触摸 | JD9366 触摸方案 (I2C) |

### 屏幕接口 (U4, 15-Pin MIPI DSI 插座)

| 引脚 | 功能 |
|------|------|
| Pin 2/3 | MIPI DSI 数据 1- / 1+ |
| Pin 5/6 | MIPI DSI 时钟 - / + |
| Pin 8/9 | MIPI DSI 数据 0- / 0+ |
| Pin 11 | I2C SCL (`GPIO8`) |
| Pin 12 | I2C SDA (`GPIO7`) |
| Pin 14/15 | 3.3V 供电 |

> 该 15-Pin 接口无独立复位/背光/中断引脚：复位由板上 RC 电路完成，其余通过 I2C 软件控制。

## 📁 目录结构

```
esp32-p4-10.1-mipi-lvgl9/
├── CMakeLists.txt              # 项目构建入口 (project esp32-p4-10.1-mipi-lvgl9)
├── partitions.csv              # 自定义 OTA 分区表 (16MB)
├── sdkconfig.defaults          # 默认 SDK 配置
├── main/
│   ├── main.c                  # 主程序 (LCD/Touch/LVGL 初始化 + Demo)
│   ├── board_config.h          # 板级硬件配置 (引脚/分辨率/时序)
│   ├── CMakeLists.txt
│   └── idf_component.yml       # 组件依赖 (lvgl/esp_lvgl_port/esp_lcd_touch)
└── components/
    ├── esp_lcd_jd9366/         # JD9366 LCD 驱动 (MIPI DSI)
    └── esp_lcd_touch_jd9366/   # JD9366 触摸驱动 (I2C BackDoor)
```

## 🚀 快速开始

### 环境要求

- **ESP-IDF** ≥ 5.5（本项目基于 v5.5.4 开发）
- 目标芯片：`esp32p4`

### 构建与烧录

```bash
# 1. 设置 IDF 环境
export IDF_PATH=...   # Windows: idf.py 由 ESP-IDF 扩展提供

# 2. 完整构建（首次或更改项目名后）
idf.py fullclean build

# 3. 烧录并监控 (COM16 为示例端口，按实际修改)
idf.py -p COM16 flash monitor
```

### 演示切换

在 `menuconfig` 中配置 LVGL Demo（`Component config → LVGL configuration → Demos`）：

- 启用 `LV_USE_DEMO_BENCHMARK` → 运行基准测试
- 启用 `LV_USE_DEMO_WIDGETS`（同时关闭 benchmark）→ 运行控件演示
- 两者都关闭 → 运行简单的文字 Demo（`main.c` 内置）

## 🔧 关键配置

### `main/board_config.h`

| 配置项 | 值 | 说明 |
|--------|-----|------|
| `BOARD_LCD_ENABLED` | 1 | 设为 0 可在无屏时跳过 LCD 初始化 |
| `BOARD_LCD_H_RES/V_RES` | 800×1280 | 屏幕分辨率 |
| `BOARD_MIPI_DSI_LANE_NUM` | 2 | DSI 数据通道数 |
| `BOARD_LCD_DPI_CLK_MHZ` | 55 | DPI 像素时钟（约 43Hz） |
| `BOARD_PIN_NUM_I2C_SDA/SCL` | 7 / 8 | 触摸 I2C 引脚 |
| `BOARD_PIN_NUM_LCD_RST/BK_LIGHT` | -1 | 无独立硬件引脚 |

### `sdkconfig.defaults`

- 芯片版本：`SELECTS_REV_LESS_V3`（Eco2 预量产版兼容）
- Flash：16MB QIO 80MHz
- PSRAM：Octal 200MHz + XIP，L2 Cache 256KB
- LVGL：24bit 色深、CLIB 内存、PPA 加速、Perf Monitor
- Watchdog：15s 超时，关闭 IDLE 任务检测

## 📦 依赖组件

| 组件 | 版本 | 来源 |
|------|------|------|
| `espressif/esp_lvgl_port` | ^2.8 | 组件注册表 |
| `lvgl/lvgl` | ~9.2 | 组件注册表 |
| `espressif/esp_lcd_touch` | >=1.1.2 | 组件注册表 |

> 本地组件（`components/`）说明：
> - `esp_lcd_jd9366`：**基于乐鑫 ESP-IDF 参考驱动修改**，按本屏规格适配初始化序列与 DPI 时序（已禁用 DSI Command ACK 以规避初始化死锁）
> - `esp_lcd_touch_jd9366`：基于乐鑫 `esp_lcd_touch` 框架，**按屏厂参考代码实现 JD9366 I2C BackDoor 协议**
>
> 两个组件均为 **Apache-2.0 衍生作品**，保留了上游版权声明，修改记录见 [`NOTICE`](NOTICE)。

## ⚠️ 已知注意事项

1. **MIPI DSI Command ACK 已禁用**：JD9366 面板不响应读命令会导致 HAL 层死锁，驱动中通过 LL 层关闭 Command ACK 规避（位于 `esp_lcd_new_panel_jd9366()`）。
2. **跳过 LCD ID 读取**：部分面板不响应 `0x04 RDID`，初始化序列不依赖 ID。
3. **刷新率约 43Hz**（RGB888 + 800×1280 @ 55MHz DPI 时钟）。如需 60Hz，将 `BOARD_LCD_DPI_CLK_MHZ` 提升至 ~70MHz，并确保 PSRAM 带宽充足。
4. **`managed_components` 为拉取的第三方源码**：`esp_lvgl_port` 曾针对 LVGL 9.2 打补丁（移除 `LV_COLOR_FORMAT_RGB565_SWAPPED`）。重新 `fullclean` 拉取后如有编译错误需重新打补丁。
5. **无屏调试**：将 `BOARD_LCD_ENABLED` 置 0 可跳过 LCD/Touch 初始化，避免看门狗超时。

## 📄 License

本项目采用 **Apache License 2.0**，完整文本见 [`LICENSE`](LICENSE)。

其中 `components/esp_lcd_jd9366` 与 `components/esp_lcd_touch_jd9366` 为
**乐鑫（Espressif Systems）驱动代码的衍生作品**，遵循上游 Apache-2.0 许可；
本项目保留其原始版权声明，并对所做的修改作出声明。详见 [`NOTICE`](NOTICE)。

```
Copyright 2026 yangcong-bit

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0
```
