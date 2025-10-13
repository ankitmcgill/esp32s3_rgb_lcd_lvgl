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
#include "esp_timer.h"
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
static _lock_t s_lvgl_api_lock;
static SemaphoreHandle_t s_lvgl_vsync_end;
static SemaphoreHandle_t s_lvgl_gui_ready;

// Local Functions
static bool s_lcd_panel_and_rgb_init(void);
static bool s_lvgl_init(void);
static bool s_lvgl_setup(void);
static void s_task_lvgl(void *arg);
static bool s_notify_lvgl_flush_ready(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx);
static void s_lvgl_tick_cb(void *arg);
static bool s_esp_lcd_on_vsync_cb(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data);
static void s_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

// External Functions
bool DRIVER_LCD_Init(void)
{
    // Initialize Driver Lcd

    s_component_type = COMPONENT_TYPE_TASK;

    ESP_LOGI(DEBUG_TAG_DRIVER_LCD, "Type %u. Init", s_component_type);

    s_lvgl_vsync_end = xSemaphoreCreateBinary();
    s_lvgl_gui_ready = xSemaphoreCreateBinary();

    // Initialize Display Subsystems
    if(!s_lcd_panel_and_rgb_init()) goto err;
    if(!s_lvgl_init()) goto err;
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
        // DRIVER_LCD_LVGL_UPDATE(lv_demo_benchmark());
        
        _lock_acquire(&s_lvgl_api_lock);
        lv_demo_benchmark();
        _lock_release(&s_lvgl_api_lock);
        ESP_LOGI(DEBUG_TAG_DRIVER_LCD, "Lvgl Demo", s_component_type);
    #endif
}

