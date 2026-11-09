#include "../include/zelda_game.h"
#include "../include/zelda_ui.h"
#include <wchar.h>
#include <locale.h>

WINDOW *win_map = NULL;

int get_display_width(const char *str) {
    mbstate_t ps;
    memset(&ps, 0, sizeof(ps));
    wchar_t wc;
    size_t len;
    int width = 0;
    const char *p = str;
    while ((len = mbrtowc(&wc, p, MB_CUR_MAX, &ps)) > 0 && len != (size_t)-1 && len != (size_t)-2) {
        int w = wcwidth(wc);
        width += (w > 0 ? w : 1);
        p += len;
    }
    return width;
}

int get_centered_x(const char *text) {
    int col = getmaxx(stdscr);
    return (col - get_display_width(text)) / 2;
}

int get_centered_y(int lines) {
    int row = getmaxy(stdscr);
    return (row - lines) / 2;
}

void init_zelda_ui(void) {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_CYAN, COLOR_BLACK);
        init_pair(2, COLOR_RED, COLOR_BLACK);
        init_pair(3, COLOR_BLUE, COLOR_BLACK);
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);
        init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(6, COLOR_WHITE, COLOR_BLACK);
    }
}

void clear_full_screen(void) {
    clear();
    refresh();
}

static void draw_title_decorations(void) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    attron(COLOR_PAIR(1));
    mvprintw(2, 4, "✦");
    mvprintw(3, max_x - 5, "✧");
    mvprintw(6, 6, "·");
    mvprintw(7, max_x - 8, "✦");
    mvprintw(12, 5, "✧");
    mvprintw(14, max_x - 6, "·");
    mvprintw(17, 8, "✦");
    mvprintw(18, max_x - 9, "✧");
    attroff(COLOR_PAIR(1));

    attron(COLOR_PAIR(4) | A_BOLD);
    const char *guide = "🎮 조작법: WASD(이동 / 연타 대시) | E(스탯&인벤토리) | ESC(시스템 메뉴)";
    mvprintw(max_y - 4, get_centered_x(guide), "%s", guide);
    attroff(COLOR_PAIR(4) | A_BOLD);

    attron(COLOR_PAIR(6) | A_BOLD);
    const char *footer = "ⓒ System Programming 001 Team 9 - 정우성 방민종";
    mvprintw(max_y - 2, get_centered_x(footer), "%s", footer);
    attroff(COLOR_PAIR(6) | A_BOLD);
}

int show_title_screen(void) {
    const char *top_text = "T H E   L E G E N D   O F";
    
    const char *title_ascii[] = {
        "████████╗ ███████╗ ██╗      ██████╗    █████╗ ",
        "╚══███╔═╝ ██╔════╝ ██║      ██╔══██╗ ██╔══██╗",
        "   ███╔╝   █████╗   ██║      ██║  ██║ ███████║",
        "  ███╔╝    ██╔══╝   ██║      ██║  ██║ ██╔══██║",
        "████████╗ ███████╗ ███████╗ ██████╔╝ ██║  ██║",
        "╚═══════╝ ╚══════╝ ╚══════╝ ╚═════╝  ╚═╝  ╚═╝"
    };
    int title_lines = sizeof(title_ascii) / sizeof(title_ascii[0]);
    const char *sub_title = "근데 이제 쓸모없는 것을 너무 줄인";
    const char *menu_options[] = { "새 게임 시작하기", "세이브 불러오기", "게임 종료" };
    int num_options = 3;
    int highlight = 0;

    while (1) {
        clear();
        box(stdscr, 0, 0);

        attron(COLOR_PAIR(4) | A_BOLD);
        mvprintw(2, get_centered_x(top_text), "%s", top_text);
        for (int i = 0; i < title_lines; i++) {
            mvprintw(i + 4, get_centered_x(title_ascii[i]), "%s", title_ascii[i]);
        }
        attroff(COLOR_PAIR(4) | A_BOLD);

        attron(COLOR_PAIR(1));
        mvprintw(11, get_centered_x("──────────────────────────────────────────────────"), "──────────────────────────────────────────────────");
        mvprintw(12, get_centered_x(sub_title), "%s", sub_title);
        mvprintw(13, get_centered_x("──────────────────────────────────────────────────"), "──────────────────────────────────────────────────");
        attroff(COLOR_PAIR(1));

        draw_title_decorations();

        int menu_start_y = 15;
        for (int i = 0; i < num_options; i++) {
            int x = get_centered_x(menu_options[i]);
            if (i == highlight) {
                attron(COLOR_PAIR(5) | A_BOLD);
                mvprintw(menu_start_y + (i * 2), x - 4, "➤ %s  ", menu_options[i]);
                attroff(COLOR_PAIR(5) | A_BOLD);
            } else {
                mvprintw(menu_start_y + (i * 2), x, "%s", menu_options[i]);
            }
        }
        refresh();

        int ch = getch();
        if (ch == KEY_UP || ch == 'w' || ch == 'W') {
            highlight = (highlight - 1 + num_options) % num_options;
        } else if (ch == KEY_DOWN || ch == 's' || ch == 'S') {
            highlight = (highlight + 1) % num_options;
        } else if (ch == '\n' || ch == ' ') {
            return highlight;
        }
    }
}

