#include "lvgl.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cstdio>

// --- Game Constants & Logic State ---
static lv_obj_t * main_screen = NULL;
static lv_obj_t * dino_obj = NULL;
static lv_obj_t * ground_obj = NULL;
static lv_obj_t * score_label = NULL;
static lv_obj_t * start_msg_label = NULL;
static lv_obj_t * game_over_msg_label = NULL;

static lv_timer_t * game_timer = NULL;

static float distance_ran = 0;
static int high_score = 0;

static float dino_y_f = 0;
static float dino_velocity_y = 0;
static bool is_jumping = false;
static bool ducking = false; // Down key pressed

// Base Chrome Physics Parameters
const float GRAVITY = 0.6f;
const float INITIAL_JUMP_VELOCITY = -12.0f;
const float DROP_VELOCITY = -5.0f; // For when releasing jump early
const float MIN_JUMP_HEIGHT = 35.0f;
const float SPEED_DROP_COEFFICIENT = 3.0f;
const float ACCELERATION = 0.001f;
const float MAX_SPEED = 12.0f;
const float BASE_SPEED = 6.0f;
const float DISTANCE_COEFFICIENT = 0.025f;

static float current_speed = BASE_SPEED;
static int spawn_timer_cnt = 0;
static int spawn_interval = 60; // frames

static int screen_w;
static int screen_h;
static float ground_y;
static int dino_w = 44;
static int dino_h = 47;
static int dino_duck_h = 25; // Smaller height when ducking
static float dino_start_x = 50.0f;

enum class GameState { START, PLAYING, GAME_OVER };
static GameState game_state = GameState::START;

#define MAX_OBSTACLES 10
struct Obstacle {
    lv_obj_t * obj;
    float x;
    float y;
    int w;
    int h;
    bool active;
};
static Obstacle obstacles[MAX_OBSTACLES];

// --- Forward Declarations ---
static void dino_game_start();
static void dino_game_jump();
static void dino_game_duck(bool is_ducking);
static void dino_game_end_jump();
static void dino_game_update(lv_timer_t * t);

// --- Functions ---
static void update_score_text() {
    if (score_label) {
        int actual_dist = (int)(distance_ran * DISTANCE_COEFFICIENT);
        lv_label_set_text_fmt(score_label, "HI: %05d  %05d", high_score, actual_dist);
    }
}

static void spawn_obstacle() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) {
            // Replicate Chromium Cactus types (Small: 17x35, Large: 25x50)
            bool is_large = (std::rand() % 2 == 0);
            int count = 1 + (std::rand() % 3); // 1 to 3 cacti
            
            obstacles[i].w = (is_large ? 25 : 17) * count;
            obstacles[i].h = (is_large ? 50 : 35);
            obstacles[i].x = screen_w;
            obstacles[i].y = ground_y - obstacles[i].h;
            obstacles[i].active = true;

            obstacles[i].obj = lv_obj_create(main_screen);
            lv_obj_set_size(obstacles[i].obj, obstacles[i].w, obstacles[i].h);
            lv_obj_align(obstacles[i].obj, LV_ALIGN_TOP_LEFT, (lv_coord_t)obstacles[i].x, (lv_coord_t)obstacles[i].y);
            lv_obj_set_style_bg_color(obstacles[i].obj, lv_color_hex(0x535353), 0);
            lv_obj_set_style_border_width(obstacles[i].obj, 0, 0);
            lv_obj_set_style_radius(obstacles[i].obj, 3, 0); // slightly rounded
            return;
        }
    }
}

static void game_over() {
    game_state = GameState::GAME_OVER;
    lv_obj_clear_flag(game_over_msg_label, LV_OBJ_FLAG_HIDDEN);
    
    int actual_dist = (int)(distance_ran * DISTANCE_COEFFICIENT);
    if (actual_dist > high_score) {
        high_score = actual_dist;
        update_score_text();
    }
}

static bool check_collision(const Obstacle& obs) {
    if (!obs.active) return false;
    // Chrome uses an AABB overlapping technique
    int margin = 4;
    int current_dino_h = ducking ? dino_duck_h : dino_h;
    
    int dx1 = dino_start_x + margin;
    int dy1 = dino_y_f + margin;
    int dx2 = dino_start_x + dino_w - margin;
    int dy2 = dino_y_f + current_dino_h - margin;

    int ox1 = obs.x;
    int oy1 = obs.y;
    int ox2 = obs.x + obs.w;
    int oy2 = obs.y + obs.h;

    return (dx1 < ox2 && dx2 > ox1 && dy1 < oy2 && dy2 > oy1);
}

