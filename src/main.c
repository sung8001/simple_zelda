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
    printf("\n[System] SIGINT 인터럽트 감지! 슬롯 1 비상 자동 저장 완료.\n");
    execute_binary_save(&g_hero, 1);
    exit(0);
}

void request_online_ranking(int sock) {
    // 랭킹 생략
}
void submit_clear_record(int sock, const HeroState *hero) {
    // 랭킹 생략
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_signal_interrupt);
    init_zelda_ui();

    while (1) {
        int title_choice = show_title_screen();
        if (title_choice == 2) break;

        if (title_choice == 0) {
            strcpy(g_hero.hero_name, "Link");
            g_hero.pos_x = 5; g_hero.pos_y = 0; 
            g_hero.current_hp = 100; g_hero.max_hp = 100;
            g_hero.attack_power = 25; g_hero.potion_inventory = 3;
            g_hero.has_master_sword = 0; g_hero.ganon_defeated = 0;
            memset(g_hero.boss_slain, 0, sizeof(g_hero.boss_slain));
        } else if (title_choice == 1) {
            open_save_selection_menu(&g_hero);
        }

        clear_full_screen();
        
        // ⚠️ 여기서 요청하신 스토리 연출이 나옵니다! 띄어쓰기 기준으로 스페이스바 대기
        print_dialog_text(&g_hero, "???", "가논이 저주를 걸었어 세글자 씩만 말할수 있어 젤다를 구해줘 부탁해");

        int game_running = 1;
        while (game_running) {
            if (g_hero.current_hp <= 0) break;
            render_dashboard(&g_hero);

            if (g_hero.pos_y == 0 && g_hero.pos_x == 7 && !g_hero.has_master_sword) attempt_master_sword_trial(&g_hero);

            if (g_hero.pos_x == 0 && g_hero.pos_y == 0) trigger_boss_encounter(&g_hero, 0);
            else if (g_hero.pos_x == 0 && g_hero.pos_y == 10) trigger_boss_encounter(&g_hero, 1);
            else if (g_hero.pos_x == 10 && g_hero.pos_y == 10) trigger_boss_encounter(&g_hero, 2);
            else if (g_hero.pos_x == 10 && g_hero.pos_y == 0) trigger_boss_encounter(&g_hero, 3);
            
            if (g_hero.pos_x == 5 && g_hero.pos_y == 5 && !g_hero.ganon_defeated) {
                trigger_final_ganon_battle(&g_hero);
                if (g_hero.ganon_defeated) game_running = 0;
            }

            if (g_hero.current_hp <= 0) break;
            
            int key = getch();
            if (key == 27) {
                if (show_esc_menu() == 3) game_running = 0;
                continue;
            }
            if (key == 'e' || key == 'E') { show_inventory_menu(&g_hero); continue; }
            process_hero_movement(&g_hero, key);
        }
    }
    shutdown_zelda_ui();
    return 0;
}