static const char *get_region_name(int r, int c) {
    if (r >= 4 && r <= 6 && c >= 4 && c <= 6) return "🏰 하이랄 성";
    else if (r <= 2 && c <= 2) return "🏜️ 겔드 사막";
    else if (r <= 2 && c >= 4 && c <= 6) return "🌿 시작의 대지";
    else if (r <= 2 && c >= 7 && c <= 10) return "🌪️ 리토의 마을";
    else if (r >= 8 && r <= 10 && c <= 2) return "💧 조라의 마을";
    else if (r >= 8 && r <= 10 && c >= 8 && c <= 10) return "🌋 데스마운틴";
    return "🗺️ 하이랄 대지";
}

void render_dashboard(const HeroState *hero) {
    clear();
    refresh();

    int screen_y, screen_x;
    getmaxyx(stdscr, screen_y, screen_x);

    if (win_map) delwin(win_map);
    win_map = newwin(screen_y - 2, screen_x - 4, 1, 2);

    wclear(win_map);
    box(win_map, 0, 0);

    const char *current_region = get_region_name(hero->pos_y, hero->pos_x);
    mvwprintw(win_map, 0, 2, " [ %s ] ", current_region);

    int win_h, win_w;
    getmaxyx(win_map, win_h, win_w);

    int row_step = (win_h - 4) / 10;
    if (row_step < 1) row_step = 1;
    int col_step = (win_w - 8) / 10;
    if (col_step < 3) col_step = 3;

    for (int r = 0; r < BOARD_DIM; r++) {
        for (int c = 0; c < BOARD_DIM; c++) {
            int dy = 2 + r * row_step;
            int dx = 4 + c * col_step;

            if (r == hero->pos_y && c == hero->pos_x) {
                mvwprintw(win_map, dy, dx, "🧝‍♂️");
                continue;
            }

            if (r == 0 && c == 0) { mvwprintw(win_map, dy, dx, hero->boss_slain[0] ? "💀" : "⚡"); continue; }
            if (r == 10 && c == 0) { mvwprintw(win_map, dy, dx, hero->boss_slain[1] ? "💀" : "💧"); continue; }
            if (r == 10 && c == 10) { mvwprintw(win_map, dy, dx, hero->boss_slain[2] ? "💀" : "🔥"); continue; }
            if (r == 0 && c == 10) { mvwprintw(win_map, dy, dx, hero->boss_slain[3] ? "💀" : "🌪️"); continue; }
            if (r == 0 && c == 7) { mvwprintw(win_map, dy, dx, hero->has_master_sword ? "☁️" : "🗡️"); continue; }
            if (r == 5 && c == 5) { mvwprintw(win_map, dy, dx, hero->ganon_defeated ? "🏰" : "👾"); continue; }

            if (r <= 2 && c <= 2) {
                if (r == 0 && c == 1) mvwprintw(win_map, dy, dx, "🟨");
                else if (r == 0 && c == 2) mvwprintw(win_map, dy, dx, "🌴");
                else if (r == 1 && c == 0) mvwprintw(win_map, dy, dx, "🏜️");
                else mvwprintw(win_map, dy, dx, "🟨");
                continue;
            }
            if (r <= 2 && c >= 4 && c <= 6) {
                if (r == 0 && c == 4) mvwprintw(win_map, dy, dx, "🌸");
                else if (r == 0 && c == 5) mvwprintw(win_map, dy, dx, "🌿");
                else if (r == 0 && c == 6) mvwprintw(win_map, dy, dx, "🌲");
                else if (r == 1 && c == 5) mvwprintw(win_map, dy, dx, "🏡");
                else mvwprintw(win_map, dy, dx, "🌿");
                continue;
            }
            if (r <= 2 && c >= 8 && c <= 10) {
                if (r == 1 && c == 9) mvwprintw(win_map, dy, dx, "✨");
                else mvwprintw(win_map, dy, dx, "☁️");
                continue;
            }
            if (r >= 8 && r <= 10 && c <= 2) {
                if (r == 8 && c == 1) mvwprintw(win_map, dy, dx, "🏝️");
                else if (r == 9 && c == 1) mvwprintw(win_map, dy, dx, "⛲");
                else mvwprintw(win_map, dy, dx, "🌊");
                continue;
            }
            if (r >= 8 && r <= 10 && c >= 8 && c <= 10) {
                if (r == 8 && c == 8) mvwprintw(win_map, dy, dx, "🌋");
                else if (r == 8 && c == 9) mvwprintw(win_map, dy, dx, "🪨");
                else mvwprintw(win_map, dy, dx, "🪨");
                continue;
            }
            if (r >= 4 && r <= 6 && c >= 4 && c <= 6) {
                if (r == 5 && (c == 4 || c == 6)) mvwprintw(win_map, dy, dx, "🟥");
                else mvwprintw(win_map, dy, dx, "🏛️");
                continue;
            }

            if (r == 3 && c == 5) { mvwprintw(win_map, dy, dx, "🌿"); continue; }
            if (r == 0 && c == 3) { mvwprintw(win_map, dy, dx, "🟨"); continue; }
            if (r == 0 && (c == 8 || c == 9)) { mvwprintw(win_map, dy, dx, "☁️"); continue; }
            if (c == 0 && r >= 3 && r <= 7) { mvwprintw(win_map, dy, dx, (r <= 5) ? "🏜️" : "🌊"); continue; }
            if (r == 10 && c >= 3 && c <= 7) { mvwprintw(win_map, dy, dx, (c <= 5) ? "🌊" : "🪨"); continue; }
            if (c == 10 && r >= 3 && r <= 7) { mvwprintw(win_map, dy, dx, (r >= 6) ? "🪨" : "☁️"); continue; }

            mvwprintw(win_map, dy, dx, "  ");
        }
    }

    wrefresh(win_map);
}

