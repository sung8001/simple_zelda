#include "../include/zelda_game.h"
#include "../include/zelda_ui.h"
#include "../include/zelda_net.h"

extern int process_hero_movement(HeroState *hero, int key_input);
extern void trigger_boss_encounter(HeroState *hero, int boss_idx);
extern void attempt_master_sword_trial(HeroState *hero);
extern void trigger_final_ganon_battle(HeroState *hero);
extern void execute_binary_save(const HeroState *hero, int slot_index);
extern int open_save_selection_menu(HeroState *hero);

static HeroState g_hero;
static int g_network_sock = -1;

void handle_signal_interrupt(int sig) {
    shutdown_zelda_ui();
    printf("\n[System] SIGINT 인터럽트 감지! 슬롯 1에 비상 자동 저장 후 안전하게 종료합니다.\n");
    execute_binary_save(&g_hero, 1);
    exit(0);
}

void request_online_ranking(int sock) {
    if (sock < 0) return;
    CommandCode code = CMD_REQ_RANK;
    send(sock, &code, sizeof(code), 0);

    RankBoardPacket packet;
    recv(sock, &packet, sizeof(packet), 0);

    clear(); box(stdscr, 0, 0);
    mvprintw(2, get_centered_x("🏆 최단 클리어 명예의 전당 🏆"), "🏆 최단 클리어 명예의 전당 🏆");
    mvprintw(4, 12, "순위    용사 이름       최종 스코어");
    mvprintw(5, 12, "------------------------------------");

    for (int i = 0; i < packet.entry_count; i++) {
        mvprintw(6 + i, 12, " %d.     %-14s   %d 점", i + 1, packet.entries[i].name, packet.entries[i].clear_turns);
    }
    mvprintw(16, 12, "* 아무 키나 누르면 돌아갑니다...");
    refresh(); getch();
}

void submit_clear_record(int sock, const HeroState *hero) {
    if (sock < 0) return;
    SubmitRecordPacket packet;
    packet.code = CMD_SAVE_RECORD;
    strncpy(packet.name, hero->hero_name, MAX_NAME_LEN - 1);
    packet.clear_turns = hero->total_score;
    packet.remaining_hp = hero->current_hp;
    send(sock, &packet, sizeof(packet), 0);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_signal_interrupt);

    if (argc == 3) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in sa = {0};
        sa.sin_family = AF_INET;
        sa.sin_port = htons(atoi(argv[2]));
        sa.sin_addr.s_addr = inet_addr(argv[1]);
        if (connect(sock, (struct sockaddr*)&sa, sizeof(sa)) == 0) {
            g_network_sock = sock;
        }
    }

    init_zelda_ui();

    while (1) {
        int title_choice = show_title_screen();
        if (title_choice == 2) break;

        if (title_choice == 0) {
            strcpy(g_hero.hero_name, "Link");
            g_hero.pos_x = 5; g_hero.pos_y = 0;
            g_hero.current_hp = 100; g_hero.max_hp = 100;
            g_hero.attack_power = 25; g_hero.potion_inventory = 3;
            g_hero.move_range = 1; g_hero.total_turns = 0;
            g_hero.combo_count = 0; g_hero.total_score = 0;
            g_hero.has_master_sword = 0; g_hero.ganon_defeated = 0;
            g_hero.token_urbosa = 0; g_hero.token_mipha = 0;
            g_hero.token_daruk = 0; g_hero.token_revali = 0;
            memset(g_hero.boss_slain, 0, sizeof(g_hero.boss_slain));
        } else if (title_choice == 1) {
            open_save_selection_menu(&g_hero);
        }

        clear_full_screen();
        print_dialog_text("지혜의 선인 🌟", "시작의 초원에서 깨어났도다! 4대 신수를 구하고 마스터소드의 시련을 극복하게나.");

        int game_running = 1;
        int key;

        while (game_running) {
            if (g_hero.current_hp <= 0) break;

            render_dashboard(&g_hero);

            if (g_hero.pos_y == 0 && g_hero.pos_x == 7 && !g_hero.has_master_sword) {
                attempt_master_sword_trial(&g_hero);
            }

            if (g_hero.pos_x == 0 && g_hero.pos_y == 0) trigger_boss_encounter(&g_hero, 0);
            else if (g_hero.pos_x == 0 && g_hero.pos_y == 10) trigger_boss_encounter(&g_hero, 1);
            else if (g_hero.pos_x == 10 && g_hero.pos_y == 10) trigger_boss_encounter(&g_hero, 2);
            else if (g_hero.pos_x == 10 && g_hero.pos_y == 0) trigger_boss_encounter(&g_hero, 3);

            if (g_hero.pos_x == 5 && g_hero.pos_y == 5 && !g_hero.ganon_defeated) {
                trigger_final_ganon_battle(&g_hero);
                if (g_hero.ganon_defeated) {
                    if (g_network_sock >= 0) submit_clear_record(g_network_sock, &g_hero);
                    game_running = 0;
                }
            }

            if (g_hero.current_hp <= 0) break;

            key = getch();

            if (key == 27) {
                int menu_sel = show_esc_menu();
                if (menu_sel == 1) {
                    print_dialog_text("조작 안내 📖", "WASD: 이동 | E: 스탯/인벤토리 | S: 세이브 | R: 랭킹 | ESC: 메뉴");
                } else if (menu_sel == 2) {
                    execute_binary_save(&g_hero, 1);
                    print_dialog_text("시스템 💾", "슬롯 1에 성공적으로 저장되었습니다!");
                } else if (menu_sel == 3) {
                    game_running = 0;
                }
                continue;
            }

            if (key == 'e' || key == 'E') {
                show_inventory_menu(&g_hero);
                continue;
            }

            if (key == 's' || key == 'S') { execute_binary_save(&g_hero, 1); }
            if (key == 'r' || key == 'R') { request_online_ranking(g_network_sock); }

            process_hero_movement(&g_hero, key);
        }
    }

    shutdown_zelda_ui();
    if (g_network_sock >= 0) close(g_network_sock);
    return 0;
}
