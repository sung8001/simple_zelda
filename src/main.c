#include "../include/zelda_game.h"
#include "../include/zelda_ui.h"
#include "../include/zelda_net.h"

extern int process_hero_movement(HeroState *hero, int key_input);
extern void trigger_boss_encounter(HeroState *hero, int boss_idx);
extern void attempt_master_sword_trial(HeroState *hero);
extern void trigger_final_ganon_battle(HeroState *hero);
extern void execute_binary_save(const HeroState *hero, int slot_index);

static HeroState g_hero;
static char g_server_ip[64] = {0};
static int g_server_port = 0;

void handle_signal_interrupt(int sig) {
    (void)sig;
    shutdown_zelda_ui();
    printf("\n[System] SIGINT 감지! 비상 로그 기록 후 종료합니다.\n");
    execute_binary_save(&g_hero, 1);
    exit(0);
}

static int compare_local_rank(const void *a, const void *b) {
    const RankEntry *ra = (const RankEntry *)a;
    const RankEntry *rb = (const RankEntry *)b;
    if (rb->score != ra->score) return (rb->score > ra->score) ? 1 : -1;
    if (ra->clear_time_sec != rb->clear_time_sec) return (ra->clear_time_sec > rb->clear_time_sec) ? 1 : -1;
    return 0;
}

static void render_hall_of_fame_ui(const RankEntry *entries, int count, int total_records, int my_rank, const HeroState *hero, int is_clear) {
    clear();
    box(stdscr, 0, 0);
    attron(COLOR_PAIR(4) | A_BOLD);
    mvprintw(2, get_centered_x("🏆 하이랄 명예의 전당 (점수 높은 순 / 동점 시 클리어 시간 짧은 순) 🏆"),
             "🏆 하이랄 명예의 전당 (점수 높은 순 / 동점 시 클리어 시간 짧은 순) 🏆");
    attroff(COLOR_PAIR(4) | A_BOLD);

    mvprintw(4, 10, "순위     용사 이름            최종 점수        클리어 시간");
    mvprintw(5, 10, "──────────────────────────────────────────────────────────");

    int show_n = (count < MAX_RANK_ENTRIES) ? count : MAX_RANK_ENTRIES;
    for (int i = 0; i < show_n; i++) {
        mvprintw(6 + i, 10, " %2d위    %-16s   %7d 점      %7.2f 초",
                 i + 1, entries[i].name, entries[i].score, entries[i].clear_time_sec);
    }
    if (show_n == 0) {
        mvprintw(7, 10, "아직 등록된 클리어 기록이 없습니다.");
    }

    if (is_clear && hero != NULL) {
        attron(COLOR_PAIR(1) | A_BOLD);
        mvprintw(17, 10, "==========================================================");
        mvprintw(18, 10, "★ [ 내 클리어 기록 ] 순위: %d위 / 전체 %d명", my_rank, total_records);
        mvprintw(19, 10, "  용사: %s | 점수: %d 점 | 클리어 시간: %.2f 초 | 남은 HP: %d",
                 hero->hero_name, hero->total_score, hero->clear_time_sec, hero->current_hp);
        mvprintw(20, 10, "==========================================================");
        attroff(COLOR_PAIR(1) | A_BOLD);
    }

    mvprintw(22, 10, "▶ 아무 키나 누르면 메인 로비로 돌아갑니다...");
    refresh();
    getch();
}

static void record_and_show_local_hall_of_fame(const HeroState *hero, int is_clear) {
    mkdir("saves", 0755);
    RankEntry list[MAX_STORED_RECORDS];
    int count = 0;

    int fd = open("saves/local_hall_of_fame.bin", O_RDONLY);
    if (fd >= 0) {
        ssize_t bytes = read(fd, list, sizeof(list));
        if (bytes > 0) count = (int)(bytes / sizeof(RankEntry));
        close(fd);
    }

    int my_rank = -1;
    if (is_clear && hero != NULL) {
        RankEntry me;
        memset(&me, 0, sizeof(RankEntry));
        strncpy(me.name, hero->hero_name, MAX_NAME_LEN - 1);
        me.score = hero->total_score;
        me.clear_time_sec = hero->clear_time_sec;
        me.remaining_hp = hero->current_hp;

        if (count < MAX_STORED_RECORDS) list[count++] = me;
        else list[MAX_STORED_RECORDS - 1] = me;

        qsort(list, count, sizeof(RankEntry), compare_local_rank);

        fd = open("saves/local_hall_of_fame.bin", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
            write(fd, list, sizeof(RankEntry) * count);
            close(fd);
        }

        for (int i = 0; i < count; i++) {
            if (strcmp(list[i].name, me.name) == 0 &&
                list[i].score == me.score &&
                list[i].clear_time_sec == me.clear_time_sec) {
                my_rank = i + 1;
                break;
            }
        }
    }

    render_hall_of_fame_ui(list, count, count, my_rank, hero, is_clear);
}