int show_esc_menu(void) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    WINDOW *pop = newwin(13, 46, (max_y - 13) / 2, (max_x - 46) / 2);
    keypad(pop, TRUE);

    const char *opts[] = { "1. 🎮 게임 계속하기", "2. 📖 상세 조작법 안내", "3. 💾 세이브 슬롯 저장", "4. 🚪 게임 종료하기" };
    int highlight = 0;

    while (1) {
        wclear(pop);
        box(pop, 0, 0);
        mvwprintw(pop, 1, (46 - get_display_width("⚙️ 시스템 일시정지 메뉴")) / 2, "⚙️ 시스템 일시정지 메뉴");
        mvwprintw(pop, 2, 2, "──────────────────────────────────────────");

        for (int i = 0; i < 4; i++) {
            if (i == highlight) {
                wattron(pop, COLOR_PAIR(5) | A_BOLD);
                mvwprintw(pop, 4 + i * 2, 8, "➤ %s", opts[i]);
                wattroff(pop, COLOR_PAIR(5) | A_BOLD);
            } else {
                mvwprintw(pop, 4 + i * 2, 10, "%s", opts[i]);
            }
        }
        wrefresh(pop);

        int ch = wgetch(pop);
        if (ch == KEY_UP || ch == 'w' || ch == 'W') highlight = (highlight - 1 + 4) % 4;
        else if (ch == KEY_DOWN || ch == 's' || ch == 'S') highlight = (highlight + 1) % 4;
        else if (ch == 27) { delwin(pop); return 0; }
        else if (ch == '\n' || ch == ' ') { delwin(pop); return highlight; }
    }
}

