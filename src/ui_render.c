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
    int x = (col - get_display_width(text)) / 2;
    return (x > 2) ? x : 2;
}

int get_centered_y(int lines) {
    int row = getmaxy(stdscr);
    return (row - lines) / 2;
}

void init_zelda_ui(void) {
    if (setlocale(LC_ALL, "C.UTF-8") == NULL) {
        if (setlocale(LC_ALL, "en_US.UTF-8") == NULL) {
            setlocale(LC_ALL, "");
        }
    }
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

    attron(COLOR_PAIR(6) | A_BOLD);
    const char *footer = "(c) System Programming 001 Team 9 - 정우성 방민종";
    mvprintw(max_y - 2, get_centered_x(footer), "%s", footer);
    attroff(COLOR_PAIR(6) | A_BOLD);
}

int show_title_screen(void) {
    const char *top_text = "T H E   L E G E N D   O F";
    
    const char *title_ascii[] = {
        "████████╗ ███████╗ ██╗      ██████╗    █████╗ ",
        "╚════███║ ██╔════╝ ██║      ██╔══██╗  ██╔══██╗",
        "   ███╔═╝ █████╗   ██║      ██║  ██║  ███████║",
        " ███╔═╝   ██╔══╝   ██║      ██║  ██║  ██╔══██║",
        "████████╗ ███████╗ ███████╗ ██████╔╝  ██║  ██║",
        "╚═══════╝ ╚══════╝ ╚══════╝ ╚═════╝   ╚═╝  ╚═╝"
    };
    int title_lines = sizeof(title_ascii) / sizeof(title_ascii[0]);
    const char *sub_title = "근데 이제 쓸모없는 것을 너무 줄인";
    const char *divider = "──────────────────────────────────────────────────";
    const char *menu_options[] = { "새 게임", "조작법 및 게임 방법", "종료" };
    int num_options = 3;
    int highlight = 0;

    while (1) {
        clear();
        box(stdscr, 0, 0);

        attron(COLOR_PAIR(4) | A_BOLD);
        mvprintw(2, get_centered_x(top_text), "%s", top_text);
        int base_x = get_centered_x(title_ascii[0]);
        for (int i = 0; i < title_lines; i++) {
            mvprintw(i + 4, base_x, "%s", title_ascii[i]);
        }
        attroff(COLOR_PAIR(4) | A_BOLD);

        attron(COLOR_PAIR(1));
        mvprintw(11, get_centered_x(divider), "%s", divider);
        mvprintw(12, get_centered_x(sub_title), "%s", sub_title);
        mvprintw(13, get_centered_x(divider), "%s", divider);
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

void show_how_to_play(void) {
    const char *lines[] = {
        "조작법",
        "WASD : 이동  |  E : 상태창  |  ESC : 메뉴",
        "",
        "게임 방법",
        "하이랄 성 재앙 가논 처치"
    };
    int num_lines = sizeof(lines) / sizeof(lines[0]);
    int start_y = get_centered_y(num_lines + 4);
    if (start_y < 2) start_y = 2;

    clear();
    box(stdscr, 0, 0);

    for (int i = 0; i < num_lines; i++) {
        if (lines[i][0] == '\0') continue;
        if (i == 0 || i == 3) {
            attron(COLOR_PAIR(4) | A_BOLD);
            mvprintw(start_y + i, get_centered_x(lines[i]), "%s", lines[i]);
            attroff(COLOR_PAIR(4) | A_BOLD);
        } else {
            mvprintw(start_y + i, get_centered_x(lines[i]), "%s", lines[i]);
        }
    }

    const char *exit_msg = "[ 아무 키나 눌러 돌아가기 ]";
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(start_y + num_lines + 2, get_centered_x(exit_msg), "%s", exit_msg);
    attroff(COLOR_PAIR(1) | A_BOLD);

    refresh();
    getch();
}

void input_hero_name(char *out_name, size_t max_len) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    WINDOW *win = newwin(6, 46, (max_y - 6) / 2, (max_x - 46) / 2);
    keypad(win, TRUE);
    curs_set(1);

    char buf[MAX_NAME_LEN] = {0};
    size_t len = 0;

    while (1) {
        wclear(win);
        box(win, 0, 0);
        mvwprintw(win, 1, 3, "용사 이름 (미입력 시 Link)");
        mvwprintw(win, 3, 3, "> %s_", buf);
        wrefresh(win);

        int ch = wgetch(win);
        if (ch == '\n' || ch == '\r') {
            break;
        } else if ((ch == KEY_BACKSPACE || ch == 127 || ch == '\b') && len > 0) {
            buf[--len] = '\0';
        } else if (ch >= 33 && ch <= 126 && len + 1 < max_len) {
            buf[len++] = (char)ch;
            buf[len] = '\0';
        }
    }

    curs_set(0);
    delwin(win);

    if (len == 0) {
        strncpy(out_name, "Link", max_len - 1);
    } else {
        strncpy(out_name, buf, max_len - 1);
    }
    out_name[max_len - 1] = '\0';
}

int prompt_yes_no_dialog(const char *speaker, const char *msg) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    WINDOW *dlg = newwin(6, 62, (max_y - 6) / 2, (max_x - 62) / 2);
    keypad(dlg, TRUE);

    wclear(dlg);
    box(dlg, 0, 0);
    mvwprintw(dlg, 1, 2, "[%s]", speaker);
    mvwprintw(dlg, 2, 4, "%s", msg);
    wattron(dlg, COLOR_PAIR(4) | A_BOLD);
    mvwprintw(dlg, 4, 18, "[ Y: 진행 ]   [ N: 취소 ]");
    wattroff(dlg, COLOR_PAIR(4) | A_BOLD);
    wrefresh(dlg);

    while (1) {
        int ch = wgetch(dlg);
        if (ch == 'y' || ch == 'Y' || ch == '\n' || ch == '\r') {
            delwin(dlg);
            return 1;
        } else if (ch == 'n' || ch == 'N' || ch == 27) {
            delwin(dlg);
            return 0;
        }
    }
}

