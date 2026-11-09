#include "../include/zelda_game.h"
#include "../include/zelda_ui.h"

static int g_last_key = 0;
static struct timeval g_last_time;

static int is_valid_tile(int r, int c) {
    if (r < 0 || r >= BOARD_DIM || c < 0 || c >= BOARD_DIM) return 0;
    if (r <= 2 && c <= 2) return 1;
    if (r <= 2 && c >= 4 && c <= 6) return 1;
    if (r <= 2 && c >= 7 && c <= 10) return 1;
    if (r >= 8 && r <= 10 && c <= 2) return 1;
    if (r >= 8 && r <= 10 && c >= 8 && c <= 10) return 1;
    if (r >= 4 && r <= 6 && c >= 4 && c <= 6) return 1;
    if (r == 3 && c == 5) return 1;
    if (r == 0 && c == 3) return 1;
    if (c == 0 && r >= 3 && r <= 7) return 1;
    if (r == 10 && c >= 3 && c <= 7) return 1;
    if (c == 10 && r >= 3 && r <= 7) return 1;
    return 0;
}

static void trigger_random_field_event(HeroState *hero) {
    int roll = rand() % 100;
    if (roll < 10) {
        hero->potion_inventory++;
        print_dialog_text(hero, "이벤트", "길목에서 비밀 상자를 찾아 회복 포션 1개를 획득했습니다!");
    } else if (roll >= 10 && roll < 18) {
        print_dialog_text(hero, "이벤트", "야생의 보코블린이 튀어나왔습니다!");
        if (rand() % 2 == 0) {
            hero->current_hp -= 15;
            print_dialog_text(hero, "시스템", "기습을 당해 HP가 감소했습니다.");
        } else {
            hero->total_score += 300;
            print_dialog_text(hero, "시스템", "반격하여 물리치고 점수를 획득했습니다!");
        }
    }
}

int process_hero_movement(HeroState *hero, int key_input) {
    struct timeval now; gettimeofday(&now, NULL);
    long diff_ms = (now.tv_sec - g_last_time.tv_sec) * 1000 + (now.tv_usec - g_last_time.tv_usec) / 1000;
    int step = 1;
    if (hero->token_revali && (key_input == g_last_key) && (diff_ms < 350)) step = 2;
    g_last_key = key_input; g_last_time = now;

    int nx = hero->pos_x, ny = hero->pos_y, moved = 0;
    if (key_input == 'w' || key_input == 'W') ny -= step;
    else if (key_input == 's' || key_input == 'S') ny += step;
    else if (key_input == 'a' || key_input == 'A') nx -= step;
    else if (key_input == 'd' || key_input == 'D') nx += step;
    else return 0;

    if (is_valid_tile(ny, nx)) {
        if (nx != hero->pos_x || ny != hero->pos_y) {
            hero->pos_x = nx; hero->pos_y = ny; moved = 1;
            if (!(ny == 0 && nx == 7) && !(ny == 5 && nx == 5)) trigger_random_field_event(hero);
        }
    }
    return moved;
}
