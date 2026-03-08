#include "lvgl.h"
#include <cstdlib>
#include <ctime>

// --- Game Constants & Logic State ---
static lv_obj_t * main_screen = NULL;
static lv_obj_t * dino_obj = NULL;
static lv_obj_t * ground_obj = NULL;
static lv_obj_t * score_label = NULL;
static lv_obj_t * start_msg_label = NULL;
static lv_obj_t * game_over_msg_label = NULL;

static lv_timer_t * game_timer = NULL;

static int score = 0;
static int high_score = 0;
static int dino_y = 0;
static int dino_velocity_y = 0;
static bool is_jumping = false;

// Physics parameters
static int screen_w;
static int screen_h;
static int ground_y;
static int dino_w;
static int dino_h;
static int dino_start_x;
static int jump_strength;
static int gravity;
static int obstacle_speed;

static int spawn_interval = 60;
static int spawn_timer_cnt = 0;

enum class GameState { START, PLAYING, GAME_OVER };
static GameState game_state = GameState::START;

#define MAX_OBSTACLES 10
struct Obstacle {
    lv_obj_t * obj;
    int x;
    int y;
    int w;
    int h;
    bool passed;
    bool active;
};
static Obstacle obstacles[MAX_OBSTACLES];

// --- Forward Declarations ---
static void dino_game_start();
static void dino_game_jump();
static void dino_game_update(lv_timer_t * t);

// --- Functions ---
static void update_score_text() {
    if (score_label) {
        lv_label_set_text_fmt(score_label, "HI: %05d  %05d", high_score, score);
    }
}

static void spawn_obstacle() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) {
            obstacles[i].w = dino_w / 2 + (std::rand() % (dino_w / 2 + 1));
            obstacles[i].h = dino_h / 2 + (std::rand() % (dino_h + 1));
            obstacles[i].x = screen_w;
            obstacles[i].y = ground_y - obstacles[i].h;
            obstacles[i].passed = false;
            obstacles[i].active = true;

            obstacles[i].obj = lv_obj_create(main_screen);
            lv_obj_set_size(obstacles[i].obj, obstacles[i].w, obstacles[i].h);
            lv_obj_align(obstacles[i].obj, LV_ALIGN_TOP_LEFT, obstacles[i].x, obstacles[i].y);
            lv_obj_set_style_bg_color(obstacles[i].obj, lv_color_hex(0x000000), 0); // Black blocks
            lv_obj_set_style_border_width(obstacles[i].obj, 0, 0);
            return;
        }
    }
}

static void game_over() {
    game_state = GameState::GAME_OVER;
    lv_obj_clear_flag(game_over_msg_label, LV_OBJ_FLAG_HIDDEN);
    if (score > high_score) {
        high_score = score;
        update_score_text();
    }
}

static bool check_collision(const Obstacle& obs) {
    if (!obs.active) return false;
    int margin = 4;
    int dx1 = dino_start_x + margin;
    int dy1 = dino_y + margin;
    int dx2 = dino_start_x + dino_w - margin;
    int dy2 = dino_y + dino_h - margin;

    int ox1 = obs.x;
    int oy1 = obs.y;
    int ox2 = obs.x + obs.w;
    int oy2 = obs.y + obs.h;

    return (dx1 < ox2 && dx2 > ox1 && dy1 < oy2 && dy2 > oy1);
}

static void dino_game_reset() {
    score = 0;
    spawn_timer_cnt = 0;
    update_score_text();

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active && obstacles[i].obj) {
            lv_obj_del(obstacles[i].obj);
        }
        obstacles[i].active = false;
    }

    dino_y = ground_y - dino_h;
    dino_velocity_y = 0;
    is_jumping = false;
    lv_obj_align(dino_obj, LV_ALIGN_TOP_LEFT, dino_start_x, dino_y);

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
        dino_velocity_y = jump_strength;
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
        }
    }
}

