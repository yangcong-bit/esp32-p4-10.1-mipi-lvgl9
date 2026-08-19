/*
 * WTDKP4C5-S1 开发板 — LCD 显示 + 触摸屏初始化
 * LCD  : JD9366 (MIPI DSI, 800×1280, IPS)
 * Touch: JD9366 (I2C, BackDoor 协议)
 *
 * 使用 esp_lvgl_port 适配库简化 LVGL 集成
 */

#include <stdio.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_ldo_regulator.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

/* LCD / Touch 驱动 */
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_jd9366.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_jd9366.h"

/* Task watchdog */
#include "esp_task_wdt.h"

/* LVGL + ESP LVGL Port */
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "demos/lv_demos.h"

#include "board_config.h"

static const char *TAG = "main";

/* I2C 总线句柄 (LCD 和触摸共用) */
static i2c_master_bus_handle_t s_i2c_bus = NULL;

/* ===================== 硬件初始化 ===================== */

static void bsp_enable_dsi_phy_power(void)
{
    ESP_LOGI(TAG, "Enable MIPI DSI PHY power (LDO3)");
    esp_ldo_channel_config_t ldo_config = {
        .chan_id    = 3,
        .voltage_mv = 2500,
    };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_config, NULL));
}

static void bsp_init_lcd_backlight(void)
{
#if BOARD_PIN_NUM_LCD_BK_LIGHT >= 0
    gpio_config_t bk_gpio_config = {
        .mode         = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << BOARD_PIN_NUM_LCD_BK_LIGHT,
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(BOARD_PIN_NUM_LCD_BK_LIGHT, BOARD_LCD_BK_LIGHT_OFF_LEVEL);
#endif
}

static void bsp_set_lcd_backlight(int level)
{
#if BOARD_PIN_NUM_LCD_BK_LIGHT >= 0
    gpio_set_level(BOARD_PIN_NUM_LCD_BK_LIGHT, level);
#endif
}

static void bsp_init_i2c(void)
{
    ESP_LOGI(TAG, "Init I2C master (SDA=%d, SCL=%d, %dkHz)",
             BOARD_PIN_NUM_I2C_SDA, BOARD_PIN_NUM_I2C_SCL, BOARD_I2C_CLK_HZ / 1000);

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port   = I2C_NUM_0,
        .sda_io_num = BOARD_PIN_NUM_I2C_SDA,
        .scl_io_num = BOARD_PIN_NUM_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &s_i2c_bus));
}

/* ===================== LCD 初始化 ===================== */

static void bsp_init_lcd(esp_lcd_panel_handle_t *out_panel)
{
    ESP_LOGI(TAG, "Init MIPI DSI LCD (JD9366, %dx%d)", BOARD_LCD_H_RES, BOARD_LCD_V_RES);

    /* LCD 初始化 */

    bsp_enable_dsi_phy_power();
    bsp_init_lcd_backlight();

    /* 创建 MIPI DSI 总线 */
    esp_lcd_dsi_bus_handle_t dsi_bus;
    esp_lcd_dsi_bus_config_t bus_cfg = {
        .bus_id             = 0,
        .num_data_lanes     = BOARD_MIPI_DSI_LANE_NUM,
        .lane_bit_rate_mbps = BOARD_MIPI_DSI_LANE_RATE,
    };
    ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus_cfg, &dsi_bus));

    /* 创建 DBI 控制接口 */
    esp_lcd_panel_io_handle_t dbi_io;
    esp_lcd_dbi_io_config_t dbi_cfg = {
        .virtual_channel = 0,
        .lcd_cmd_bits    = 8,
        .lcd_param_bits  = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(dsi_bus, &dbi_cfg, &dbi_io));

    /* DPI 数据面板配置 (JD9366 800x1280 @60Hz) */
    static esp_lcd_dpi_panel_config_t dpi_cfg = {
        .virtual_channel  = 0,
        .dpi_clk_src      = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = BOARD_LCD_DPI_CLK_MHZ,
        .pixel_format     = BOARD_LCD_COLOR_FORMAT,
        .num_fbs          = 1,
        .video_timing     = {
            .h_size              = BOARD_LCD_H_RES,
            .v_size              = BOARD_LCD_V_RES,
            .hsync_pulse_width   = BOARD_LCD_HSYNC,
            .hsync_back_porch    = BOARD_LCD_HBP,
            .hsync_front_porch   = BOARD_LCD_HFP,
            .vsync_pulse_width   = BOARD_LCD_VSYNC,
            .vsync_back_porch    = BOARD_LCD_VBP,
            .vsync_front_porch   = BOARD_LCD_VFP,
        },
        .flags.use_dma2d  = true,
    };

    /* 创建 JD9366 LCD 面板 */
    esp_lcd_panel_handle_t panel = NULL;
    static jd9366_vendor_config_t vendor_cfg;
    vendor_cfg.mipi_config.dsi_bus = dsi_bus;
    vendor_cfg.mipi_config.dpi_config = &dpi_cfg;

    static esp_lcd_panel_dev_config_t dev_cfg;
    dev_cfg.reset_gpio_num = BOARD_PIN_NUM_LCD_RST;
    dev_cfg.rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB;
    dev_cfg.bits_per_pixel = BOARD_LCD_BITS_PER_PIXEL;
    dev_cfg.vendor_config  = &vendor_cfg;

    ESP_ERROR_CHECK(esp_lcd_new_panel_jd9366(dbi_io, &dev_cfg, &panel));

    /* 复位 + 初始化面板 + 开启显示 */
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

    /* 开启背光 */
    bsp_set_lcd_backlight(BOARD_LCD_BK_LIGHT_ON_LEVEL);
    ESP_LOGI(TAG, "LCD initialized, display & backlight ON");

    *out_panel = panel;
}

