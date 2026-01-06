idf_component_register(SRCS ui_screen1.c
                            ui.c
                            ui_comp_hook.c
                            ui_helpers.c
                            ui_img_images_img_clear_day_png.c
                            ui_img_images_location_png.c
                            ui_font_fontinter120.c
                            ui_font_fontinter20.c
                            ui_font_fontinter30.c
                        INCLUDE_DIRS
                            "."
                        PRIV_REQUIRES
                            lvgl__lvgl)