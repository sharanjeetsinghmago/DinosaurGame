#ifndef DINO_GAME_HPP
#define DINO_GAME_HPP

#include "lvgl.h" // Assuming lvgl is in the include path
#include <vector>

enum class GameState {
    START_SCREEN,
    PLAYING,
    GAME_OVER
};

struct Obstacle {
    lv_obj_t* obj;
    int x;
    int y;
    int width;
    int height;
    bool passed;
};

class DinoGame {
public:
    DinoGame(lv_obj_t* parent_screen);
    ~DinoGame();

    void start();
    void jump();
    void update(); // Called by an LVGL timer

private:
    void reset();
    void spawn_obstacle();
    bool check_collision(const Obstacle& obs);
    void game_over();
    void update_score_label();

    lv_obj_t* main_screen;

    // UI Elements
    lv_obj_t* dino_obj;
    lv_obj_t* ground_obj;
    lv_obj_t* score_label;
    lv_obj_t* start_msg_label;
    lv_obj_t* game_over_msg_label;

    // Game Timer
    lv_timer_t* game_timer;

    // Game State
    GameState current_state;
    std::vector<Obstacle> obstacles;
    
    // Physics & Metrics
    int score;
    int high_score;
    int dino_y; // Actual Y position
    int dino_velocity_y;
    bool is_jumping;

    // Constants (adjustable based on screen size)
    int screen_width;
    int screen_height;
    int ground_y;
    int dino_width;
    int dino_height;
    int dino_start_x;
    int jump_strength; // Initial negative velocity for jump
    int gravity;
    int obstacle_speed;
    int spawn_timer;
    int spawn_interval;
};

#endif // DINO_GAME_HPP