static void show_hall_of_fame_screen(const HeroState *hero, int is_clear) {
    if (g_server_port <= 0) {
        record_and_show_local_hall_of_fame(hero, is_clear);
        return;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        record_and_show_local_hall_of_fame(hero, is_clear);
        return;
    }

    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(g_server_port);
    sa.sin_addr.s_addr = inet_addr(g_server_ip);

    if (connect(sock, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
        close(sock);
        record_and_show_local_hall_of_fame(hero, is_clear);
        return;
    }

    RankBoardPacket packet;
    if (is_clear && hero != NULL) {
        SubmitRecordPacket sub;
        memset(&sub, 0, sizeof(sub));
        sub.code = CMD_SAVE_RECORD;
        strncpy(sub.name, hero->hero_name, MAX_NAME_LEN - 1);
        sub.score = hero->total_score;
        sub.clear_time_sec = hero->clear_time_sec;
        sub.remaining_hp = hero->current_hp;
        send(sock, &sub, sizeof(sub), 0);
    } else {
        CommandCode code = CMD_REQ_RANK;
        send(sock, &code, sizeof(code), 0);
    }

    ssize_t rlen = recv(sock, &packet, sizeof(packet), 0);
    close(sock);

    if (rlen <= 0) {
        record_and_show_local_hall_of_fame(hero, is_clear);
        return;
    }

    render_hall_of_fame_ui(packet.entries, packet.entry_count, packet.total_records, packet.my_rank, hero, is_clear);
}

int main(int argc, char *argv[]) {
    srand((unsigned int)time(NULL));
    signal(SIGINT, handle_signal_interrupt);

    if (argc == 3) {
        strncpy(g_server_ip, argv[1], sizeof(g_server_ip) - 1);
        g_server_port = atoi(argv[2]);
    }

    init_zelda_ui();

    while (1) {
        int title_choice = show_title_screen();
        if (title_choice == 2) break;
        if (title_choice == 1) {
            show_how_to_play();
            continue;
        }

        memset(&g_hero, 0, sizeof(HeroState));
        input_hero_name(g_hero.hero_name, MAX_NAME_LEN);

        g_hero.pos_x = 5; g_hero.pos_y = 0;
        g_hero.current_hp = 100; g_hero.max_hp = 100;
        g_hero.attack_power = 25; g_hero.potion_inventory = 3;
        g_hero.move_range = 1;

        struct timeval game_start_tv, game_end_tv;
        gettimeofday(&game_start_tv, NULL);

        clear_full_screen();
        print_dialog_text("???", "\"용사...\"");
        print_dialog_text("???", "\"가논\"");
        print_dialog_text("???", "\"재앙\"");
        print_dialog_text("???", "\"처치\"");

        int game_running = 1;
        int key;

        while (game_running) {
            if (g_hero.current_hp <= 0) break;

            render_dashboard(&g_hero);

            if (g_hero.pos_y == 0 && g_hero.pos_x == 7 && !g_hero.has_master_sword) {
                attempt_master_sword_trial(&g_hero);
                if (g_hero.current_hp <= 0) break;
                continue;
            }

            if (g_hero.pos_x == 0 && g_hero.pos_y == 0 && !g_hero.boss_slain[0]) {
                trigger_boss_encounter(&g_hero, 0);
                continue;
            } else if (g_hero.pos_x == 0 && g_hero.pos_y == 10 && !g_hero.boss_slain[1]) {
                trigger_boss_encounter(&g_hero, 1);
                continue;
            } else if (g_hero.pos_x == 10 && g_hero.pos_y == 10 && !g_hero.boss_slain[2]) {
                trigger_boss_encounter(&g_hero, 2);
                continue;
            } else if (g_hero.pos_x == 10 && g_hero.pos_y == 0 && !g_hero.boss_slain[3]) {
                trigger_boss_encounter(&g_hero, 3);
                continue;
            }

            if (g_hero.pos_x == 5 && g_hero.pos_y == 5 && !g_hero.ganon_defeated) {
                trigger_final_ganon_battle(&g_hero);
                if (g_hero.ganon_defeated) {
                    gettimeofday(&game_end_tv, NULL);
                    g_hero.clear_time_sec = (game_end_tv.tv_sec - game_start_tv.tv_sec) +
                                            (game_end_tv.tv_usec - game_start_tv.tv_usec) / 1000000.0;
                    show_hall_of_fame_screen(&g_hero, 1);
                    game_running = 0;
                }
                continue;
            }

            if (g_hero.current_hp <= 0) break;

            key = getch();

            if (key == 27) {
                int menu_sel = show_esc_menu();
                if (menu_sel == 1) {
                    show_how_to_play();
                } else if (menu_sel == 2) {
                    game_running = 0;
                }
                continue;
            }

            if (key == 'e' || key == 'E') {
                show_inventory_menu(&g_hero);
                continue;
            }

            process_hero_movement(&g_hero, key);
        }
    }

    shutdown_zelda_ui();
    return 0;
}
