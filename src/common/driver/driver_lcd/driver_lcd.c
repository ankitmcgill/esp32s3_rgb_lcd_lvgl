// DRIVER_LCD
// SEPTEMBER 30, 2025

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <sys/lock.h>
#include <sys/param.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_idf_version.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "lvgl.h"
#include "lv_demos.h"
#include "driver_lcd.h"
#include "define_common_data_types.h"
#include "define_rtos_globals.h"
#include "define_rtos_tasks.h"
#include "bsp.h"

// Extern Variables

// Local Variables
static TaskHandle_t s_handle_task_lvgl;
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

    s_handle_semaphore_vsync = xSemaphoreCreateBinary();
    xSemaphoreGive(s_handle_semaphore_vsync);
    // s_lvgl_gui_ready = xSemaphoreCreateBinary();

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
    const esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_color_trans_done = s_lcd_panel_color_trans_cb,
        .on_vsync = s_lcd_panel_vsync_cb
    };
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

    // Create Esp Lcd Panel
    ESP_GOTO_ON_ERROR(esp_lcd_new_rgb_panel(&rgb_panel_config, &s_handle_lcd_panel),
        err,
        DEBUG_TAG_DRIVER_LCD,
        "Esp_lcd Panel Create Fail"
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
        "Esp_lcd Panel Reset Fail"
    );

    // Init Panel
    ESP_GOTO_ON_ERROR(esp_lcd_panel_init(s_handle_lcd_panel),
        err,
        DEBUG_TAG_DRIVER_LCD,
        "Esp_lcd Panel Init Fail"
    );

    // Register Panel Event Callbacks
    ESP_GOTO_ON_ERROR(esp_lcd_rgb_panel_register_event_callbacks(s_handle_lcd_panel, &cbs, NULL),
        err,
        DEBUG_TAG_DRIVER_LCD,
        "ESP_lcd Panel Cb Register Fail"
    );

    ESP_LOGI(DEBUG_TAG_DRIVER_LCD, "Lcd Panel Setup Done");
    return true;

    err:
        return false;
}

static bool s_lvgl_setup(void)
{
    // Initialize Lvgl Port

    esp_err_t ret = ESP_OK;
    lv_color16_t* buf1 = NULL;
    lv_color16_t* buf2 = NULL;
    size_t buf_size = (DRIVER_LCD_DISPLAY_RESOLUTION_X * DRIVER_LCD_DISPLAY_RESOLUTION_Y * sizeof(lv_color16_t));

    // Initialize Lvgl
    lv_init();

    // Allocate Buffers
    buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    buf2 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    assert(buf1 && buf2);

    ESP_LOGI(DEBUG_TAG_LVGL, "Lvgl buf0 buf1 Allocated");

    // Create An Lvgl Display & Initialize Buffers
    lv_display_t *lvgl_display = lv_display_create(DRIVER_LCD_DISPLAY_RESOLUTION_X,DRIVER_LCD_DISPLAY_RESOLUTION_Y);
    lv_display_set_buffers(lvgl_display, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_FULL);
    assert(lvgl_display);

    // Set Color Depth
    // Set Display Rotation
    // Set Cb Function That Copies Rendered Image To Display Area
    lv_display_set_color_format(lvgl_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_rotation(lvgl_display, LV_DISPLAY_ROTATION_180);
    lv_display_set_flush_cb(lvgl_display, s_lvgl_flush_cb);

    ESP_LOGI(DEBUG_TAG_LVGL, "Lvgl Display Created");

    // Create Lvgl Task
    xTaskCreate(
        s_task_lvgl,
        DEBUG_TAG_LVGL,
        TASK_STACK_DEPTH_LVGL,
        NULL,
        TASK_PRIORITY_LVGL,
        &s_handle_task_lvgl
    );

    ESP_LOGI(DEBUG_TAG_LVGL, "Lvgl Task Created");
    return true;
}

static void s_task_lvgl(void *arg)
{
    // LvgL Task

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

    // Tell Lvgl How Many Milliseconds Have Elapsed
    lv_tick_inc(DRIVER_LCD_LVGL_TICK_PERIOD_MS);
}

static bool s_lcd_panel_vsync_cb(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data)
{
    // Esp_lcd Panel Vsync Cb

    ESP_EARLY_LOGI(DEBUG_TAG_DRIVER_LCD, "vsync cb");

    BaseType_t high_task_awoken = pdFALSE;

    // Wait Till Lvgl Has Finished
    // if(xSemaphoreTakeFromISR(s_lvgl_gui_ready, &high_task_awoken) == pdTRUE){
        xSemaphoreGiveFromISR( s_handle_semaphore_vsync , &high_task_awoken);
    // }

    // lv_display_t *disp = (lv_display_t *)user_ctx;
    // lv_display_flush_ready(disp);

    return (high_task_awoken == pdTRUE);
}

static bool s_lcd_panel_color_trans_cb(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
{
    // Esp_lcd Panel Color Trans Done Cb

    ESP_EARLY_LOGI(DEBUG_TAG_DRIVER_LCD, "color trans done cb");

    // Notify Lvgl Flush Ready

    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);

    return false;
}

static void s_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    // Lvgl Flush Cb

    // Lvgl Rendering Is Finished
    // xSemaphoreGive(s_lvgl_gui_ready);

    // Pass the Draw Buffer To The Driver
    esp_lcd_panel_draw_bitmap(s_handle_lcd_panel,
        area->x1,
        area->y1,
        area->x2 + 1,
        area->y2 + 1,
        px_map
    );

    // Wait For The VSync Event
    xSemaphoreTake(s_handle_semaphore_vsync, portMAX_DELAY);

    lv_display_flush_ready(disp);
}