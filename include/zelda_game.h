#ifndef ZELDA_GAME_H
#define ZELDA_GAME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <ncursesw/ncurses.h>

#define BOARD_DIM 11
#define MAX_NAME_LEN 24

typedef struct {
    char hero_name[MAX_NAME_LEN];
    int pos_x, pos_y;
    int current_hp, max_hp;
    int attack_power;
    int potion_inventory;
    int move_range;
    int has_master_sword;
    int boss_slain[4];
    
    int token_urbosa; 
    int token_mipha;  
    int token_daruk;  
    int token_revali; 

    int combo_count;
    int max_combo;
    int total_score;
    int ganon_defeated;
    int total_turns;
    double clear_time_sec;
} HeroState;

typedef struct {
    char name[48];
    int hp, max_hp;
    int base_damage;
    char symbol[8];
} BossMonster;

#endif
