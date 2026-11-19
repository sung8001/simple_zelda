#ifndef ZELDA_NET_H
#define ZELDA_NET_H

#include "zelda_game.h"

#define MAX_RANK_ENTRIES 10
#define MAX_STORED_RECORDS 100

typedef enum {
    CMD_REQ_RANK = 1,
    CMD_SAVE_RECORD = 2
} CommandCode;

typedef struct {
    char name[MAX_NAME_LEN];
    int score;
    double clear_time_sec;
    int remaining_hp;
} RankEntry;

typedef struct {
    int entry_count;
    int total_records;
    int my_rank;
    RankEntry entries[MAX_RANK_ENTRIES];
} RankBoardPacket;

typedef struct {
    CommandCode code;
    char name[MAX_NAME_LEN];
    int score;
    double clear_time_sec;
    int remaining_hp;
} SubmitRecordPacket;

#endif