static const char *get_region_name(int r, int c) {
    if (r >= 4 && r <= 6 && c >= 4 && c <= 6) return "하이랄 성";
    else if (r <= 2 && c <= 2) return "겔드 사막";
    else if (r <= 2 && c >= 4 && c <= 6) return "시작의 대지";
    else if (r <= 2 && c >= 7 && c <= 10) return "리토의 마을";
    else if (r >= 8 && r <= 10 && c <= 2) return "조라의 마을";
    else if (r >= 8 && r <= 10 && c >= 8 && c <= 10) return "데스마운틴";
    return "하이랄 대지";
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
    mvwprintw(win_map, 0, 2, " [ %s ] HP:%d/%d | ATK:%d | 포션:%d | %d점 ", 
              current_region, hero->current_hp, hero->max_hp, hero->attack_power, hero->potion_inventory, hero->total_score);

    int win_h, win_w;
    getmaxyx(win_map, win_h, win_w);

    int row_step = (win_h - 4) / 10;
    if (row_step < 1) row_step = 1;
    int col_step = (win_w - 16) / 10;
    if (col_step < 3) col_step = 3;

    int grid_h = 10 * row_step + 1;
    int grid_w = 10 * col_step + 2;
    int start_y = (win_h - grid_h) / 2;
    if (start_y < 2) start_y = 2;
    int start_x = (win_w - grid_w) / 2;
    if (start_x < 4) start_x = 4;

    for (int r = 0; r < BOARD_DIM; r++) {
        for (int c = 0; c < BOARD_DIM; c++) {
            int dy = start_y + r * row_step;
            int dx = start_x + c * col_step;

            if (r == hero->pos_y && c == hero->pos_x) {
                mvwprintw(win_map, dy, dx, "🦸");
                continue;
            }

            if (r == 0 && c == 0) { mvwprintw(win_map, dy, dx, hero->boss_slain[0] ? "💀" : "⚡"); continue; }
            if (r == 10 && c == 0) { mvwprintw(win_map, dy, dx, hero->boss_slain[1] ? "💀" : "💧"); continue; }
            if (r == 10 && c == 10) { mvwprintw(win_map, dy, dx, hero->boss_slain[2] ? "💀" : "🔥"); continue; }
            if (r == 0 && c == 10) { mvwprintw(win_map, dy, dx, hero->boss_slain[3] ? "💀" : "🌀"); continue; }
            if (r == 0 && c == 7) { mvwprintw(win_map, dy, dx, hero->has_master_sword ? "💨" : "🗡️"); continue; }
            if (r == 5 && c == 5) { mvwprintw(win_map, dy, dx, "🏰"); continue; }

            if (r <= 2 && c <= 2) {
                if (r == 0 && c == 1) mvwprintw(win_map, dy, dx, "🟨");
                else if (r == 0 && c == 2) mvwprintw(win_map, dy, dx, "🌴");
                else if (r == 1 && c == 0) mvwprintw(win_map, dy, dx, "🌵");
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
                else mvwprintw(win_map, dy, dx, "💨");
                continue;
            }
            if (r >= 8 && r <= 10 && c <= 2) {
                if (r == 8 && c == 1) mvwprintw(win_map, dy, dx, "🌴");
                else if (r == 9 && c == 1) mvwprintw(win_map, dy, dx, "⛲");
                else mvwprintw(win_map, dy, dx, "🌊");
                continue;
            }
            if (r >= 8 && r <= 10 && c >= 8 && c <= 10) {
                if (r == 8 && c == 8) mvwprintw(win_map, dy, dx, "🌋");
                else mvwprintw(win_map, dy, dx, "🪨");
                continue;
            }
            if (r >= 4 && r <= 6 && c >= 4 && c <= 6) {
                if (r == 5 && (c == 4 || c == 6)) mvwprintw(win_map, dy, dx, "🟥");
                else mvwprintw(win_map, dy, dx, "🏛");
                continue;
            }

            if (r == 3 && c == 5) { mvwprintw(win_map, dy, dx, "🌿"); continue; }
            if (r == 0 && c == 3) { mvwprintw(win_map, dy, dx, "🟨"); continue; }
            if (r == 0 && (c == 8 || c == 9)) { mvwprintw(win_map, dy, dx, "💨"); continue; }
            if (c == 0 && r >= 3 && r <= 7) { mvwprintw(win_map, dy, dx, (r <= 5) ? "🌵" : "🌊"); continue; }
            if (r == 10 && c >= 3 && c <= 7) { mvwprintw(win_map, dy, dx, (c <= 5) ? "🌊" : "🪨"); continue; }
            if (c == 10 && r >= 3 && r <= 7) { mvwprintw(win_map, dy, dx, (r >= 6) ? "🪨" : "💨"); continue; }

            mvwprintw(win_map, dy, dx, "  ");
        }
    }

    wrefresh(win_map);
}

