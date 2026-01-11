idf_component_register(SRCS ui_screen1.c
                            ui.c
                            ui_comp_hook.c
                            ui_helpers.c
                            ui_img_images_img_clear_day_png.c
                            ui_img_images_button_red_png.c
                            ui_img_images_location_png.c
                            ui_img_images_temperature1_png.c
                            ui_img_images_humidity1_png.c
                            ui_img_images_button_green_png.c
                            ui_img_images_img_clear_night_png.c
                            ui_font_fontinter120.c
                            ui_font_fontinter20.c
                            ui_font_fontinter30.c
                        INCLUDE_DIRS
                            "."
                        PRIV_REQUIRES
                            lvgl__lvgl)