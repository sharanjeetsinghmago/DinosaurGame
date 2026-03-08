cmake_minimum_required(VERSION 3.10)
project(dinosaur_game C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

# Find system-installed LVGL and drivers
pkg_check_modules(LVGL REQUIRED lvgl)
pkg_check_modules(LV_DRIVERS REQUIRED lv-drivers)
pkg_check_modules(LIBDRM REQUIRED libdrm)
pkg_check_modules(LIBINPUT REQUIRED libinput)
pkg_check_modules(FREETYPE REQUIRED freetype2)

add_executable(dino_game
    main.cpp
    dino_game_standalone.cpp
)

target_compile_definitions(dino_game PRIVATE
    # Skip lv_drv_conf.h — we provide our own defines
    LV_DRV_NO_CONF
    # LVGL include style
    LV_LVGL_H_INCLUDE_SIMPLE
    # Enable DRM display driver
    USE_DRM=1
    DRM_CARD="/dev/dri/card0"
    DRM_CONNECTOR_ID=-1
    # Enable libinput input driver
    USE_LIBINPUT=1
)

target_include_directories(dino_game PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LVGL_INCLUDE_DIRS}
    ${LV_DRIVERS_INCLUDE_DIRS}
    ${LIBDRM_INCLUDE_DIRS}
    ${LIBINPUT_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(dino_game PRIVATE
    ${LV_DRIVERS_LIBRARIES}
    ${LVGL_LIBRARIES}
    ${LIBDRM_LIBRARIES}
    ${LIBINPUT_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    pthread
)

target_link_directories(dino_game PRIVATE
    ${LVGL_LIBRARY_DIRS}
    ${LV_DRIVERS_LIBRARY_DIRS}
    ${LIBDRM_LIBRARY_DIRS}
    ${LIBINPUT_LIBRARY_DIRS}
    ${FREETYPE_LIBRARY_DIRS}
)