int show_esc_menu(void) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    WINDOW *pop = newwin(11, 38, (max_y - 11) / 2, (max_x - 38) / 2);
    keypad(pop, TRUE);

    const char *opts[] = { "계속하기", "조작법 및 게임 방법", "메인으로" };
    int highlight = 0;

    while (1) {
        wclear(pop);
        box(pop, 0, 0);
        mvwprintw(pop, 1, (38 - get_display_width("메뉴")) / 2, "메뉴");
        mvwprintw(pop, 2, 2, "──────────────────────────────────");

        for (int i = 0; i < 3; i++) {
            if (i == highlight) {
                wattron(pop, COLOR_PAIR(5) | A_BOLD);
                mvwprintw(pop, 4 + i * 2, 7, "➤ %s", opts[i]);
                wattroff(pop, COLOR_PAIR(5) | A_BOLD);
            } else {
                mvwprintw(pop, 4 + i * 2, 9, "%s", opts[i]);
            }
        }
        wrefresh(pop);

        int ch = wgetch(pop);
        if (ch == KEY_UP || ch == 'w' || ch == 'W') highlight = (highlight - 1 + 3) % 3;
        else if (ch == KEY_DOWN || ch == 's' || ch == 'S') highlight = (highlight + 1) % 3;
        else if (ch == 27) { delwin(pop); return 0; }
        else if (ch == '\n' || ch == ' ') { delwin(pop); return highlight; }
    }
}

