#ifndef ZELDA_UI_H
#define ZELDA_UI_H

#include "zelda_game.h"

int get_display_width(const char *str);
int get_centered_x(const char *text);
int get_centered_y(int lines);
void init_zelda_ui(void);
void clear_full_screen(void);
int show_title_screen(void);
void render_dashboard(const HeroState *hero);
int show_esc_menu(void);
void show_inventory_menu(const HeroState *hero);
void print_dialog_text(const char *speaker, const char *msg);
void shutdown_zelda_ui(void);

#endif