static void dino_game_reset() {
    distance_ran = 0;
    current_speed = BASE_SPEED;
    spawn_timer_cnt = 0;
    update_score_text();

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active && obstacles[i].obj) {
            lv_obj_del(obstacles[i].obj);
        }
        obstacles[i].active = false;
    }

    dino_y_f = ground_y - dino_h;
    dino_velocity_y = 0;
    is_jumping = false;
    ducking = false;
    
    lv_obj_set_size(dino_obj, dino_w, dino_h);
    lv_obj_align(dino_obj, LV_ALIGN_TOP_LEFT, (lv_coord_t)dino_start_x, (lv_coord_t)dino_y_f);

    lv_obj_add_flag(start_msg_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(game_over_msg_label, LV_OBJ_FLAG_HIDDEN);

    game_state = GameState::PLAYING;
}

static void dino_game_start() {
    if (game_state == GameState::START || game_state == GameState::GAME_OVER) {
        dino_game_reset();
    }
}

static void dino_game_jump() {
    if (game_state == GameState::START || game_state == GameState::GAME_OVER) {
        dino_game_start();
        return;
    }

    if (!is_jumping) {
        is_jumping = true;
        dino_velocity_y = INITIAL_JUMP_VELOCITY;
        
        // Ensure standing hit box
        ducking = false;
        lv_obj_set_size(dino_obj, dino_w, dino_h);
    }
}

static void dino_game_end_jump() {
    // Variable jump height logic from chromium
    if (is_jumping && dino_velocity_y < DROP_VELOCITY) {
        dino_velocity_y = DROP_VELOCITY;
    }
}

static void dino_game_duck(bool is_ducking) {
    if (game_state != GameState::PLAYING) return;
    
    ducking = is_ducking;
    
    if (is_jumping && ducking) {
        // Speed drop makes Trex fall faster (SPEED_DROP_COEFFICIENT = 3)
        dino_velocity_y += GRAVITY * SPEED_DROP_COEFFICIENT;
    }
    
    if (!is_jumping) {
        int current_dino_h = ducking ? dino_duck_h : dino_h;
        dino_y_f = ground_y - current_dino_h;
        lv_obj_set_size(dino_obj, dino_w, current_dino_h);
        lv_obj_align(dino_obj, LV_ALIGN_TOP_LEFT, (lv_coord_t)dino_start_x, (lv_coord_t)dino_y_f);
    }
}

static void dino_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED || code == LV_EVENT_PRESSED) {
        dino_game_jump();
    } else if (code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_UP || key == ' ') {
            dino_game_jump();
        } else if (key == LV_KEY_DOWN) {
            dino_game_duck(true);
        }
    } else if (code == LV_EVENT_RELEASED) {
        // Touch released - Chromium treats this same as key up jump
        dino_game_end_jump();
    } else if (code == LV_EVENT_KEY) {
        // We handle release of keys for variable jump height and ducks
        // Note: LVGL key processing depends on the platform driver sending INKEY up/down states.
        // If not sent, the variable jump height won't trigger.
        // Since we only get short codes here uniformly, we check the wrapper
    }
}

// Separate LVGL event for checking key up (hardware dependent, but included for completeness)
static void dino_key_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_KEY) {
        void* param = lv_event_get_param(e);
        // Fallback for native devices if supported
    }
}