void show_inventory_menu(const HeroState *hero) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    WINDOW *inv = newwin(19, 62, (max_y - 19) / 2, (max_x - 62) / 2);
    keypad(inv, TRUE);

    wclear(inv);
    box(inv, 0, 0);
    mvwprintw(inv, 1, (62 - get_display_width("🎒 용사 스탯창 & 인벤토리")) / 2, "🎒 용사 스탯창 & 인벤토리");
    mvwprintw(inv, 2, 2, "──────────────────────────────────────────────────────────");

    mvwprintw(inv, 4, 4, "[ 용사 스탯 정보 ]");
    mvwprintw(inv, 5, 6, "- 용사 이름  : %s", hero->hero_name);
    mvwprintw(inv, 6, 6, "- 현재 체력  : %d / %d HP", hero->current_hp, hero->max_hp);
    mvwprintw(inv, 7, 6, "- 기본 공격력: %d ATK", hero->attack_power);
    mvwprintw(inv, 8, 6, "- 장착 무기  : %s", hero->has_master_sword ? "🗡️ 마스터소드 (기본 ATK +40 폭증!)" : "🗡️ 낡은 목검");

    mvwprintw(inv, 10, 4, "[ 4대 영걸의 증표 소지 현황 ]");
    mvwprintw(inv, 11, 6, "⚡ 우르보사의 가누다: %s", hero->token_urbosa ? "✅ 획득 (공격력 +25)" : "❌ 미획득");
    mvwprintw(inv, 12, 6, "💧 미파의 기도     : %s", hero->token_mipha ? "✅ 획득 (사망 시 1회 풀부활)" : "❌ 미획득");
    mvwprintw(inv, 13, 6, "🔥 다르케르의 수호 : %s", hero->token_daruk ? "✅ 획득 (최대 체력 +50)" : "❌ 미획득");
    mvwprintw(inv, 14, 6, "🌪️ 리발의 용오름   : %s", hero->token_revali ? "✅ 획득 (WASD 더블클릭 2칸 대시)" : "❌ 미획득");

    mvwprintw(inv, 16, 6, "🔥 현재 QTE 콤보: %d 스택 | 스코어: %d 점", hero->combo_count, hero->total_score);
    mvwprintw(inv, 17, 14, "(닫으려면 E, ESC 또는 Space 누름)");
    wrefresh(inv);

    while (1) {
        int ch = wgetch(inv);
        if (ch == 'e' || ch == 'E' || ch == 27 || ch == ' ') break;
    }
    delwin(inv);
}

void print_dialog_text(const char *speaker, const char *msg) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    WINDOW *dlg = newwin(6, 70, (max_y - 6) / 2, (max_x - 70) / 2);

    wclear(dlg);
    box(dlg, 0, 0);
    mvwprintw(dlg, 1, 2, "💬 [%s]", speaker);
    mvwprintw(dlg, 3, 4, "%s", msg);
    mvwprintw(dlg, 4, 45, "(Space 누르면 계속...)");
    wrefresh(dlg);
    while (getch() != ' ');
    delwin(dlg);
}

void shutdown_zelda_ui(void) {
    if (win_map) delwin(win_map);
    endwin();
}