void show_inventory_menu(const HeroState *hero) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    WINDOW *inv = newwin(16, 48, (max_y - 16) / 2, (max_x - 48) / 2);
    keypad(inv, TRUE);

    wclear(inv);
    box(inv, 0, 0);
    mvwprintw(inv, 1, (48 - get_display_width("상태창")) / 2, "상태창");
    mvwprintw(inv, 2, 2, "────────────────────────────────────────────");

    mvwprintw(inv, 4, 4, "이름: %s  |  HP: %d/%d  |  ATK: %d", hero->hero_name, hero->current_hp, hero->max_hp, hero->attack_power);
    mvwprintw(inv, 5, 4, "무기: %s", hero->has_master_sword ? "마스터소드" : "목검");

    mvwprintw(inv, 7, 4, "⚡ 우르보사의 증표 : %s", hero->token_urbosa ? "O" : "X");
    mvwprintw(inv, 8, 4, "💧 미파의 증표     : %s", hero->token_mipha ? "O" : "X");
    mvwprintw(inv, 9, 4, "🔥 다르케르의 증표 : %s", hero->token_daruk ? "O" : "X");
    mvwprintw(inv, 10, 4, "🌪️ 리발의 증표     : %s", hero->token_revali ? "O" : "X");

    mvwprintw(inv, 12, 4, "최대 콤보: %d  |  점수: %d점", hero->max_combo, hero->total_score);
    mvwprintw(inv, 14, (48 - get_display_width("닫기 : E")) / 2, "닫기 : E");
    wrefresh(inv);

    while (1) {
        int ch = wgetch(inv);
        if (ch == 'e' || ch == 'E' || ch == 27) break;
    }
    delwin(inv);
}

void print_dialog_text(const char *speaker, const char *msg) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    int w = 54;
    int h = 7;
    WINDOW *dlg = newwin(h, w, (max_y - h) / 2, (max_x - w) / 2);
    wtimeout(dlg, 400);

    char spk_buf[64];
    snprintf(spk_buf, sizeof(spk_buf), "[ %s ]", speaker);

    int blink = 1;
    while (1) {
        wclear(dlg);
        box(dlg, 0, 0);

        wattron(dlg, COLOR_PAIR(4) | A_BOLD);
        mvwprintw(dlg, 1, 3, "%s", spk_buf);
        wattroff(dlg, COLOR_PAIR(4) | A_BOLD);

        wattron(dlg, A_BOLD);
        mvwprintw(dlg, 3, (w - get_display_width(msg)) / 2, "%s", msg);
        wattroff(dlg, A_BOLD);

        if (blink) {
            wattron(dlg, COLOR_PAIR(4) | A_BOLD);
            mvwprintw(dlg, 5, w - 4, "▼");
            wattroff(dlg, COLOR_PAIR(4) | A_BOLD);
        }

        wrefresh(dlg);

        int ch = wgetch(dlg);
        if (ch == ' ') break;
        blink = !blink;
    }

    delwin(dlg);
}

void shutdown_zelda_ui(void) {
    if (win_map) delwin(win_map);
    endwin();
}
