#include "../include/zelda_game.h"
#include "../include/zelda_ui.h"

static BossMonster g_bosses[4] = {
    {"사막의 전기 커스 가논 ⚡", 140, 140, 20, "⚡"},
    {"조라의 물 커스 가논 💧", 190, 190, 24, "💧"},
    {"데스마운틴 불 커스 가논 🔥", 260, 260, 30, "🔥"},
    {"리토의 바람 커스 가논 🌪️", 340, 340, 38, "🌪️"}
};

static BossMonster g_ganon = {"👾 재앙 가논 (Calamity Ganon)", 550, 550, 50, "👾"};

// 랜덤 QTE 출제용 공격 / 방어 단어 풀
static const char *g_attack_words[] = {
    "slash", "strike", "thrust", "smash", "blade", "charge", "critical", "triforce"
};
static const char *g_defense_words[] = {
    "shield", "guard", "block", "dodge", "barrier", "defend", "evade", "reflect"
};

#define NUM_WORDS 8

static const char *get_random_attack_word(void) {
    return g_attack_words[rand() % NUM_WORDS];
}

static const char *get_random_defense_word(void) {
    return g_defense_words[rand() % NUM_WORDS];
}

// select() 시스템 콜을 활용한 3초 제한 비동기 문자열 타이핑 QTE
static int execute_qte_typing(const char *phase_label, const char *target_word, int timeout_sec) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    WINDOW *win_qte = newwin(7, 58, (max_y - 7) / 2, (max_x - 58) / 2);
    nodelay(win_qte, TRUE);
    keypad(win_qte, TRUE);

    char input_buf[32] = {0};
    int input_len = 0;
    
    struct timeval start_tv, current_tv;
    gettimeofday(&start_tv, NULL);

    int qte_success = 0;

    while (1) {
        gettimeofday(&current_tv, NULL);
        double elapsed = (current_tv.tv_sec - start_tv.tv_sec) + (current_tv.tv_usec - start_tv.tv_usec) / 1000000.0;
        double remain = timeout_sec - elapsed;

        if (remain <= 0.0) {
            qte_success = 0;
            break;
        }

        wclear(win_qte);
        box(win_qte, 0, 0);
        mvwprintw(win_qte, 1, 2, "%s | 입력 단어: [ %s ]", phase_label, target_word);
        mvwprintw(win_qte, 3, 2, "⏱️ 남은 시간: %.2f 초 (입력 후 Enter)", remain);
        mvwprintw(win_qte, 5, 2, "> 입력: %s_", input_buf);
        wrefresh(win_qte);

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        struct timeval tv = {0, 50000};

        int sel = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
        if (sel > 0 && FD_ISSET(STDIN_FILENO, &fds)) {
            int ch = wgetch(win_qte);
            if (ch == '\n' || ch == '\r') {
                if (strcmp(input_buf, target_word) == 0) {
                    qte_success = 1;
                    break;
                } else {
                    qte_success = 0;
                    break;
                }
            } else if ((ch == KEY_BACKSPACE || ch == 127 || ch == '\b') && input_len > 0) {
                input_buf[--input_len] = '\0';
            } else if (ch >= 32 && ch <= 126 && input_len < 30) {
                input_buf[input_len++] = (char)ch;
                input_buf[input_len] = '\0';
            }
        }
    }

    delwin(win_qte);
    return qte_success;
}

static void handle_permadeath(HeroState *hero) {
    (void)hero;
    clear(); box(stdscr, 0, 0);
    mvprintw(get_centered_y(3), get_centered_x("💀 용사가 전투 중 완전히 쓰러졌습니다..."), "💀 용사가 전투 중 완전히 쓰러졌습니다...");
    mvprintw(get_centered_y(3) + 2, get_centered_x("⚠️ 영구 사망(Permadeath) 시스템으로 인해 세이브 데이터가 삭제됩니다!"), "⚠️ 영구 사망(Permadeath) 시스템으로 인해 세이브 데이터가 삭제됩니다!");
    mvprintw(get_centered_y(3) + 4, get_centered_x("(아무 키나 누르면 로비로 돌아갑니다...)"), "(아무 키나 누르면 로비로 돌아갑니다...)");
    refresh(); getch();

    unlink("saves/zelda_slot1.bin");
}

