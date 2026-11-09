#include "../include/zelda_game.h"
#include "../include/zelda_ui.h"

static BossMonster g_bosses[4] = {
    {"사막의 전기 커스 가논 ⚡", 140, 140, 20, "⚡"},
    {"조라의 물 커스 가논 💧", 190, 190, 24, "💧"},
    {"데스마운틴 불 커스 가논 🔥", 260, 260, 30, "🔥"},
    {"리토의 바람 커스 가논 🌪️", 340, 340, 38, "🌪️"}
};

static BossMonster g_ganon = {"재앙 가논 👾", 550, 550, 50, "👾"};

static int execute_qte_typing(const char *target_word, int timeout_sec) {
    int max_y, max_x; getmaxyx(stdscr, max_y, max_x);
    WINDOW *win_qte = newwin(7, 56, (max_y - 7) / 2, (max_x - 56) / 2);
    nodelay(win_qte, TRUE); keypad(win_qte, TRUE);
    char input_buf[32] = {0}; int input_len = 0, qte_success = 0;
    struct timeval start_tv, current_tv; gettimeofday(&start_tv, NULL);

    while (1) {
        gettimeofday(&current_tv, NULL);
        double remain = timeout_sec - ((current_tv.tv_sec - start_tv.tv_sec) + (current_tv.tv_usec - start_tv.tv_usec) / 1000000.0);
        if (remain <= 0.0) break;

        wclear(win_qte); box(win_qte, 0, 0);
        mvwprintw(win_qte, 1, 2, "⚡ [3초 제한 QTE!] 단어: [%s]", target_word);
        mvwprintw(win_qte, 3, 2, "⏱️ 남은 시간: %.2f 초", remain);
        mvwprintw(win_qte, 5, 2, "➤ 입력: %s_", input_buf);
        wrefresh(win_qte);

        fd_set fds; FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
        struct timeval tv = {0, 50000};
        if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
            int ch = wgetch(win_qte);
            if (ch == '\n' || ch == '\r') {
                if (strcmp(input_buf, target_word) == 0) qte_success = 1;
                break;
            } else if ((ch == KEY_BACKSPACE || ch == 127) && input_len > 0) {
                input_buf[--input_len] = '\0';
            } else if (ch >= 32 && ch <= 126 && input_len < 30) {
                input_buf[input_len++] = (char)ch; input_buf[input_len] = '\0';
            }
        }
    }
    delwin(win_qte); return qte_success;
}

static void handle_permadeath(HeroState *hero) {
    clear(); box(stdscr, 0, 0);
    mvprintw(get_centered_y(3), get_centered_x("💀 용사가 전투 중 완전히 쓰러졌습니다..."), "💀 용사가 전투 중 완전히 쓰러졌습니다...");
    mvprintw(get_centered_y(3) + 2, get_centered_x("⚠️ 영구 사망(Permadeath)으로 세이브 데이터 삭제!"), "⚠️ 영구 사망(Permadeath)으로 세이브 데이터 삭제!");
    refresh(); getch();
    unlink("saves/zelda_slot1.bin");
}

void trigger_boss_encounter(HeroState *hero, int boss_idx) {
    if (hero->boss_slain[boss_idx]) return;
    BossMonster boss = g_bosses[boss_idx];
    int highlight = 0, mipha_used = 0;
    const char *actions[] = {"1. ⚔️ 일반 공격 (QTE 보너스)", "2. 🧪 포션 체력 회복"};

    while (boss.hp > 0 && hero->current_hp > 0) {
        clear(); box(stdscr, 0, 0);
        mvprintw(2, 8, "[ %s ] 체력: %d/%d", boss.name, boss.hp, boss.max_hp);
        mvprintw(4, 8, "[ 용사 ] HP: %d | 포션: %d | 콤보: %d", hero->current_hp, hero->potion_inventory, hero->combo_count);

        for (int i = 0; i < 2; i++) {
            if (i == highlight) { attron(A_BOLD | COLOR_PAIR(5)); mvprintw(7 + i, 12, "➤ %s", actions[i]); attroff(A_BOLD | COLOR_PAIR(5)); }
            else mvprintw(7 + i, 12, "  %s", actions[i]);
        }
        refresh();

        int ch = getch();
        if (ch == KEY_UP || ch == 'w') highlight = (highlight - 1 + 2) % 2;
        else if (ch == KEY_DOWN || ch == 's') highlight = (highlight + 1) % 2;
        else if (ch == '\n' || ch == ' ') {
            int p_done = 0;
            if (highlight == 0) {
                int base_dmg = hero->attack_power + (rand() % 6);
                print_dialog_text(hero, "시스템", "공격을 시도합니다 'attack'을 쳐서 치명타를 노리세요");
                if (execute_qte_typing("attack", 3)) {
                    hero->combo_count++;
                    boss.hp -= (base_dmg + (hero->combo_count * 8) + 12);
                    hero->total_score += 150 * hero->combo_count;
                    print_dialog_text(hero, "시스템", "QTE 성공 치명타가 들어갔습니다");
                } else {
                    hero->combo_count = 0; boss.hp -= base_dmg;
                    print_dialog_text(hero, "시스템", "QTE 실패 기본 공격만 들어갔습니다");
                }
                p_done = 1;
            } else if (highlight == 1) {
                if (hero->potion_inventory > 0) { hero->current_hp += 60; hero->potion_inventory--; p_done = 1; }
            }

            if (p_done && boss.hp > 0) {
                print_dialog_text(hero, "시스템", "보스가 반격합니다 'barrier'를 쳐서 방어하세요");
                if (execute_qte_typing("barrier", 3)) {
                    hero->combo_count++; hero->current_hp -= (boss.base_damage / 3);
                    print_dialog_text(hero, "시스템", "방어 성공 피해를 줄였습니다");
                } else {
                    hero->combo_count = 0; hero->current_hp -= boss.base_damage;
                    print_dialog_text(hero, "시스템", "방어 실패 큰 피해를 입었습니다");
                }
            }

            if (hero->current_hp <= 0 && hero->token_mipha && !mipha_used) {
                mipha_used = 1; hero->current_hp = hero->max_hp;
                print_dialog_text(hero, "미파", "미파의 기도로 체력을 전부 회복합니다");
            }
        }
    }

    if (hero->current_hp <= 0) handle_permadeath(hero);
    else {
        hero->boss_slain[boss_idx] = 1; hero->potion_inventory += 2; hero->total_score += 2000;
        print_dialog_text(hero, "시스템", "보스를 물리쳤습니다");
    }
}

