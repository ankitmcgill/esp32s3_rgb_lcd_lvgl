// DRIVER_LCD
// SEPTEMBER 30, 2025

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_idf_version.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lvgl_port.h"
#include "lv_demos.h"

#include "driver_lcd.h"
#include "define_common_data_types.h"
#include "define_rtos_globals.h"
#include "define_rtos_tasks.h"
#include "bsp.h"

// Extern Variables

// Local Variables
static rtos_component_type_t s_component_type;
static esp_lcd_panel_handle_t s_handle_lcd_panel;
static lv_display_t* s_handle_lvgl_display;

// Local Functions
static bool s_lcd_panel_rgb_init(void);
static bool s_lvgl_port_init(void);
static bool s_add_lcd_screen(void);

// External Functions
bool DRIVER_LCD_Init(void)
{
    // Initialize Driver Lcd

    s_component_type = COMPONENT_TYPE_TASK;

    ESP_LOGI(DEBUG_TAG_DRIVER_LCD, "Type %u. Init", s_component_type);

    // Initialize Display Subsystems
    if(!s_lcd_panel_rgb_init()) goto err;
    if(!s_lvgl_port_init()) goto err;
    if(!s_add_lcd_screen()) goto err;

    return true;
    err:
        return false;
}

void DRIVER_LCD_Demo(void)
{
    // Lvgl Demo
    // For This To Work The Following Should Be Enabed Through Menuconfig
    // 1. Component Config -> LVGL Configuration -> Demos -> Build Demos
    // 1. Component Config -> LVGL Configuration -> Demos -> Benchmark Your System
    // 1. Component Config -> LVGL Configuration -> Others -> Enable System Monitor Component
    // 1. Component Config -> LVGL Configuration -> Others -> Show CPU Usage And Fps Count

    #if defined CONFIG_LV_BUILD_DEMOS && defined CONFIG_LV_USE_SYSMON && defined CONFIG_LV_USE_PERF_MONITOR
        DRIVER_LCD_LVGL_UPDATE(lv_demo_benchmark());
        ESP_LOGI(DEBUG_TAG_DRIVER_LCD, "Lvgl Demo");
    #endif
}

static bool s_lcd_panel_rgb_init(void)
{
    // Initialize LCD Panel & RGB

    esp_err_t ret = ESP_OK;
    s_handle_lcd_panel = NULL;
    esp_lcd_rgb_panel_config_t rgb_panel_config = {
        .clk_src = LCD_CLK_SRC_PLL160M,
        .timings = {
            .pclk_hz = DRIVER_LCD_DISPLAY_PIXEL_CLK_HZ,
            .h_res = 800,
            .v_res = 480,
            .hsync_pulse_width = 40,
            .hsync_back_porch = 40,
            .hsync_front_porch = 48,
            .vsync_pulse_width = 23,
            .vsync_back_porch = 32,
            .vsync_front_porch = 13,
            .flags.pclk_active_neg = true
        },
        .data_width = 16,
        .bits_per_pixel = 16,
        .num_fbs = 2,
        .bounce_buffer_size_px = (800 * 10 * 2),
        .dma_burst_size = 64,
        .flags.fb_in_psram = true,
        .hsync_gpio_num = BSP_LCD_GPIO_HSYNC,
        .vsync_gpio_num = BSP_LCD_GPIO_VSYNC,
        .pclk_gpio_num = BSP_LCD_GPIO_PCLK,
        .de_gpio_num = BSP_LCD_GPIO_DE,
        .disp_gpio_num = BSP_LCD_GPIO_DISP,
        .data_gpio_nums={
            BSP_LCD_GPIO_DATA0,
            BSP_LCD_GPIO_DATA1,
            BSP_LCD_GPIO_DATA2,
            BSP_LCD_GPIO_DATA3,
            BSP_LCD_GPIO_DATA4,
            BSP_LCD_GPIO_DATA5,
            BSP_LCD_GPIO_DATA6,
            BSP_LCD_GPIO_DATA7,
            BSP_LCD_GPIO_DATA8,
            BSP_LCD_GPIO_DATA9,
            BSP_LCD_GPIO_DATA10,
            BSP_LCD_GPIO_DATA11,
            BSP_LCD_GPIO_DATA12,
            BSP_LCD_GPIO_DATA13,
            BSP_LCD_GPIO_DATA14,
            BSP_LCD_GPIO_DATA15
        }
    };

    ESP_GOTO_ON_ERROR(
        esp_lcd_new_rgb_panel(&rgb_panel_config, &s_handle_lcd_panel),
        err,
        DEBUG_TAG_DRIVER_LCD,
        "Lcd Rgb Panel Init Fail"
    );

    ESP_GOTO_ON_ERROR(
        esp_lcd_panel_reset(s_handle_lcd_panel),
        err,
        DEBUG_TAG_DRIVER_LCD,
        "Lcd Rgb Panel Reset Fail"
    );

    ESP_GOTO_ON_ERROR(
        esp_lcd_panel_init(s_handle_lcd_panel),
        err,
        DEBUG_TAG_DRIVER_LCD,
        "Lcd Panel Init Fail"
    );

    return true;
    err:
        return false;
}

static bool s_lvgl_port_init(void)
{
    // Initialize Lvgl Port

    esp_err_t ret = ESP_OK;
    const lvgl_port_cfg_t lvgl_port_config = ESP_LVGL_PORT_INIT_CONFIG();

    ESP_GOTO_ON_ERROR(
        lvgl_port_init(&lvgl_port_config),
        err,
        DEBUG_TAG_DRIVER_LCD,
        "Lvgl Port Init Fail"
    );

    return true;
    err:
        return false;
}

static bool s_add_lcd_screen(void)
{
    // Add Lcd Screen

    s_handle_lvgl_display = NULL;
    uint32_t buff_size = 800 * 480 * sizeof(lv_color16_t);

    const lvgl_port_display_cfg_t display_config = {
        .panel_handle = s_handle_lcd_panel,
        .hres = 800,
        .vres = 480,
        .buffer_size = buff_size,
        .double_buffer = false,
        .monochrome = false,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false
        },
        .flags = {
            .buff_dma = true,
            .buff_spiram = true,
            .full_refresh = true,
            .swap_bytes = false
        }
    };
    const lvgl_port_display_rgb_cfg_t display_rgb_config = {
        .flags = {
            .bb_mode = true,
            .avoid_tearing = true,
        }
    };

    s_handle_lvgl_display = lvgl_port_add_disp_rgb(&display_config, &display_rgb_config);

    return true;
}