void trigger_boss_encounter(HeroState *hero, int boss_idx) {
    if (hero->boss_slain[boss_idx]) return;

    BossMonster boss = g_bosses[boss_idx];
    int highlight = 0;
    int mipha_used_in_battle = 0;
    int is_player_turn = 1; // 1: 내 공격 턴, 0: 적 공격 턴
    const char *actions[] = {"1. ⚔️ QTE 검술 공격 (랜덤 커맨드)", "2. 🧪 포션 체력 회복"};

    while (boss.hp > 0 && hero->current_hp > 0) {
        clear();
        box(stdscr, 0, 0);
        int y = 2;

        mvprintw(y, get_centered_x("⚔️ 신수 보스전 (교대 턴제) ⚔️"), "⚔️ 신수 보스전 (교대 턴제) ⚔️");
        mvprintw(y + 2, 8, "[ %s ] 체력: %d/%d", boss.name, boss.hp, boss.max_hp);
        mvprintw(y + 4, 8, "[ 용사 %s ] HP: %d/%d | 🧪 포션: %d개 | 🔥 콤보: %d 스택", 
                 hero->hero_name, hero->current_hp, hero->max_hp, hero->potion_inventory, hero->combo_count);

        if (is_player_turn) {
            attron(A_BOLD | COLOR_PAIR(1));
            mvprintw(y + 6, 8, "▶ [ 내 공격 턴 ] 행동을 선택하세요!");
            attroff(A_BOLD | COLOR_PAIR(1));

            for (int i = 0; i < 2; i++) {
                if (i == highlight) {
                    attron(A_BOLD | COLOR_PAIR(5));
                    mvprintw(y + 8 + i, 12, "> %s", actions[i]);
                    attroff(A_BOLD | COLOR_PAIR(5));
                } else {
                    mvprintw(y + 8 + i, 12, "  %s", actions[i]);
                }
            }
            refresh();

            int ch = getch();
            if (ch == KEY_UP || ch == 'w' || ch == 'W') highlight = (highlight - 1 + 2) % 2;
            else if (ch == KEY_DOWN || ch == 's' || ch == 'S') highlight = (highlight + 1) % 2;
            else if (ch == '\n' || ch == ' ') {
                if (highlight == 0) { // 내 공격 턴 -> 공격 QTE
                    const char *cmd = get_random_attack_word();
                    int q_res = execute_qte_typing("⚔️ 공격 QTE!", cmd, 3);
                    if (q_res) {
                        hero->combo_count++;
                        int bonus_dmg = hero->combo_count * 5;
                        int dmg = hero->attack_power + bonus_dmg + (rand() % 10);
                        boss.hp -= dmg;
                        hero->total_score += 150 * hero->combo_count;
                        print_dialog_text("공격 성공 💥", "QTE 성공! 보스에게 강력한 일격을 가했습니다!");
                    } else {
                        hero->combo_count = 0;
                        print_dialog_text("공격 실패 ❌", "QTE 시간 초과 또는 오타! 공격이 빗나갔습니다.");
                    }
                    is_player_turn = 0; // 적 턴으로 전환
                } else if (highlight == 1) { // 포션 사용
                    if (hero->potion_inventory > 0) {
                        hero->current_hp = (hero->current_hp + 60 > hero->max_hp) ? hero->max_hp : hero->current_hp + 60;
                        hero->potion_inventory--;
                        print_dialog_text("아이템 🧪", "포션을 마셔 체력을 60 회복했습니다! (적 턴으로 넘어갑니다)");
                        is_player_turn = 0; // 적 턴으로 전환
                    } else {
                        print_dialog_text("아이템 ❌", "포션이 부족합니다! 다른 행동을 선택하세요.");
                    }
                }
            }
        } else {
            // 적의 공격 턴 -> 방어 QTE 발동
            attron(A_BOLD | COLOR_PAIR(2));
            mvprintw(y + 6, 8, "⚠️ [ 적의 공격 턴 ] 보스가 공격해옵니다! 방어 QTE를 준비하세요!");
            attroff(A_BOLD | COLOR_PAIR(2));
            refresh();

            print_dialog_text(boss.name, "크아아아! 내 공격을 받아내 보아라!");

            const char *def_cmd = get_random_defense_word();
            int q_res = execute_qte_typing("🛡️ 방어 QTE!", def_cmd, 3);
            if (q_res) {
                hero->combo_count++;
                int boss_dmg = (boss.base_damage / 3);
                hero->current_hp -= boss_dmg;
                print_dialog_text("방어 성공 🛡️", "방어 QTE 성공! 방패로 막아내 피해를 70% 경감했습니다!");
            } else {
                hero->combo_count = 0;
                int boss_dmg = boss.base_damage + (rand() % 8);
                hero->current_hp -= boss_dmg;
                print_dialog_text("방어 실패 💥", "방어 QTE 실패! 보스의 강력한 공격을 정통으로 맞았습니다!");
            }

            if (hero->current_hp <= 0 && hero->token_mipha && !mipha_used_in_battle) {
                mipha_used_in_battle = 1;
                hero->current_hp = hero->max_hp;
                print_dialog_text("미파의 기도 💧", "미파의 영혼이 가호하여 체력을 100% 회복시키고 부활합니다!");
            }

            is_player_turn = 1; // 다시 내 공격 턴으로 전환
        }
    }

    if (hero->current_hp <= 0) {
        handle_permadeath(hero);
    } else {
        hero->boss_slain[boss_idx] = 1;
        hero->potion_inventory += 2;
        hero->total_score += 2000;

        if (boss_idx == 0) {
            hero->token_urbosa = 1; hero->attack_power += 25;
            print_dialog_text("우르보사의 영혼 ⚡", "[우르보사의 분노] 획득! (공격력 +25 상승)");
        } else if (boss_idx == 1) {
            hero->token_mipha = 1;
            print_dialog_text("미파의 영혼 💧", "[미파의 기도] 획득! (사망 시 1회 풀부활)");
        } else if (boss_idx == 2) {
            hero->token_daruk = 1; hero->max_hp += 50; hero->current_hp += 50;
            print_dialog_text("다르케르의 영혼 🔥", "[다르케르의 수호] 획득! (최대 체력 +50)");
        } else if (boss_idx == 3) {
            hero->token_revali = 1;
            print_dialog_text("리발의 영혼 🌪️", "[리발의 용오름] 획득! (WASD 연타 2칸 대시)");
        }
    }
}