void attempt_master_sword_trial(HeroState *hero) {
    int slain_count = 0;
    for (int i = 0; i < 4; i++) if (hero->boss_slain[i]) slain_count++;
    print_dialog_text(hero, "시스템", "마스터소드가 제단에 꽂혀 있습니다");

    if (slain_count >= 2 && hero->max_hp >= 150) {
        hero->has_master_sword = 1; hero->attack_power += 40;
        print_dialog_text(hero, "시스템", "자격을 증명하여 마스터소드를 뽑았습니다");
    } else {
        hero->current_hp -= 30;
        print_dialog_text(hero, "시스템", "자격이 부족하여 벼락을 맞았습니다");
        if (hero->current_hp <= 0) handle_permadeath(hero);
    }
}

void trigger_final_ganon_battle(HeroState *hero) {
    print_dialog_text(hero, "젤다 공주", "링크님 드디어 오셨군요 저주를 풀어주세요");
    print_dialog_text(hero, "재앙 가논", "하하하 하이랄을 멸망시키겠다");

    BossMonster ganon = g_ganon;
    int highlight = 0, mipha_used = 0;
    const char *actions[] = {"1. ⚔️ 일격 (QTE 보너스)", "2. 🧪 포션"};

    while (ganon.hp > 0 && hero->current_hp > 0) {
        clear(); box(stdscr, 0, 0);
        mvprintw(2, 8, "[ %s ] 체력: %d/%d", ganon.name, ganon.hp, ganon.max_hp);
        mvprintw(4, 8, "[ 용사 젤다 ] HP: %d | 콤보: %d", hero->current_hp, hero->combo_count);

        for (int i = 0; i < 2; i++) {
            if (i == highlight) { attron(A_BOLD | COLOR_PAIR(2)); mvprintw(7 + i, 12, "➤ %s", actions[i]); attroff(A_BOLD | COLOR_PAIR(2)); }
            else mvprintw(7 + i, 12, "  %s", actions[i]);
        }
        refresh();

        int ch = getch();
        if (ch == KEY_UP || ch == 'w') highlight = (highlight - 1 + 2) % 2;
        else if (ch == KEY_DOWN || ch == 's') highlight = (highlight + 1) % 2;
        else if (ch == '\n' || ch == ' ') {
            int p_done = 0;
            if (highlight == 0) {
                print_dialog_text(hero, "시스템", "가논에게 마스터소드 일격을 날립니다");
                if (execute_qte_typing("attack", 3)) {
                    hero->combo_count++;
                    ganon.hp -= ((hero->attack_power * 2) + (hero->combo_count * 15));
                    hero->total_score += 300 * hero->combo_count;
                    print_dialog_text(hero, "시스템", "마스터소드의 퇴마의 빛이 직격했습니다");
                } else {
                    hero->combo_count = 0; ganon.hp -= (hero->attack_power * 2);
                    print_dialog_text(hero, "시스템", "QTE 실패 기본 퇴마 데미지 적용");
                }
                p_done = 1;
            } else if (highlight == 1) {
                if (hero->potion_inventory > 0) { hero->current_hp += 70; hero->potion_inventory--; p_done = 1; }
            }

            if (p_done && ganon.hp > 0) {
                print_dialog_text(hero, "시스템", "가논이 반격합니다 방어하세요");
                if (execute_qte_typing("barrier", 3)) {
                    hero->combo_count++; hero->current_hp -= (ganon.base_damage / 4);
                    print_dialog_text(hero, "시스템", "방어 결계 성공");
                } else {
                    hero->combo_count = 0; hero->current_hp -= ganon.base_damage;
                    print_dialog_text(hero, "시스템", "방어 실패 엄청난 타격");
                }
            }
        }
    }

    if (hero->current_hp <= 0) handle_permadeath(hero);
    else {
        hero->ganon_defeated = 1;
        hero->total_score += 10000;
        print_dialog_text(hero, "젤다 공주", "링크 드디어 해냈군요 마왕이 쓰러지면서 저주가 풀렸어요");
        print_dialog_text(hero, "지혜의 선인", "오오 드디어 내 말도 편하게 다 나오는구만 고맙소 용사여");
    }
}
