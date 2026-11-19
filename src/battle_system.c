#include "../include/zelda_game.h"
#include "../include/zelda_ui.h"

static BossMonster g_bosses[4] = {
    {"전기 커스 가논 ⚡", 140, 140, 20, "⚡"},
    {"물 커스 가논 💧", 190, 190, 24, "💧"},
    {"불 커스 가논 🔥", 260, 260, 30, "🔥"},
    {"바람 커스 가논 🌪️", 340, 340, 38, "🌪️"}
};

static const char *g_attack_words[] = {
    "slash", "strike", "thrust", "smash", "blade", "charge", "critical", "triforce"
};
static const char *g_defense_words[] = {
    "shield", "guard", "block", "dodge", "barrier", "defend", "evade", "reflect"
};

static const char *g_ganon_p2_attack_words[] = {
    "excalibur", "annihilate", "catastrophe", "obliterate",
    "retribution", "vanquish", "judgment", "sacredlight"
};
static const char *g_ganon_p2_defense_words[] = {
    "invincible", "impenetrable", "sanctuary", "counteract",
    "absolution", "fortress", "aegisguard", "redemption"
};

#define NUM_WORDS 8

static const char *get_random_attack_word(int hard_mode) {
    if (hard_mode) return g_ganon_p2_attack_words[rand() % NUM_WORDS];
    return g_attack_words[rand() % NUM_WORDS];
}

static const char *get_random_defense_word(int hard_mode) {
    if (hard_mode) return g_ganon_p2_defense_words[rand() % NUM_WORDS];
    return g_defense_words[rand() % NUM_WORDS];
}

static int execute_qte_typing(const char *phase_label, const char *target_word, double timeout_sec) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    WINDOW *win_qte = newwin(6, 52, (max_y - 6) / 2, (max_x - 52) / 2);
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
        mvwprintw(win_qte, 1, 2, "%s : [ %s ]", phase_label, target_word);
        mvwprintw(win_qte, 2, 2, "남은 시간: %.2f초", remain);
        mvwprintw(win_qte, 4, 2, "> %s_", input_buf);
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
    mvprintw(get_centered_y(2), get_centered_x("사망했습니다. 게임 오버."), "사망했습니다. 게임 오버.");
    mvprintw(get_centered_y(2) + 2, get_centered_x("(아무 키나 누르면 로비로)"), "(아무 키나 누르면 로비로)");
    refresh(); getch();

    unlink("saves/zelda_slot1.bin");
}

static void render_battle_center(const BossMonster *boss, const HeroState *hero, int is_player_turn, int highlight, const char *actions[]) {
    clear();
    box(stdscr, 0, 0);

    char boss_line[128];
    char hero_line[128];
    snprintf(boss_line, sizeof(boss_line), "[ %s ]  HP: %d / %d", boss->name, boss->hp, boss->max_hp);
    snprintf(hero_line, sizeof(hero_line), "[ %s ]  HP: %d / %d  |  포션: %d  |  콤보: %d",
             hero->hero_name, hero->current_hp, hero->max_hp, hero->potion_inventory, hero->combo_count);

    const char *divider = "──────────────────────────────────────────────────";
    int start_y = get_centered_y(9);
    if (start_y < 2) start_y = 2;

    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(start_y, get_centered_x(boss_line), "%s", boss_line);
    attroff(A_BOLD | COLOR_PAIR(4));

    mvprintw(start_y + 1, get_centered_x(divider), "%s", divider);

    mvprintw(start_y + 2, get_centered_x(hero_line), "%s", hero_line);

    if (is_player_turn) {
        for (int i = 0; i < 2; i++) {
            if (i == highlight) {
                char opt_buf[64];
                snprintf(opt_buf, sizeof(opt_buf), "▶ %s", actions[i]);
                attron(A_BOLD | COLOR_PAIR(6));
                mvprintw(start_y + 4 + i, get_centered_x(opt_buf), "%s", opt_buf);
                attroff(A_BOLD | COLOR_PAIR(6));
            } else {
                attron(COLOR_PAIR(6));
                mvprintw(start_y + 4 + i, get_centered_x(actions[i]), "%s", actions[i]);
                attroff(COLOR_PAIR(6));
            }
        }
    } else {
        const char *turn_label = "▶ 적 턴";
        attron(A_BOLD | COLOR_PAIR(2));
        mvprintw(start_y + 5, get_centered_x(turn_label), "%s", turn_label);
        attroff(A_BOLD | COLOR_PAIR(2));
    }

    refresh();
}

