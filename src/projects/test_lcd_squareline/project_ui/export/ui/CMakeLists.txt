idf_component_register(SRCS
                            "ui_screen1.c"
                            "ui.c"
                            "ui_comp_hook.c"
                            "ui_helpers.c"
                            "ui_img_images_bg_png.c"
                            "ui_font_FontRoboto80.c"
                            "ui_font_FontRobotoBold120.c"
                        PRIV_REQUIRES
                            lvgl__lvgl
                        INCLUDE_DIRS "")