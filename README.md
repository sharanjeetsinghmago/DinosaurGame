# DinosaurGame - LVGL

A self-contained Chrome-like Dinosaur game built for embedded environments (like smart calculators) using C++ and the Light and Versatile Graphics Library (LVGL).

## Features
- **Zero Assets:** Uses native LVGL shapes (`lv_obj_create`, `lv_obj_set_style_bg_color`, etc.) to keep the deployment footprint minimal.
- **Auto-Scaling:** Automatically queries the screen size using `lv_disp_get_hor_res` to scale the game's physics (gravity, jumps, speed, obstacle size) for any display resolution perfectly (e.g. 320x240, 480x320).
- **Embedded Ready:** No external dependencies other than `<cstdlib>`, `<ctime>`, and `"lvgl.h"`. Designed heavily for environments where memory and CPU are constrained.

## Integration / Flashing

To integrate this game into your smart calculator OS or embedded menu:

1. Drop `dino_game_standalone.cpp` into your project's firmware source tree.
2. In your calculator's main OS logic (where an app is selected), expose and call the main game function:

```cpp
// Forward declaration
extern void create_lvgl_dino_game();

// Example callback when the user selects the "Dino Game" app
void on_app_icon_clicked(lv_event_t * e) {
    create_lvgl_dino_game(); 
}
```

3. The function will dynamically create a new LVGL screen, draw the scene, attach the keypads/touch input (`LV_EVENT_CLICKED`, `LV_EVENT_KEY`), and start the physics timer.

## Files
- `dino_game_standalone.cpp`: The single, minimal C/C++ module you need to add to your firmware.
- (Optional) `CMakeLists.txt`, `dino_game.cpp/hpp` & `main.cpp`: Additional structured files if you prefer a class-based architecture over the self-contained functional implementation.