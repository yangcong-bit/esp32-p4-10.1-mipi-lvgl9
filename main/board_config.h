/*
 * WTDKP4C5-S1 开发板硬件配置
 * 基于 ESP32-P4 + 10.1寸 TFT IPS 屏幕 (JD9366 LCD + JD9366 Touch)
 *
 * NOTE: 引脚编号请根据实际硬件原理图确认！
 *       以下为 ESP32-P4 常见开发板默认值，可能需要根据实际板子调整。
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================
 *  LCD 显示 (MIPI DSI + JD9366)
 * ====================================================================== */

/* 设为 0 可在没有屏幕时跳过 LCD 初始化，避免看门狗超时 */
#define BOARD_LCD_ENABLED            1

/* 屏幕分辨率 */
#define BOARD_LCD_H_RES            800
#define BOARD_LCD_V_RES            1280

/* MIPI DSI 参数 */
#define BOARD_MIPI_DSI_LANE_NUM    2
#define BOARD_MIPI_DSI_LANE_RATE   1000    /* Mbps */

/* DPI 时钟和视频时序 (JD9366 800x1280 @60Hz) */
#define BOARD_LCD_DPI_CLK_MHZ      55
#define BOARD_LCD_HSYNC             8
#define BOARD_LCD_HBP              28
#define BOARD_LCD_HFP              40
#define BOARD_LCD_VSYNC             8
#define BOARD_LCD_VBP              20
#define BOARD_LCD_VFP             140

/* LCD 背光 — 无独立引脚，通过 I2C 软件控制 */
#define BOARD_PIN_NUM_LCD_BK_LIGHT  -1
#define BOARD_LCD_BK_LIGHT_ON_LEVEL   1
#define BOARD_LCD_BK_LIGHT_OFF_LEVEL  0

/* LCD 复位 — 无独立硬件复位，上电自动 RC 复位 */
#define BOARD_PIN_NUM_LCD_RST      -1

/* 像素格式 */
#define BOARD_LCD_COLOR_FORMAT     LCD_COLOR_FMT_RGB888
#define BOARD_LCD_BITS_PER_PIXEL   24

/* ======================================================================
 *  触摸屏 (I2C + JD9366 Touch)
 * ====================================================================== */

/* I2C 总线 — 屏幕插座 Pin11/12 */
#define BOARD_PIN_NUM_I2C_SDA       7      /* GPIO7 — I2C SDA (SDA0_2) */
#define BOARD_PIN_NUM_I2C_SCL       8      /* GPIO8 — I2C SCL (SCL0_2) */
#define BOARD_I2C_CLK_HZ           400000  /* 400 kHz */

/* 触摸控制引脚 — 无独立引脚，I2C 软控制 */
#define BOARD_PIN_NUM_TOUCH_RST     -1
#define BOARD_PIN_NUM_TOUCH_INT     -1

/* ======================================================================
 *  LVGL 配置
 * ====================================================================== */

#define BOARD_LVGL_TICK_PERIOD_MS   2
#define BOARD_LVGL_DRAW_BUF_LINES   20
#define BOARD_LVGL_TASK_STACK_SIZE  (8 * 1024)
#define BOARD_LVGL_TASK_PRIORITY    2

#ifdef __cplusplus
}
#endif