void trigger_boss_encounter(HeroState *hero, int boss_idx) {
    if (hero->boss_slain[boss_idx]) return;

    BossMonster boss = g_bosses[boss_idx];
    int highlight = 0;
    int mipha_used_in_battle = 0;
    int is_player_turn = 1;
    int battle_max_combo = 0;
    const char *actions[] = {"공격", "포션"};

    while (boss.hp > 0 && hero->current_hp > 0) {
        render_battle_center(&boss, hero, is_player_turn, highlight, actions);

        if (is_player_turn) {
            int ch = getch();
            if (ch == KEY_UP || ch == 'w' || ch == 'W') highlight = (highlight - 1 + 2) % 2;
            else if (ch == KEY_DOWN || ch == 's' || ch == 'S') highlight = (highlight + 1) % 2;
            else if (ch == '\n' || ch == ' ') {
                if (highlight == 0) {
                    const char *cmd = get_random_attack_word(0);
                    int q_res = execute_qte_typing("공격", cmd, 3.0);
                    if (q_res) {
                        hero->combo_count++;
                        if (hero->combo_count > battle_max_combo) battle_max_combo = hero->combo_count;
                        if (hero->combo_count > hero->max_combo) hero->max_combo = hero->combo_count;
                        int dmg = hero->attack_power + (hero->combo_count * 5) + (rand() % 10);
                        boss.hp -= dmg;
                        print_dialog_text("공격", "성공! 적중.");
                    } else {
                        hero->combo_count = 0;
                        print_dialog_text("공격", "실패! 빗나감.");
                    }
                    is_player_turn = 0;
                } else if (highlight == 1) {
                    if (hero->potion_inventory > 0) {
                        hero->current_hp = (hero->current_hp + 60 > hero->max_hp) ? hero->max_hp : hero->current_hp + 60;
                        hero->potion_inventory--;
                        print_dialog_text("포션", "HP +60 회복.");
                        is_player_turn = 0;
                    } else {
                        print_dialog_text("포션", "포션 없음.");
                    }
                }
            }
        } else {
            const char *def_cmd = get_random_defense_word(0);
            int q_res = execute_qte_typing("방어", def_cmd, 3.0);
            if (q_res) {
                hero->combo_count++;
                if (hero->combo_count > battle_max_combo) battle_max_combo = hero->combo_count;
                if (hero->combo_count > hero->max_combo) hero->max_combo = hero->combo_count;
                hero->current_hp -= (boss.base_damage / 3);
                print_dialog_text("방어", "성공! 피해 경감.");
            } else {
                hero->combo_count = 0;
                int boss_dmg = boss.base_damage + (rand() % 8);
                hero->current_hp -= boss_dmg;
                print_dialog_text("방어", "실패! 피격.");
            }

            if (hero->current_hp <= 0 && hero->token_mipha && !mipha_used_in_battle) {
                mipha_used_in_battle = 1;
                hero->current_hp = hero->max_hp;
                print_dialog_text("미파의 증표", "부활! HP 전체 회복.");
            }

            is_player_turn = 1;
        }
    }

    if (hero->current_hp <= 0) {
        handle_permadeath(hero);
    } else {
        hero->boss_slain[boss_idx] = 1;
        hero->potion_inventory += 2;
        int combo_bonus = battle_max_combo * 200;
        hero->total_score += 2000 + combo_bonus;

        char reward_msg[128];
        if (boss_idx == 0) {
            print_dialog_text("우르보사", "\"고생\"");
            print_dialog_text("우르보사", "\"내힘\"");
            print_dialog_text("우르보사", "\"화이팅\"");
            hero->token_urbosa = 1; hero->attack_power += 25;
            snprintf(reward_msg, sizeof(reward_msg), "우르보사의 증표 (공격력 +25) | +%d점", 2000 + combo_bonus);
            print_dialog_text("증표 획득", reward_msg);
        } else if (boss_idx == 1) {
            print_dialog_text("미파", "\"감사\"");
            print_dialog_text("미파", "\"치유\"");
            print_dialog_text("미파", "\"응원\"");
            hero->token_mipha = 1;
            snprintf(reward_msg, sizeof(reward_msg), "미파의 증표 (사망 시 1회 부활) | +%d점", 2000 + combo_bonus);
            print_dialog_text("증표 획득", reward_msg);
        } else if (boss_idx == 2) {
            print_dialog_text("다르케르", "\"대단\"");
            print_dialog_text("다르케르", "\"수호\"");
            print_dialog_text("다르케르", "\"가자\"");
            hero->token_daruk = 1; hero->max_hp += 50; hero->current_hp += 50;
            snprintf(reward_msg, sizeof(reward_msg), "다르케르의 증표 (최대 체력 +50) | +%d점", 2000 + combo_bonus);
            print_dialog_text("증표 획득", reward_msg);
        } else if (boss_idx == 3) {
            print_dialog_text("리발", "\"인정\"");
            print_dialog_text("리발", "\"질풍\"");
            print_dialog_text("리발", "\"비상\"");
            hero->token_revali = 1;
            snprintf(reward_msg, sizeof(reward_msg), "리발의 증표 (방향키 연타 시 대시) | +%d점", 2000 + combo_bonus);
            print_dialog_text("증표 획득", reward_msg);
        }
    }
}

