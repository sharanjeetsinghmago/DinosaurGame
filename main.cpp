#include "lvgl.h"
#include "lv_drivers/display/drm.h"
#include "lv_drivers/indev/libinput_drv.h"
#include <unistd.h>
#include <cstdio>

// Defined in dino_game_standalone.cpp
void create_lvgl_dino_game();

int main(int argc, char **argv)
{
    // 1. Initialize LVGL
    lv_init();

    // 2. Initialize DRM display driver
    drm_init();

    lv_coord_t disp_width = 0, disp_height = 0;
    uint32_t dpi = 0;
    drm_get_sizes(&disp_width, &disp_height, &dpi);
    printf("Display: %dx%d @ %u dpi\n", disp_width, disp_height, dpi);

    // 3. Set up display buffers (double-buffered, full screen)
    uint32_t buf_size = disp_width * disp_height;
    lv_color_t *buf1 = new lv_color_t[buf_size];
    lv_color_t *buf2 = new lv_color_t[buf_size];

    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_size);

    // 4. Register display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf   = &draw_buf;
    disp_drv.flush_cb   = drm_flush;
    disp_drv.hor_res    = disp_width;
    disp_drv.ver_res    = disp_height;
    disp_drv.direct_mode = 0;
    disp_drv.full_refresh = 0;
    disp_drv.antialiasing = 1;
    disp_drv.sw_rotate  = 1;
    lv_disp_drv_register(&disp_drv);

    // 5. Initialize libinput keyboard driver
    libinput_init();

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = libinput_read;
    lv_indev_t *kb_indev = lv_indev_drv_register(&indev_drv);

    // 6. Create a default input group and assign keypad
    lv_group_t *g = lv_group_create();
    lv_group_set_default(g);
    lv_indev_set_group(kb_indev, g);

    // 7. Launch the dinosaur game
    create_lvgl_dino_game();

    // 8. Main event loop
    while (true) {
        lv_timer_handler();
        usleep(5000); // ~5ms tick
    }

    // Cleanup (unreachable but good practice)
    drm_exit();
    delete[] buf1;
    delete[] buf2;

    return 0;
}
