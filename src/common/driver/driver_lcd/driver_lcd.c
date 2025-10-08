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
// #include "esp_lvgl_port.h"
// #include "lv_demos.h"

#include "driver_lcd.h"
#include "define_common_data_types.h"
#include "define_rtos_globals.h"
#include "define_rtos_tasks.h"
#include "bsp.h"

// Extern Variables

// Local Variables
static rtos_component_type_t s_component_type;
// static lv_display_t* s_lvgl_display;
static esp_lcd_panel_handle_t s_handle_lcd_panel;
// static lv_display_t* s_display;
// static lv_display_t* s_lv_display;

// Local Functions
static bool s_lcd_panel_and_rgb_init(void);
// static bool s_lvgl_port_init(void);
// static bool s_add_lcd_screen(void);

// External Functions
bool DRIVER_LCD_Init(void)
{
    // Initialize Driver Lcd

    s_component_type = COMPONENT_TYPE_TASK;

    ESP_LOGI(DEBUG_TAG_DRIVER_LCD, "Type %u. Init", s_component_type);

    // Initialize Display Subsystems
    if(!s_lcd_panel_and_rgb_init()) goto err;
    // if(!s_lvgl_port_init()) goto err;
    // if(!s_add_lcd_screen()) goto err;

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
    #endif
}

static bool s_lcd_panel_and_rgb_init(void)
{
    // Initialize LCD
    esp_err_t ret = ESP_OK;
    s_handle_lcd_panel = NULL;
    esp_lcd_rgb_panel_config_t rgb_panel_config = {
        .clk_src = LCD_CLK_SRC_PLL160M,
        .timings = {
            .pclk_hz = DRIVER_LCD_DISPLAY_PIXEL_CLK_HZ,
            .h_res = DRIVER_LCD_DISPLAY_RESOLUTION_X,
            .v_res = DRIVER_LCD_DISPLAY_RESOLUTION_Y,
            .hsync_pulse_width = 40,
            .hsync_back_porch = 40,
            .hsync_front_porch = 48,
            .vsync_pulse_width = 23,
            .vsync_back_porch = 32,
            .vsync_front_porch = 13,
            .flags.pclk_active_neg = true
        },
        .data_width = DRIVER_LCD_DISPLAY_DATA_WIDTH,
        .bits_per_pixel = DRIVER_LCD_DISPLAY_BITS_PER_PIXEL,
        .num_fbs = DRIVER_LCD_DISPLAY_NUM_FBS,
        .bounce_buffer_size_px = (DRIVER_LCD_DISPLAY_RESOLUTION_X * DRIVER_LCD_RGB_BOUNCE_BUFFER_HEIGHT),
        .dma_burst_size = 64,
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
        },
        .flags.fb_in_psram = true,
    };

    ESP_GOTO_ON_ERROR(
        esp_lcd_new_rgb_panel(&rgb_panel_config, &s_handle_lcd_panel),
        err,
        DEBUG_TAG_DRIVER_LCD,
        "Lcd Rgb Panel Init Fail"
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

// static bool s_lvgl_port_init(void)
// {
//     // Initialize Lvgl

//     esp_err_t ret = ESP_OK;
//     const lvgl_port_cfg_t lvgl_port_config = ESP_LVGL_PORT_INIT_CONFIG();

//     ESP_GOTO_ON_ERROR(
//         lvgl_port_init(&lvgl_port_config),
//         err,
//         DEBUG_TAG_DRIVER_LCD,
//         "Lvgl Port Init Fail"
//     );

//     return true;
//     err:
//         return false;
// }

// static bool s_add_lcd_screen(void)
// {
//     // Add Lcd Screen

//     s_display = NULL;
//     uint32_t buff_size = DRIVER_LCD_DISPLAY_RESOLUTION_X * DRIVER_LCD_DRAW_BUFF_HEIGHT;
//     #if DRIVER_LCD_LVGL_FULL_REFRESH || DRIVER_LCD_LVGL_DIRECT_MODE
//         buff_size = DRIVER_LCD_DISPLAY_RESOLUTION_X * DRIVER_LCD_DISPLAY_RESOLUTION_Y;
//     #endif

//     const lvgl_port_display_cfg_t display_config = {
//         .panel_handle = s_handle_lcd_panel,
//         .buffer_size = buff_size,
//         .double_buffer = DRIVER_LCD_DRAW_BUFF_DOUBLE,
//         .hres = DRIVER_LCD_DISPLAY_RESOLUTION_X,
//         .vres = DRIVER_LCD_DISPLAY_RESOLUTION_Y,
//         .monochrome = false,
//         .color_format = LV_COLOR_FORMAT_RGB565,
//         .rotation = {
//             .swap_xy = false,
//             .mirror_x = false,
//             .mirror_y = false
//         },
//         .flags = {
//             .buff_dma = false,
//             .buff_spiram = false,
//             #if DRIVER_LCD_LVGL_FULL_REFRESH
//             .full_refresh = true,
//             #elif DRIVER_LCD_LVGL_DIRECT_MODE
//             .direct_mode = true,
//             #endif
//             .swap_bytes = false
//         }
//     };
//     const lvgl_port_display_rgb_cfg_t display_rgb_config = {
//         .flags = {
//             #if DRIVER_LCD_RGB_BOUNCE_BUFFER_MODE
//             .bb_mode = true,
//             #else
//             .bb_mode = false,
//             #endif
//             #if DRIVER_LCD_LVGL_AVOID_TEAR
//             .avoid_tearing = true,
//             #else
//             .avoid_tearing = false,
//             #endif
//         }
//     };

//     s_display = lvgl_port_add_disp_rgb(&display_config, &display_rgb_config);

//     return true;
// }