void attempt_master_sword_trial(HeroState *hero) {
    int slain_count = 0;
    for (int i = 0; i < 4; i++) if (hero->boss_slain[i]) slain_count++;

    print_dialog_text("마스터소드의 제단 🗡️", "전설의 마스터소드가 바위에 깊게 꽂혀있습니다.");

    if (slain_count >= 2 && hero->max_hp >= 150) {
        hero->has_master_sword = 1;
        hero->attack_power += 40;
        print_dialog_text("시련 통과 ✨", "자격을 증명했습니다! 전설의 마스터소드를 뽑았습니다! (기본 ATK +40 폭증!)");
    } else {
        hero->current_hp -= 30;
        print_dialog_text("시련 실패 ⚡", "아직 자격이 부족합니다! (필요: 보스 2 처치 & Max HP 150) 벼락을 맞아 HP가 30 차감됩니다.");
        if (hero->current_hp <= 0) handle_permadeath(hero);
    }
}

void trigger_final_ganon_battle(HeroState *hero) {
    print_dialog_text("젤다 공주 👸", "링크! 드디어 오셨군요! 봉인이 한계에 다다랐습니다!");
    print_dialog_text("재앙 가논 👾", "크아아아-!! 하이랄을 멸망시키겠다!");

    BossMonster ganon = g_ganon;
    int highlight = 0;
    int mipha_used_in_battle = 0;
    int is_player_turn = 1;
    const char *actions[] = {"1. ⚔️ QTE 퇴마 일격 (랜덤 커맨드)", "2. 🧪 포션 체력 회복"};

    while (ganon.hp > 0 && hero->current_hp > 0) {
        clear();
        box(stdscr, 0, 0);
        int y = 2;

        mvprintw(y, get_centered_x("👑 최종 결전: 재앙 가논 (교대 턴제) 👑"), "👑 최종 결전: 재앙 가논 (교대 턴제) 👑");
        mvprintw(y + 2, 8, "[ %s ] 체력: %d/%d", ganon.name, ganon.hp, ganon.max_hp);
        mvprintw(y + 4, 8, "[ 용사 %s & 젤다 ] HP: %d/%d | 🧪 포션: %d개 | 🔥 콤보: %d", 
                 hero->hero_name, hero->current_hp, hero->max_hp, hero->potion_inventory, hero->combo_count);

        if (is_player_turn) {
            attron(A_BOLD | COLOR_PAIR(1));
            mvprintw(y + 6, 8, "▶ [ 내 공격 턴 ] 행동을 선택하세요!");
            attroff(A_BOLD | COLOR_PAIR(1));

            for (int i = 0; i < 2; i++) {
                if (i == highlight) {
                    attron(A_BOLD | COLOR_PAIR(2));
                    mvprintw(y + 8 + i, 12, "> %s", actions[i]);
                    attroff(A_BOLD | COLOR_PAIR(2));
                } else {
                    mvprintw(y + 8 + i, 12, "  %s", actions[i]);
                }
            }
            refresh();

            int ch = getch();
            if (ch == KEY_UP || ch == 'w' || ch == 'W') highlight = (highlight - 1 + 2) % 2;
            else if (ch == KEY_DOWN || ch == 's' || ch == 'S') highlight = (highlight + 1) % 2;
            else if (ch == '\n' || ch == ' ') {
                if (highlight == 0) {
                    const char *cmd = get_random_attack_word();
                    int q_res = execute_qte_typing("⚔️ 퇴마 공격 QTE!", cmd, 3);
                    if (q_res) {
                        hero->combo_count++;
                        int dmg = (hero->attack_power * 2) + (hero->combo_count * 10);
                        ganon.hp -= dmg;
                        hero->total_score += 300 * hero->combo_count;
                        print_dialog_text("퇴마의 빛 ✨", "가논에게 강력한 일격을 꽂아 넣었습니다!");
                    } else {
                        hero->combo_count = 0;
                        print_dialog_text("공격 실패 ❌", "QTE 실패! 가논이 공격을 받아쳤습니다.");
                    }
                    is_player_turn = 0;
                } else if (highlight == 1) {
                    if (hero->potion_inventory > 0) {
                        hero->current_hp = (hero->current_hp + 70 > hero->max_hp) ? hero->max_hp : hero->current_hp + 70;
                        hero->potion_inventory--;
                        print_dialog_text("아이템 🧪", "포션을 마셔 체력을 70 회복했습니다!");
                        is_player_turn = 0;
                    } else {
                        print_dialog_text("아이템 ❌", "포션이 부족합니다!");
                    }
                }
            }
        } else {
            attron(A_BOLD | COLOR_PAIR(2));
            mvprintw(y + 6, 8, "⚠️ [ 적의 공격 턴 ] 재앙 가논이 원념을 발사합니다! 방어 QTE를 준비하세요!");
            attroff(A_BOLD | COLOR_PAIR(2));
            refresh();

            print_dialog_text("재앙 가논 👾", "사라져라, 하이랄의 용사여!!");

            const char *def_cmd = get_random_defense_word();
            int q_res = execute_qte_typing("🛡️ 결계 방어 QTE!", def_cmd, 3);
            if (q_res) {
                hero->combo_count++;
                hero->current_hp -= (ganon.base_damage / 4);
                print_dialog_text("방어 성공 🛡️", "가논의 흑마법을 결계로 막아냈습니다!");
            } else {
                hero->combo_count = 0;
                hero->current_hp -= ganon.base_damage;
                print_dialog_text("방어 실패 💥", "가논의 원념 공격에 직격 당했습니다!");
            }

            if (hero->current_hp <= 0 && hero->token_mipha && !mipha_used_in_battle) {
                mipha_used_in_battle = 1;
                hero->current_hp = hero->max_hp;
                print_dialog_text("미파의 기도 💧", "미파의 영혼이 체력을 완전 회복시킵니다!");
            }

            is_player_turn = 1;
        }
    }

    if (hero->current_hp <= 0) {
        handle_permadeath(hero);
    } else {
        hero->ganon_defeated = 1;
        hero->total_score += 10000;
        print_dialog_text("젤다 공주 👸", "링크! 해냈군요! 재앙 가논을 소멸시켰습니다!");
        print_dialog_text("지혜의 선인 🌟", "하이랄 대륙에 마침내 영원한 평화가 찾아왔도다!");
    }
}
