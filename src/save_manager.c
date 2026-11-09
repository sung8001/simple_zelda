#include "../include/zelda_game.h"
#include "../include/zelda_ui.h"

void execute_binary_save(const HeroState *hero, int slot_index) {
    char path[64];
    snprintf(path, sizeof(path), "saves/zelda_slot%d.bin", slot_index);

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return;

    write(fd, hero, sizeof(HeroState));
    close(fd);
}

int execute_binary_load(HeroState *hero, int slot_index) {
    char path[64];
    snprintf(path, sizeof(path), "saves/zelda_slot%d.bin", slot_index);

    int fd = open(path, O_RDONLY);
    if (fd < 0) return 0;

    read(fd, hero, sizeof(HeroState));
    close(fd);
    return 1;
}

int open_save_selection_menu(HeroState *hero) {
    clear(); box(stdscr, 0, 0);
    int y = get_centered_y(12);

    mvprintw(y, get_centered_x("🛡️ 세상에서 가장 간단한 젤다"), "🛡️ 세상에서 가장 간단한 젤다");
    mvprintw(y + 2, get_centered_x("세이브 슬롯 선택 (opendir / stat 시스템 콜 구현)"), "세이브 슬롯 선택 (opendir / stat 시스템 콜 구현)");

    DIR *dir = opendir("saves");
    if (!dir) {
        mkdir("saves", 0755);
    } else {
        closedir(dir);
    }

    mvprintw(y + 5, 12, "[0] ✨ 새로운 용사의 모험 시작");

    for (int i = 1; i <= 3; i++) {
        char path[64];
        struct stat st;
        snprintf(path, sizeof(path), "saves/zelda_slot%d.bin", i);

        if (stat(path, &st) == 0) {
            mvprintw(y + 5 + i, 12, "[%d] 📁 세이브 데이터 %d (파일 크기: %ld bytes)", i, i, st.st_size);
        } else {
            mvprintw(y + 5 + i, 12, "[%d] ⬜ 슬롯 비어있음", i);
        }
    }

    mvprintw(y + 10, 12, "선택 입력 (0~3): ");
    refresh();

    int ch = getch() - '0';
    if (ch >= 1 && ch <= 3) {
        if (execute_binary_load(hero, ch)) return 1;
    }

    strcpy(hero->hero_name, "Link");
    hero->pos_x = 0; hero->pos_y = 0;
    hero->current_hp = 100; hero->max_hp = 100;
    hero->attack_power = 25; hero->potion_inventory = 3;
    hero->move_range = 1; hero->total_turns = 0;
    memset(hero->boss_slain, 0, sizeof(hero->boss_slain));
    return 0;
}
