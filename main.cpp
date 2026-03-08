#include "lvgl.h"
#include "dino_game.hpp"
#include <unistd.h>
#include <pthread.h>

#if 0
// ==============================================================================
// 1. LINUX FRAME BUFFER & INPUT DRIVERS (EXAMPLE)
// ==============================================================================
// If using official lv_drivers, you'd include them here.
// e.g.,
// #include "lv_drivers/display/fbdev.h"
// #include "lv_drivers/indev/evdev.h"
//
// To init:
//   fbdev_init();
//   evdev_init();
//   
//   // Register display driver
//   static lv_disp_draw_buf_t draw_buf;
//   static lv_color_t buf[DISP_BUF_SIZE];
//   lv_disp_draw_buf_init(&draw_buf, buf, NULL, DISP_BUF_SIZE);
//   
//   static lv_disp_drv_t disp_drv;
//   lv_disp_drv_init(&disp_drv);
//   disp_drv.draw_buf   = &draw_buf;
//   disp_drv.flush_cb   = fbdev_flush;
//   disp_drv.hor_res    = 800; // your screen width
//   disp_drv.ver_res    = 480; // your screen height
//   lv_disp_drv_register(&disp_drv);
//   
//   // Register input driver
//   static lv_indev_drv_t indev_drv;
//   lv_indev_drv_init(&indev_drv);
//   indev_drv.type = LV_INDEV_TYPE_POINTER; // OR LV_INDEV_TYPE_KEYPAD
//   indev_drv.read_cb = evdev_read;
//   lv_indev_drv_register(&indev_drv);
// ==============================================================================
#endif 

// A mocked stub for development without actual hardware drivers
void hw_init_stub() {
    /* 
     * In a real embedded environment, here you initialize 
     * the frame buffer driver or display controller.
     */
}

int main(int argc, char** argv) {
    // 1. Initialize LVGL
    lv_init();

    // 2. Initialize Hardware / Drivers
    hw_init_stub();

    // NOTE: This assumes LVGL is fully configured with drivers for your target
    // IF running on SDL PC, initialization looks different.
    // Ensure display and input interfaces are registered here.

    // 3. Game Initialization
    // We attach the game to the active screen
    lv_obj_t* scr = lv_scr_act();
    DinoGame game(scr);

    // 4. Main Event Loop
    while (1) {
        lv_timer_handler(); // let the GUI do its work
        usleep(5000); // sleep ~5ms
    }

    return 0;
}