/* ===================== Touch 初始化 ===================== */

static esp_lcd_touch_handle_t bsp_init_touch(void)
{
    ESP_LOGI(TAG, "Init JD9366 Touch (I2C addr=0x%02X)", ESP_LCD_TOUCH_IO_I2C_JD9366_ADDRESS);

    /* 创建触摸 I2C IO */
    esp_lcd_panel_io_handle_t tp_io;
    esp_lcd_panel_io_i2c_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_I2C_JD9366_CONFIG();
    tp_io_cfg.scl_speed_hz = BOARD_I2C_CLK_HZ;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(s_i2c_bus, &tp_io_cfg, &tp_io));

    /* 触摸配置 */
    esp_lcd_touch_config_t tp_cfg = {
        .x_max   = BOARD_LCD_H_RES,
        .y_max   = BOARD_LCD_V_RES,
        .rst_gpio_num = BOARD_PIN_NUM_TOUCH_RST,
        .int_gpio_num = BOARD_PIN_NUM_TOUCH_INT,
        .levels = {
            .reset     = 0,
            .interrupt = 0,
        },
    };

    /* 创建 JD9366 触摸实例 */
    esp_lcd_touch_handle_t touch = NULL;
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_jd9366(tp_io, &tp_cfg, &touch));
    ESP_LOGI(TAG, "JD9366 Touch initialized");

    return touch;
}

static void wdt_feed_timer_cb(lv_timer_t * timer)
{
    /* 周期性主动阻塞 1 个 FreeRTOS Tick，让出 CPU 给 IDLE 任务以喂狗 */
    vTaskDelay(1);
}

/* ===================== app_main ===================== */

void app_main(void)
{
    ESP_LOGI(TAG, "=== WTDKP4C5-S1 Board Init ===");

    /* 1. 初始化 I2C 总线 */
    bsp_init_i2c();

    /* 2. 初始化 LCD (MIPI DSI + JD9366) */
    esp_lcd_panel_handle_t lcd_panel = NULL;
#if BOARD_LCD_ENABLED
    bsp_init_lcd(&lcd_panel);
#else
    ESP_LOGW(TAG, "LCD disabled (BOARD_LCD_ENABLED=0), skip LCD init");
#endif

    /* 3. 初始化触摸 (I2C + JD9366 Touch) */
    esp_lcd_touch_handle_t touch = NULL;
#if BOARD_LCD_ENABLED
    touch = bsp_init_touch();
#endif

    /* 4. 初始化 LVGL Port (内部创建 lv_init + task + tick timer) */
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    /* 5. 注册 MIPI DSI 显示 */
#if BOARD_LCD_ENABLED
    static lvgl_port_display_cfg_t disp_cfg = {
        .io_handle    = NULL,          /* DPI 模式不需要 io_handle */
        .panel_handle = NULL,          /* 运行时动态赋值，避开 C 静态常量限制 */
        .buffer_size  = BOARD_LCD_H_RES * BOARD_LVGL_DRAW_BUF_LINES,
        .double_buffer = true,
        .hres         = BOARD_LCD_H_RES,
        .vres         = BOARD_LCD_V_RES,
        .color_format = LV_COLOR_FORMAT_RGB888,
        .flags = {
            .buff_spiram = false,
            .buff_dma    = false,
            .sw_rotate   = false,
        },
    };
    disp_cfg.panel_handle = lcd_panel;
    static const lvgl_port_display_dsi_cfg_t dsi_cfg = {
        .flags = {
            .avoid_tearing = false,
        },
    };
    lv_display_t *display = lvgl_port_add_disp_dsi(&disp_cfg, &dsi_cfg);
    assert(display);

    /* 恢复原生竖屏，无需软件旋转 */

    /* 6. 注册触摸输入 */
    if (touch) {
        const lvgl_port_touch_cfg_t touch_cfg = {
            .disp   = display,
            .handle = touch,
        };
        lv_indev_t *indev = lvgl_port_add_touch(&touch_cfg);
        assert(indev);
    }

    /* 7. 创建演示 UI */
    lvgl_port_lock(0);

    /* 周期性主动让出 CPU 以喂狗，防止基准测试等 100% 渲染负荷下 IDLE 任务饿死 */
    lv_timer_create(wdt_feed_timer_cb, 500, NULL);

#if LV_USE_DEMO_BENCHMARK
    lv_demo_benchmark();
#elif LV_USE_DEMO_WIDGETS
    lv_demo_widgets();
#else
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x003366), 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "WTDKP4C5-S1\nJD9366 LCD + Touch\nLVGL Demo (No Widget Demo)");
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);
#endif

    lvgl_port_unlock();
    ESP_LOGI(TAG, "Demo UI created");
#endif  /* BOARD_LCD_ENABLED */

    ESP_LOGI(TAG, "System ready!");
}
