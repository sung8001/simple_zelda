#ifndef ZELDA_NET_H
#define ZELDA_NET_H

#define MAX_RANK_ENTRIES 10

typedef enum {
    CMD_REQ_RANK = 1,
    CMD_SAVE_RECORD = 2
} CommandCode;

typedef struct {
    char name[MAX_NAME_LEN];
    int clear_turns;
    int remaining_hp;
} RankEntry;

typedef struct {
    int entry_count;
    RankEntry entries[MAX_RANK_ENTRIES];
} RankBoardPacket;

typedef struct {
    CommandCode code;
    char name[MAX_NAME_LEN];
    int clear_turns;
    int remaining_hp;
} SubmitRecordPacket;

#endif