static void dino_game_update(lv_timer_t * t) {
    if (game_state != GameState::PLAYING) return;

    // Physics
    if (is_jumping) {
        dino_y += dino_velocity_y;
        dino_velocity_y += gravity;

        if (dino_y >= ground_y - dino_h) {
            dino_y = ground_y - dino_h;
            is_jumping = false;
            dino_velocity_y = 0;
        }
        lv_obj_align(dino_obj, LV_ALIGN_TOP_LEFT, dino_start_x, dino_y);
    }

    // Obstacle Spawn
    spawn_timer_cnt++;
    if (spawn_timer_cnt >= spawn_interval) {
        spawn_timer_cnt = 0;
        spawn_interval = 40 + (std::rand() % 40); // Randomize interval
        spawn_obstacle();
    }

    // Move & Collide
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active) {
            obstacles[i].x -= obstacle_speed;
            lv_obj_align(obstacles[i].obj, LV_ALIGN_TOP_LEFT, obstacles[i].x, obstacles[i].y);

            if (check_collision(obstacles[i])) {
                game_over();
                return;
            }

            if (!obstacles[i].passed && (obstacles[i].x + obstacles[i].w < dino_start_x)) {
                obstacles[i].passed = true;
                score += 10;
                if (score % 100 == 0 && obstacle_speed < screen_w / 20) {
                    obstacle_speed++; // Increase difficulty
                }
                update_score_text();
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
 * It will clear the screen and launch the single page game.
 */
void create_lvgl_dino_game() {
    std::srand(std::time(nullptr));

    main_screen = lv_obj_create(NULL); // Create new screen
    screen_w = 320; // LVGL typical calc displays, overwrite these if known
    screen_h = 240;
    
    // Better to query actual resolution if available:
    if (lv_disp_get_default() != NULL) {
        screen_w = lv_disp_get_hor_res(lv_disp_get_default());
        screen_h = lv_disp_get_ver_res(lv_disp_get_default());
    }

    // Calculate dimensions dynamically
    ground_y = screen_h - (screen_h / 6);
    dino_w = screen_w / 15;
    if(dino_w < 15) dino_w = 15;
    dino_h = screen_h / 8;
    if(dino_h < 20) dino_h = 20;

    dino_start_x = screen_w / 8;
    jump_strength = - (screen_h / 14);
    gravity = screen_h / 100;
    if(gravity < 1) gravity = 1;
    obstacle_speed = screen_w / 50;
    if(obstacle_speed < 2) obstacle_speed = 2;


    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0xFFFFFF), 0);
    lv_obj_add_event_cb(main_screen, dino_event_cb, LV_EVENT_ALL, NULL);

    // Ground line
    ground_obj = lv_obj_create(main_screen);
    lv_obj_set_size(ground_obj, screen_w, 2);
    lv_obj_align(ground_obj, LV_ALIGN_TOP_LEFT, 0, ground_y);
    lv_obj_set_style_bg_color(ground_obj, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(ground_obj, 0, 0);
    lv_obj_set_style_radius(ground_obj, 0, 0);

    // Dino (simple dark gray block representing dino)
    dino_obj = lv_obj_create(main_screen);
    lv_obj_set_size(dino_obj, dino_w, dino_h);
    lv_obj_set_style_bg_color(dino_obj, lv_color_hex(0x535353), 0);
    lv_obj_set_style_radius(dino_obj, 4, 0); // slight rounding
    lv_obj_set_style_border_width(dino_obj, 0, 0);
    
    dino_y = ground_y - dino_h;
    lv_obj_align(dino_obj, LV_ALIGN_TOP_LEFT, dino_start_x, dino_y);

    // Text labels
    score_label = lv_label_create(main_screen);
    lv_obj_align(score_label, LV_ALIGN_TOP_RIGHT, -10, 10);
    update_score_text();

    start_msg_label = lv_label_create(main_screen);
    lv_label_set_text(start_msg_label, "Press/Tap to Start");
    lv_obj_align(start_msg_label, LV_ALIGN_CENTER, 0, -20);

    game_over_msg_label = lv_label_create(main_screen);
    lv_label_set_text(game_over_msg_label, "GAME OVER");
    lv_obj_add_flag(game_over_msg_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(game_over_msg_label, LV_ALIGN_CENTER, 0, -20);

    // Focus screen for keypad/button inputs
    lv_group_t * g = lv_group_get_default();
    if(g != NULL) {
        lv_group_add_obj(g, main_screen);
        lv_group_focus_obj(main_screen);
    }

    // Load the game screen
    lv_scr_load(main_screen);

    // Create a 30fps timer
    if (game_timer == NULL) {
        game_timer = lv_timer_create(dino_game_update, 33, NULL);
    }

    game_state = GameState::START;
}
