cmake_minimum_required(VERSION 3.10)
project(lvgl_dino_game C CXX)

# Require C++14 for our game logic (or newer if needed)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Assuming LVGL is placed in a subdirectory called "lvgl"
# If LVGL is installed system-wide, use find_package(lvgl REQUIRED) instead
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lvgl/CMakeLists.txt")
    add_subdirectory(lvgl)
else()
    message(WARNING "Local lvgl folder not found. Please ensure LVGL is available.")
    # Attempt to find system installed LVGL (if applicable)
    # find_package(lvgl REQUIRED)
endif()

# We might also need drivers like lv_drivers if you are using fbdev/evdev
# if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lv_drivers/CMakeLists.txt")
#    add_subdirectory(lv_drivers)
# endif()

add_executable(dino_game
    main.cpp
    dino_game.cpp
)

# Link directories & libraries (update based on how lvgl is provided)
# If using the subdirectory method:
if(TARGET lvgl)
    target_link_libraries(dino_game PRIVATE lvgl)
endif()

# If using lv_drivers
# if(TARGET lv_drivers)
#    target_link_libraries(dino_game PRIVATE lv_drivers)
# endif()

# Add include paths
target_include_directories(dino_game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