void attempt_master_sword_trial(HeroState *hero) {
    if (hero->has_master_sword) return;

    int slain_count = 0;
    for (int i = 0; i < 4; i++) if (hero->boss_slain[i]) slain_count++;

    int try_pull = prompt_yes_no_dialog(
        "마스터소드",
        "뽑으시겠습니까? (조건: 보스 2처치, MaxHP 150)"
    );

    if (!try_pull) {
        hero->pos_y = 1;
        return;
    }

    if (slain_count >= 2 && hero->max_hp >= 150) {
        hero->has_master_sword = 1;
        hero->attack_power += 40;
        hero->total_score += 1500;
        print_dialog_text("마스터소드", "획득! (공격력 +40, +1500점)");
    } else {
        hero->current_hp -= 30;
        hero->pos_y = 1;
        print_dialog_text("마스터소드", "조건 미달! (HP -30)");
        if (hero->current_hp <= 0) handle_permadeath(hero);
    }
}

void trigger_final_ganon_battle(HeroState *hero) {
    int enter_castle = prompt_yes_no_dialog(
        "재앙 가논",
        "최종 결전에 돌입하시겠습니까?"
    );

    if (!enter_castle) {
        hero->pos_y = 6;
        return;
    }

    int phase = 1;
    BossMonster ganon = {"👾 재앙 가논 [1페이즈]", 260, 260, 32, "👾"};

    int highlight = 0;
    int mipha_used_in_battle = 0;
    int is_player_turn = 1;
    int battle_max_combo = 0;
    const char *actions[] = {"공격", "포션"};

    while (hero->current_hp > 0) {
        if (ganon.hp <= 0 && phase == 1) {
            phase = 2;
            ganon = (BossMonster){"🔥 마수 가논 [2페이즈]", 360, 360, 45, "🔥"};
            is_player_turn = 1;
            print_dialog_text("재앙 가논", "2페이즈 돌입! (고난도 단어 출제)");
            continue;
        }

        if (ganon.hp <= 0 && phase == 2) {
            break;
        }

        render_battle_center(&ganon, hero, is_player_turn, highlight, actions);

        if (is_player_turn) {
            int ch = getch();
            if (ch == KEY_UP || ch == 'w' || ch == 'W') highlight = (highlight - 1 + 2) % 2;
            else if (ch == KEY_DOWN || ch == 's' || ch == 'S') highlight = (highlight + 1) % 2;
            else if (ch == '\n' || ch == ' ') {
                if (highlight == 0) {
                    int hard = (phase == 2) ? 1 : 0;
                    double limit = (phase == 2) ? 3.5 : 3.0;
                    const char *cmd = get_random_attack_word(hard);
                    int q_res = execute_qte_typing("공격", cmd, limit);
                    if (q_res) {
                        hero->combo_count++;
                        if (hero->combo_count > battle_max_combo) battle_max_combo = hero->combo_count;
                        if (hero->combo_count > hero->max_combo) hero->max_combo = hero->combo_count;
                        int dmg = (int)(hero->attack_power * 1.8) + (hero->combo_count * 10);
                        ganon.hp -= dmg;
                        print_dialog_text("공격", "성공! 적중.");
                    } else {
                        hero->combo_count = 0;
                        print_dialog_text("공격", "실패! 빗나감.");
                    }
                    is_player_turn = 0;
                } else if (highlight == 1) {
                    if (hero->potion_inventory > 0) {
                        hero->current_hp = (hero->current_hp + 70 > hero->max_hp) ? hero->max_hp : hero->current_hp + 70;
                        hero->potion_inventory--;
                        print_dialog_text("포션", "HP +70 회복.");
                        is_player_turn = 0;
                    } else {
                        print_dialog_text("포션", "포션 없음.");
                    }
                }
            }
        } else {
            int hard = (phase == 2) ? 1 : 0;
            double limit = (phase == 2) ? 3.5 : 3.0;
            const char *def_cmd = get_random_defense_word(hard);
            int q_res = execute_qte_typing("방어", def_cmd, limit);
            if (q_res) {
                hero->combo_count++;
                if (hero->combo_count > battle_max_combo) battle_max_combo = hero->combo_count;
                if (hero->combo_count > hero->max_combo) hero->max_combo = hero->combo_count;
                hero->current_hp -= (ganon.base_damage / 4);
                print_dialog_text("방어", "성공! 피해 경감.");
            } else {
                hero->combo_count = 0;
                hero->current_hp -= ganon.base_damage;
                print_dialog_text("방어", "실패! 피격.");
            }

            if (hero->current_hp <= 0 && hero->token_mipha && !mipha_used_in_battle) {
                mipha_used_in_battle = 1;
                hero->current_hp = hero->max_hp;
                print_dialog_text("미파의 증표", "부활! HP 전체 회복.");
            }

            is_player_turn = 1;
        }
    }

    if (hero->current_hp <= 0) {
        handle_permadeath(hero);
    } else {
        hero->ganon_defeated = 1;
        int combo_bonus = battle_max_combo * 300;
        int total_clear_pts = 5000 + combo_bonus;
        hero->total_score += total_clear_pts;

        char clear_msg[128];
        snprintf(clear_msg, sizeof(clear_msg), "재앙 가논 처치 완료! (+5000점 + 콤보 보너스 %d점)", combo_bonus);
        print_dialog_text("클리어", clear_msg);
    }
}

// 필드 보코블린 단판 QTE 조우
void trigger_bokoblin_encounter(HeroState *hero) {
    print_dialog_text("보코블린", "기습! 제한시간 내 입력해 처치하세요.");

    const char *cmd = (rand() % 2 == 0) ? get_random_attack_word(0) : get_random_defense_word(0);
    int q_res = execute_qte_typing("기습", cmd, 3.0);

    if (q_res) {
        hero->total_score += 300;
        print_dialog_text("보코블린", "처치 성공! (+300점)");
    } else {
        hero->current_hp -= 15;
        print_dialog_text("보코블린", "실패! 피격 (HP -15)");
        if (hero->current_hp <= 0) {
            handle_permadeath(hero);
        }
    }
}
