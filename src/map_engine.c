#include "../include/zelda_game.h"
#include "../include/zelda_ui.h"

extern void trigger_bokoblin_encounter(HeroState *hero);

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
    if (roll < 5) {
        hero->potion_inventory++;
        print_dialog_text("보물상자", "포션 +1 획득.");
    } else if (roll < 11) {
        trigger_bokoblin_encounter(hero);
    }
}

int process_hero_movement(HeroState *hero, int key_input) {
    struct timeval now;
    gettimeofday(&now, NULL);

    long diff_ms = (now.tv_sec - g_last_time.tv_sec) * 1000 + (now.tv_usec - g_last_time.tv_usec) / 1000;
    int step = 1;

    if (hero->token_revali && (key_input == g_last_key) && (diff_ms < 350)) {
        step = 2;
    }

    g_last_key = key_input;
    g_last_time = now;

    int nx = hero->pos_x;
    int ny = hero->pos_y;
    int moved = 0;

    if (key_input == 'w' || key_input == 'W' || key_input == KEY_UP) ny -= step;
    else if (key_input == 's' || key_input == 'S' || key_input == KEY_DOWN) ny += step;
    else if (key_input == 'a' || key_input == 'A' || key_input == KEY_LEFT) nx -= step;
    else if (key_input == 'd' || key_input == 'D' || key_input == KEY_RIGHT) nx += step;
    else return 0;

    if (is_valid_tile(ny, nx)) {
        if (nx != hero->pos_x || ny != hero->pos_y) {
            hero->pos_x = nx;
            hero->pos_y = ny;
            moved = 1;

            int is_special_tile =
                (ny == 0 && nx == 7) ||
                (ny == 5 && nx == 5) ||
                (ny == 0 && nx == 0) ||
                (ny == 10 && nx == 0) ||
                (ny == 10 && nx == 10) ||
                (ny == 0 && nx == 10);

            if (!is_special_tile) {
                trigger_random_field_event(hero);
            }
        }
    }

    return moved;
}