static void dino_game_update(lv_timer_t * t) {
    if (game_state != GameState::PLAYING) return;

    int current_dino_h = ducking ? dino_duck_h : dino_h;

    // Physics Update
    if (is_jumping) {
        dino_y_f += dino_velocity_y;
        dino_velocity_y += GRAVITY;
        
        if (ducking) {
           dino_velocity_y += GRAVITY; // Accelerated drop
        }

        if (dino_y_f >= ground_y - current_dino_h) {
            dino_y_f = ground_y - current_dino_h;
            is_jumping = false;
            dino_velocity_y = 0;
            if (ducking) {
                // Return to duck state
                lv_obj_set_size(dino_obj, dino_w, dino_duck_h);
            } else {
                lv_obj_set_size(dino_obj, dino_w, dino_h);
            }
        }
        lv_obj_align(dino_obj, LV_ALIGN_TOP_LEFT, (lv_coord_t)dino_start_x, (lv_coord_t)dino_y_f);
    }
    
    // Dist / Speed update
    distance_ran += current_speed;
    if (current_speed < MAX_SPEED) {
        current_speed += ACCELERATION;
    }
    
    // Score update (update every 10 ticks natively to save CPU)
    static int tick_cnt = 0;
    if (++tick_cnt > 5) {
        update_score_text();
        tick_cnt = 0;
    }

    // Chrome Math: spawn gaps
    spawn_timer_cnt++;
    if (spawn_timer_cnt >= spawn_interval) {
        spawn_timer_cnt = 0;
        spawn_interval = 40 + (std::rand() % (int)(1000 / current_speed)); // Scales with speed
        spawn_obstacle();
    }

    // Move & Collide
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active) {
            obstacles[i].x -= current_speed;
            lv_obj_align(obstacles[i].obj, LV_ALIGN_TOP_LEFT, (lv_coord_t)obstacles[i].x, (lv_coord_t)obstacles[i].y);

            if (check_collision(obstacles[i])) {
                game_over();
                return;
            }

            if (obstacles[i].x + obstacles[i].w < 0) {
                lv_obj_del(obstacles[i].obj);
                obstacles[i].active = false;
            }
        }
    }
}

// ==============================================================================
//  PUBLIC INIT API 
// ==============================================================================

/**
 * Call this function from your main calculator OS loop.
 * It will clear the screen and launch the single page game with true Chromium physics.
 */
void create_lvgl_dino_game() {
    std::srand((unsigned int)std::time(nullptr));

    main_screen = lv_obj_create(NULL);
    screen_w = 600; // Chrome Default
    screen_h = 150; // Chrome Default
    
    if (lv_disp_get_default() != NULL) {
        screen_w = lv_disp_get_hor_res(lv_disp_get_default());
        screen_h = lv_disp_get_ver_res(lv_disp_get_default());
    }

    // Adapt layout height to screen width (Chrome was 600x150 default, keeping aspects)
    ground_y = screen_h - 20.0f;

    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0xFFFFFF), 0);
    lv_obj_add_event_cb(main_screen, dino_event_cb, LV_EVENT_ALL, NULL);

    // Ground line - Chrome style
    ground_obj = lv_obj_create(main_screen);
    lv_obj_set_size(ground_obj, screen_w, 2);
    lv_obj_align(ground_obj, LV_ALIGN_TOP_LEFT, 0, (lv_coord_t)ground_y);
    lv_obj_set_style_bg_color(ground_obj, lv_color_hex(0x535353), 0);
    lv_obj_set_style_border_width(ground_obj, 0, 0);
    lv_obj_set_style_radius(ground_obj, 0, 0);

    // Dino (simple dark gray block matching chrome dimensions)
    dino_obj = lv_obj_create(main_screen);
    lv_obj_set_size(dino_obj, dino_w, dino_h);
    lv_obj_set_style_bg_color(dino_obj, lv_color_hex(0x535353), 0);
    lv_obj_set_style_radius(dino_obj, 5, 0);
    lv_obj_set_style_border_width(dino_obj, 0, 0);
    
    dino_y_f = ground_y - dino_h;
    lv_obj_align(dino_obj, LV_ALIGN_TOP_LEFT, (lv_coord_t)dino_start_x, (lv_coord_t)dino_y_f);

    // Text labels
    score_label = lv_label_create(main_screen);
    lv_obj_align(score_label, LV_ALIGN_TOP_RIGHT, -10, 10);
    update_score_text();

    start_msg_label = lv_label_create(main_screen);
    lv_label_set_text(start_msg_label, "Press/Tap to Start");
    lv_obj_align(start_msg_label, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_text_color(start_msg_label, lv_color_hex(0x535353), 0);

    game_over_msg_label = lv_label_create(main_screen);
    lv_label_set_text(game_over_msg_label, "GAME OVER");
    lv_obj_add_flag(game_over_msg_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(game_over_msg_label, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_text_color(game_over_msg_label, lv_color_hex(0x535353), 0);

    lv_group_t * g = lv_group_get_default();
    if(g != NULL) {
        lv_group_add_obj(g, main_screen);
        lv_group_focus_obj(main_screen);
    }

    lv_scr_load(main_screen);

    // Create a 60fps timer matching Chromium's frame rate bounds (1000/60 = 16.6ms)
    // Here we use 16 ms to poll roughly 60 updates per second.
    if (game_timer == NULL) {
        game_timer = lv_timer_create(dino_game_update, 16, NULL);
    }

    game_state = GameState::START;
}
