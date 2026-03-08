#include "dino_game.hpp"
#include <cstdlib>
#include <ctime>

// Static callback for LVGL timer
static void game_timer_cb(lv_timer_t* timer) {
    if (timer && timer->user_data) {
        DinoGame* game = static_cast<DinoGame*>(timer->user_data);
        game->update();
    }
}

// Key event handler for jumping
static void game_event_handler(lv_event_t* e) {
    DinoGame* game = static_cast<DinoGame*>(lv_event_get_user_data(e));
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED || code == LV_EVENT_KEY) {
        if (code == LV_EVENT_KEY) {
            uint32_t key = lv_event_get_key(e);
            if (key == LV_KEY_UP || key == ' ') {
                game->jump();
            }
        } else {
            // Touch / Click anywhere
            game->jump();
        }
    }
}

DinoGame::DinoGame(lv_obj_t* parent_screen)
    : main_screen(parent_screen), current_state(GameState::START_SCREEN),
      score(0), high_score(0), is_jumping(false),
      screen_width(lv_obj_get_width(parent_screen)),
      screen_height(lv_obj_get_height(parent_screen)) {

    std::srand(std::time(nullptr));

    // Dynamic sizing based on screen dimensions
    ground_y = screen_height - 40;
    dino_width = screen_width / 15;
    dino_height = screen_height / 8;
    dino_start_x = screen_width / 8;
    
    jump_strength = - (screen_height / 15);
    gravity = screen_height / 120;
    if(gravity < 1) gravity = 1;
    
    obstacle_speed = screen_width / 60;
    spawn_interval = 60; // frames
    spawn_timer = 0;

    // Background style
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0xFFFFFF), 0);
    lv_obj_add_event_cb(main_screen, game_event_handler, LV_EVENT_ALL, this);

    // Ground
    ground_obj = lv_obj_create(main_screen);
    lv_obj_set_size(ground_obj, screen_width, 2);
    lv_obj_align(ground_obj, LV_ALIGN_TOP_LEFT, 0, ground_y);
    lv_obj_set_style_bg_color(ground_obj, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(ground_obj, 0, 0);

    // Dino
    dino_obj = lv_obj_create(main_screen);
    lv_obj_set_size(dino_obj, dino_width, dino_height);
    lv_obj_set_style_bg_color(dino_obj, lv_color_hex(0x535353), 0);
    lv_obj_set_style_radius(dino_obj, 5, 0);
    dino_y = ground_y - dino_height;
    lv_obj_align(dino_obj, LV_ALIGN_TOP_LEFT, dino_start_x, dino_y);

    // Score
    score_label = lv_label_create(main_screen);
    lv_obj_align(score_label, LV_ALIGN_TOP_RIGHT, -10, 10);
    update_score_label();

    // Start Screen
    start_msg_label = lv_label_create(main_screen);
    lv_label_set_text(start_msg_label, "Press/Tap to Start");
    lv_obj_align(start_msg_label, LV_ALIGN_CENTER, 0, -20);

    // Game Over Message
    game_over_msg_label = lv_label_create(main_screen);
    lv_label_set_text(game_over_msg_label, "GAME OVER");
    lv_obj_add_flag(game_over_msg_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(game_over_msg_label, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_text_color(game_over_msg_label, lv_color_hex(0xFF0000), 0);

    // Focus the screen to receive key inputs
    lv_group_t * g = lv_group_get_default();
    if(g != NULL) {
        lv_group_add_obj(g, main_screen);
        lv_group_focus_obj(main_screen);
    }

    // Timer (~30 fps)
    game_timer = lv_timer_create(game_timer_cb, 33, this);
}

DinoGame::~DinoGame() {
    lv_timer_del(game_timer);
    // Note: Children objects are automatically cleaned up when main_screen is deleted
}

void DinoGame::reset() {
    score = 0;
    spawn_timer = 0;
    update_score_label();
    
    for (auto& obs : obstacles) {
        lv_obj_del(obs.obj);
    }
    obstacles.clear();

    dino_y = ground_y - dino_height;
    dino_velocity_y = 0;
    is_jumping = false;
    lv_obj_align(dino_obj, LV_ALIGN_TOP_LEFT, dino_start_x, dino_y);

    lv_obj_add_flag(start_msg_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(game_over_msg_label, LV_OBJ_FLAG_HIDDEN);

    current_state = GameState::PLAYING;
}

void DinoGame::start() {
    if (current_state == GameState::START_SCREEN || current_state == GameState::GAME_OVER) {
        reset();
    }
}

void DinoGame::jump() {
    if (current_state == GameState::START_SCREEN || current_state == GameState::GAME_OVER) {
        start();
        return;
    }

    if (!is_jumping) {
        is_jumping = true;
        dino_velocity_y = jump_strength;
    }
}

void DinoGame::spawn_obstacle() {
    Obstacle obs;
    obs.width = dino_width / 2 + (std::rand() % (dino_width / 2));
    obs.height = dino_height / 2 + (std::rand() % dino_height);
    obs.x = screen_width;
    obs.y = ground_y - obs.height;
    obs.passed = false;

    obs.obj = lv_obj_create(main_screen);
    lv_obj_set_size(obs.obj, obs.width, obs.height);
    lv_obj_align(obs.obj, LV_ALIGN_TOP_LEFT, obs.x, obs.y);
    lv_obj_set_style_bg_color(obs.obj, lv_color_hex(0xFF0000), 0); // Red obstacles
    lv_obj_set_style_border_width(obs.obj, 0, 0);

    obstacles.push_back(obs);
}

bool DinoGame::check_collision(const Obstacle& obs) {
    // AABB Collision (Axis-Aligned Bounding Box)
    // Reduce box slightly to be forgiving
    int margin = 5; 
    
    // Dino box
    int dx1 = dino_start_x + margin;
    int dy1 = dino_y + margin;
    int dx2 = dino_start_x + dino_width - margin;
    int dy2 = dino_y + dino_height - margin;

    // Obs box
    int ox1 = obs.x;
    int oy1 = obs.y;
    int ox2 = obs.x + obs.width;
    int oy2 = obs.y + obs.height;

    // Check intersection
    return (dx1 < ox2 && dx2 > ox1 && dy1 < oy2 && dy2 > oy1);
}

void DinoGame::game_over() {
    current_state = GameState::GAME_OVER;
    lv_obj_clear_flag(game_over_msg_label, LV_OBJ_FLAG_HIDDEN);
    if (score > high_score) {
        high_score = score;
        update_score_label();
    }
}

void DinoGame::update_score_label() {
    lv_label_set_text_fmt(score_label, "HI: %05d  %05d", high_score, score);
}

void DinoGame::update() {
    if (current_state != GameState::PLAYING) return;

    // Update Physics
    if (is_jumping) {
        dino_y += dino_velocity_y;
        dino_velocity_y += gravity;

        if (dino_y >= ground_y - dino_height) {
            dino_y = ground_y - dino_height;
            is_jumping = false;
            dino_velocity_y = 0;
        }
        lv_obj_align(dino_obj, LV_ALIGN_TOP_LEFT, dino_start_x, dino_y);
    }

    // Spawn Obstacles
    spawn_timer++;
    if (spawn_timer >= spawn_interval) {
        spawn_timer = 0;
        // Randomize next spawn slightly
        spawn_interval = 40 + (std::rand() % 40); 
        spawn_obstacle();
    }

    // Update Obstacles
    for (auto it = obstacles.begin(); it != obstacles.end();) {
        it->x -= obstacle_speed;
        lv_obj_align(it->obj, LV_ALIGN_TOP_LEFT, it->x, it->y);

        if (check_collision(*it)) {
            game_over();
            return;
        }

        if (!it->passed && it->x + it->width < dino_start_x) {
            it->passed = true;
            score += 10;
            if (score % 100 == 0) {
                // Increase speed slightly
                obstacle_speed += 1;
            }
            update_score_label();
        }

        if (it->x + it->width < 0) {
            lv_obj_del(it->obj);
            it = obstacles.erase(it);
        } else {
            ++it;
        }
    }
}