static bool s_lcd_panel_and_rgb_init(void)
{
    // Initialize LCD
    esp_err_t ret = ESP_OK;
    s_handle_lcd_panel = NULL;
        const esp_lcd_rgb_panel_event_callbacks_t cbs = {
        // .on_color_trans_done = s_notify_lvgl_flush_ready,
        .on_vsync = s_esp_lcd_on_vsync_cb
    };
    esp_lcd_rgb_panel_config_t rgb_panel_config = {
        .clk_src = LCD_CLK_SRC_PLL160M,
        .timings = {
            .pclk_hz = DRIVER_LCD_DISPLAY_PIXEL_CLK_HZ,
            .h_res = DRIVER_LCD_DISPLAY_RESOLUTION_X,
            .v_res = DRIVER_LCD_DISPLAY_RESOLUTION_Y,
            .hsync_pulse_width = 4,
            .hsync_back_porch = 8,
            .hsync_front_porch = 8,
            .vsync_pulse_width = 4,
            .vsync_back_porch = 8,
            .vsync_front_porch = 8,
            .flags.pclk_active_neg = true
        },
        .data_width = DRIVER_LCD_DISPLAY_DATA_WIDTH,
        .bits_per_pixel = DRIVER_LCD_DISPLAY_BITS_PER_PIXEL,
        .num_fbs = DRIVER_LCD_DISPLAY_NUM_FBS,
        .flags.fb_in_psram = true,
        .psram_trans_align = 64,
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
    ESP_GOTO_ON_ERROR(
        esp_lcd_new_rgb_panel(&rgb_panel_config, &s_handle_lcd_panel),
        err,
        DEBUG_TAG_DRIVER_LCD,
        "Esp_lcd Rgb Panel Init Fail"
    );

    // Reset Panel
    ESP_GOTO_ON_ERROR(
        esp_lcd_panel_reset(s_handle_lcd_panel),
        err,
        DEBUG_TAG_DRIVER_LCD,
        "Esp_lcd Panel Init Fail"
    );

    // Init Panel
    ESP_GOTO_ON_ERROR(
        esp_lcd_panel_init(s_handle_lcd_panel),
        err,
        DEBUG_TAG_DRIVER_LCD,
        "Esp_lcd Panel Init Fail"
    );

    // Register Panel Event Callback For Vsync
    ESP_GOTO_ON_ERROR(
        esp_lcd_rgb_panel_register_event_callbacks(s_handle_lcd_panel, &cbs, NULL),
        err,
        DEBUG_TAG_DRIVER_LCD,
        "ESP Lcd Panel Cb Register Fail"
    );

    return true;
    err:
        return false;
}

static bool s_lvgl_init(void)
{
    // Initialize Lvgl

    lv_init();

    return true;
}

static bool s_lvgl_setup(void)
{
    // Setup Lvgl

    esp_err_t ret = ESP_OK;
    lv_color16_t* buf1 = NULL;
    lv_color16_t* buf2 = NULL;
    esp_timer_handle_t lvgl_tick_timer;
    const esp_timer_create_args_t lvgl_timer_args = {
        .callback = &s_lvgl_tick_cb,
        .name = "lvgl_tick"
    };

    // Create An Lvgl Display
    lv_display_t* lvgl_display = lv_display_create(
        DRIVER_LCD_DISPLAY_RESOLUTION_X, 
        DRIVER_LCD_DISPLAY_RESOLUTION_Y
    );

    // Allocate Buffers Used By Lvgl
    // Esp_Lcd In RGB Mode Already Allocates Buffer As Part Of Esp_Lcd Drive Create
    ESP_GOTO_ON_ERROR(
        esp_lcd_rgb_panel_get_frame_buffer(s_handle_lcd_panel, 
            DRIVER_LCD_DISPLAY_NUM_FBS, 
            (void*)&buf1, 
            (void*)&buf2
        ),
        err,
        DEBUG_TAG_DRIVER_LCD,
        "Esp_Lcd Get Buffers Fail"
    );
    assert(buf1 != NULL || buf2 != NULL);

    // Initialize Lvgl Draw Buffers
    lv_display_set_buffers(lvgl_display, 
        buf1, 
        buf2, 
        DRIVER_LCD_DISPLAY_RESOLUTION_X * DRIVER_LCD_DISPLAY_RESOLUTION_Y * sizeof(lv_color16_t), 
        LV_DISPLAY_RENDER_MODE_FULL
    );

    // Attach ESP Lcd Handle To Lvgl Handle
    lv_display_set_user_data(lvgl_display, s_handle_lcd_panel);

    // Set Color Depth
    lv_display_set_color_format(lvgl_display, LV_COLOR_FORMAT_RGB565);

    // Set Cb Function That Copies Rendered Image To Display Area
    lv_display_set_flush_cb(lvgl_display, s_lvgl_flush_cb);

    // Set Display Rotation
    lv_display_set_rotation(lvgl_display, LV_DISPLAY_ROTATION_180);

    // Setup Lvgl Tick Timer
    lvgl_tick_timer = NULL;
    ESP_GOTO_ON_ERROR(
        esp_timer_create(&lvgl_timer_args, &lvgl_tick_timer),
        err,
        DEBUG_TAG_DRIVER_LCD,
        "Lvgl Tick Timer Create Fail"
    );
    ESP_GOTO_ON_ERROR(
        esp_timer_start_periodic(lvgl_tick_timer, DRIVER_LCD_LVGL_TICK_PERIOD_MS * 1000),
        err,
        DEBUG_TAG_DRIVER_LCD,
        "Lvgl Tick Start Fail"
    );

    // Create Lvgl Task
    xTaskCreate(
        s_task_lvgl, 
        DEBUG_TAG_LVGL, 
        TASK_STACK_DEPTH_LVGL, 
        NULL, 
        TASK_PRIORITY_LVGL, 
        &s_handle_task_lvgl
    );

    return true;
    err:
        return false;
}

static void s_task_lvgl(void *arg)
{
    // LvgL Task

    ESP_LOGI(DEBUG_TAG_LVGL, "Starting LVGL task");

    uint32_t time_till_next_ms = 0;
    uint32_t time_threshold_ms = (DRIVER_LCD_LVGL_TICK_PERIOD_MS * 1000) / CONFIG_FREERTOS_HZ;
    while(true) {
        _lock_acquire(&s_lvgl_api_lock);
        time_till_next_ms = lv_timer_handler();
        _lock_release(&s_lvgl_api_lock);
        
        // In Case Of Triggering A Task Watch Dog Time Out
        time_till_next_ms = MAX(time_till_next_ms, time_threshold_ms);
        vTaskDelay(time_till_next_ms);
    }
}

static bool s_notify_lvgl_flush_ready(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
{
    // Notify Lvgl Flush Ready

    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    
    return false;
}

static void s_lvgl_tick_cb(void *arg)
{
    // Lvgl Tick Timer Cb

    // Tell Lvgl How Many Milliseconds Have Elapsed
    lv_tick_inc(DRIVER_LCD_LVGL_TICK_PERIOD_MS);
}

static bool s_esp_lcd_on_vsync_cb(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data)
{
    // Esp_lcd Vsync Cb

    BaseType_t high_task_awoken = pdFALSE;

    // Wait Till Lvgl Has Finished
    if(xSemaphoreTakeFromISR(s_lvgl_gui_ready, &high_task_awoken) == pdTRUE){
        xSemaphoreGiveFromISR(s_lvgl_vsync_end, &high_task_awoken);
    }

    return (high_task_awoken == pdTRUE);
}

static void s_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    // Lvgl Flush Cb

    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;

    // Lvgl Rendering Is Finished
    xSemaphoreGive(s_lvgl_gui_ready);

    // Wait For The VSync Event
    xSemaphoreTake(s_lvgl_vsync_end, portMAX_DELAY);
    
    // Pass the Draw Buffer To The Driver
    esp_lcd_panel_draw_bitmap(
        panel_handle,
        offsetx1,
        offsety1,
        offsetx2 + 1,
        offsety2 + 1,
        px_map
    );
    lv_disp_flush_ready(disp);
}