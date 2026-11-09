#ifndef ZELDA_NET_H
#define ZELDA_NET_H

#include <stdint.h>

#define CMD_REQ_RANK    101
#define CMD_RES_RANK    102
#define CMD_SAVE_RECORD 103

typedef uint32_t CommandCode;

typedef struct {
    char name[24];
    int clear_turns;
    int remaining_hp;
} RankEntry;

typedef struct {
    CommandCode code;
    RankEntry entries[5];
    int entry_count;
} RankBoardPacket;

typedef struct {
    CommandCode code;
    char name[24];
    int clear_turns;
    int remaining_hp;
} SubmitRecordPacket;

#endif
