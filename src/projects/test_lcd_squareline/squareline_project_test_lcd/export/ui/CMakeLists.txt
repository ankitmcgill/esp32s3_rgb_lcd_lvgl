message(INFORMATION "Compiling Squareline UI Code ...")

file(GLOB_RECURSE
    srcs
        "./*.c"
        "./components/*.c"
        "./screens/*.c"
)

set(
    include_dirs
        "./"
        "./screens"
)

idf_component_register(
    SRCS 
        ${srcs}
    INCLUDE_DIRS 
        ${include_dirs}
    REQUIRES
        lvgl__lvgl
)

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)