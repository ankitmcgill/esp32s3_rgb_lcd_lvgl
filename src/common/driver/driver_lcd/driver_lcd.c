// DRIVER_LCD
// SEPTEMBER 30, 2025

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

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
#include "display/lv_display.h" 
#include "lv_demos.h"

#include "driver_lcd.h"
#include "define_common_data_types.h"
#include "define_rtos_tasks.h"
#include "bsp.h"

// Extern Variables

// Local Variables
static TaskHandle_t s_handle_task_lvgl;
static rtos_component_type_t s_component_type;
static esp_lcd_panel_handle_t s_handle_lcd_panel;
static SemaphoreHandle_t s_handle_semaphore_vsync;
static esp_timer_handle_t s_timer;
static _lock_t s_lvgl_api_lock;

// Local Functions
static bool s_lcd_panel_setup(void);
static bool s_lvgl_setup(void);
static void s_task_lvgl(void *arg);
static void s_lvgl_tick_timer_cb(void* arg);
static bool s_lcd_panel_vsync_cb(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data);
static bool s_lcd_panel_color_trans_cb(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx);
static void s_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

// External Functions
bool DRIVER_LCD_Init(void)
{
    // Initialize Driver Lcd

    s_component_type = COMPONENT_TYPE_TASK;

    ESP_LOGI(DEBUG_TAG_DRIVER_LCD, "Type %u. Init", s_component_type);

    s_handle_semaphore_vsync = xSemaphoreCreateBinary();
    assert(s_handle_semaphore_vsync);

    // Initialize Display Subsystems
    if(!s_lcd_panel_setup()) goto err;
    if(!s_lvgl_setup()) goto err;

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
        ESP_LOGI(DEBUG_TAG_DRIVER_LCD, "Lvgl Demo", s_component_type);
        lv_demo_benchmark();
    #endif
}

static bool s_lcd_panel_setup(void)
{
    // Initialize & Setup Esp Lcd Panel

    esp_err_t ret = ESP_OK;
    s_handle_lcd_panel = NULL;
    const esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_color_trans_done = s_lcd_panel_color_trans_cb,
        .on_vsync = s_lcd_panel_vsync_cb
    };
    esp_lcd_rgb_panel_config_t rgb_panel_config = {
        .clk_src = LCD_CLK_SRC_PLL160M,
        .data_width = 16,
        .bits_per_pixel = 16,
        .num_fbs = 2,
        .psram_trans_align = 64,
        .bounce_buffer_size_px = 0,
        .flags.fb_in_psram = true,
        .timings = {
            .pclk_hz = (21 * 1000 * 1000),
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

    // Reset Panel
    ESP_GOTO_ON_ERROR(esp_lcd_panel_reset(s_handle_lcd_panel),
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
    // Initialize & Setup Lvgl

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

    ESP_LOGI(DEBUG_TAG_DRIVER_LCD, "Lvgl buf0 buf1 Allocated");

    // Create An Lvgl Display & Initialize Buffers
    lv_display_t *lvgl_display = lv_display_create(DRIVER_LCD_DISPLAY_RESOLUTION_X, DRIVER_LCD_DISPLAY_RESOLUTION_Y);
    lv_display_set_buffers(lvgl_display, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_FULL);
    assert(lvgl_display);

    /* REQUIRED for RGB */
    lv_display_set_render_mode(lvgl_display, LV_DISPLAY_RENDER_MODE_DIRECT | LV_DISPLAY_RENDER_MODE_FULL);
    // lv_display_set_full_refresh(lvgl_display, true);

    // Set Color Depth
    // Set Display Rotation
    // Set Cb Function That Copies Rendered Image To Display Area
    lv_display_set_color_format(lvgl_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_rotation(lvgl_display, LV_DISPLAY_ROTATION_180);
    lv_display_set_flush_cb(lvgl_display, s_lvgl_flush_cb);

    ESP_LOGI(DEBUG_TAG_DRIVER_LCD, "Lvgl Display Created");

    // Lvgl Tick Timer
    const esp_timer_create_args_t timer_args = {
        .callback = s_lvgl_tick_timer_cb,
        .name = "lv_tick"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &s_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_timer, DRIVER_LCD_LVGL_TICK_PERIOD_MS * 1000));

    // Create Lvgl Task
    xTaskCreate(
        s_task_lvgl,
        "t-lvgl",
        TASK_STACK_DEPTH_LVGL,
        NULL,
        TASK_PRIORITY_LVGL,
        &s_handle_task_lvgl
    );

    ESP_LOGI(DEBUG_TAG_DRIVER_LCD, "Lvgl Task Created");
    return true;
}

static void s_task_lvgl(void *arg)
{
    // LvgL Task

    ESP_LOGI(DEBUG_TAG_DRIVER_LCD, "Starting LVGL task");

    while(true) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(DRIVER_LCD_LVGL_TASK_PERIOD_MS));
    }
}

static void s_lvgl_tick_timer_cb(void* arg)
{
    // Lvgl Tick Timer Cb

    // Tell Lvgl How Many Milliseconds Have Elapsed
    lv_tick_inc(DRIVER_LCD_LVGL_TICK_PERIOD_MS);
}

static bool s_lcd_panel_vsync_cb(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data)
{
    // Esp_lcd Panel Vsync Cb

    // ESP_EARLY_LOGI(DEBUG_TAG_DRIVER_LCD, "vsync cb");
    BaseType_t high_task_awoken = pdFALSE;

    // Wait Till Lvgl Has Finished
        xSemaphoreGiveFromISR( s_handle_semaphore_vsync , &high_task_awoken);

    return (high_task_awoken == pdTRUE);
}

static bool s_lcd_panel_color_trans_cb(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
{
    // Esp_lcd Panel Color Trans Done Cb

    // ESP_EARLY_LOGI(DEBUG_TAG_DRIVER_LCD, "color trans done cb");

    // Notify Lvgl Flush Ready

    // lv_display_t *disp = (lv_display_t *)user_ctx;
    // lv_display_flush_ready(disp);

    return false;
}

static void s_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    // Lvgl Flush Cb
    // Pass the Draw Buffer To The Driver

    // Wait For The VSync Event - With A Timeout
    // Forever Blocking Is Bad
    if (xSemaphoreTake(s_handle_semaphore_vsync, pdMS_TO_TICKS(20)) != pdTRUE) {
        ESP_LOGW(DEBUG_TAG_DRIVER_LCD, "VSYNC timeout");
    }
    // ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(s_handle_lcd_panel,
    //     area->x1,
    //     area->y1,
    //     area->x2 + 1,
    //     area->y2 + 1,
    //     px_map
    // ));
    esp_lcd_rgb_panel_swap_buffers(s_handle_lcd_panel);

    lv_display_flush_ready(disp);
}