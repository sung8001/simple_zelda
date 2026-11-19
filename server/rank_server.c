#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../include/zelda_game.h"
#include "../include/zelda_net.h"

#define RANK_DB_FILE "saves/hall_of_fame.bin"

static RankEntry g_records[MAX_STORED_RECORDS];
static int g_record_count = 0;

// 1순위: 점수 내림차순, 2순위: 클리어 시간 오름차순
static int compare_rank(const void *a, const void *b) {
    const RankEntry *ra = (const RankEntry *)a;
    const RankEntry *rb = (const RankEntry *)b;
    if (rb->score != ra->score) {
        return (rb->score > ra->score) ? 1 : -1;
    }
    if (ra->clear_time_sec != rb->clear_time_sec) {
        return (ra->clear_time_sec > rb->clear_time_sec) ? 1 : -1;
    }
    return 0;
}

static void load_records_from_file(void) {
    mkdir("saves", 0755);
    int fd = open(RANK_DB_FILE, O_RDONLY);
    if (fd < 0) return;
    ssize_t bytes = read(fd, g_records, sizeof(g_records));
    if (bytes > 0) {
        g_record_count = (int)(bytes / sizeof(RankEntry));
    }
    close(fd);
}

static void save_records_to_file(void) {
    mkdir("saves", 0755);
    int fd = open(RANK_DB_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return;
    write(fd, g_records, sizeof(RankEntry) * g_record_count);
    close(fd);
}

static void build_board_packet(RankBoardPacket *pkt, int my_rank) {
    memset(pkt, 0, sizeof(RankBoardPacket));
    pkt->total_records = g_record_count;
    pkt->my_rank = my_rank;
    pkt->entry_count = (g_record_count < MAX_RANK_ENTRIES) ? g_record_count : MAX_RANK_ENTRIES;
    for (int i = 0; i < pkt->entry_count; i++) {
        pkt->entries[i] = g_records[i];
    }
}

int main(int argc, char *argv[]) {
    int port = 9190;
    if (argc >= 2) port = atoi(argv[1]);

    load_records_from_file();

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
        perror("bind");
        return 1;
    }
    listen(server_fd, 5);

    printf("🏆 [Zelda Rank Server] %d 포트에서 대기 중... (저장된 기록: %d건)\n", port, g_record_count);

    while (1) {
        struct sockaddr_in ca;
        socklen_t len = sizeof(ca);
        int client_fd = accept(server_fd, (struct sockaddr*)&ca, &len);
        if (client_fd < 0) continue;

        CommandCode code;
        if (recv(client_fd, &code, sizeof(code), MSG_PEEK) > 0) {
            if (code == CMD_REQ_RANK) {
                recv(client_fd, &code, sizeof(code), 0);
                RankBoardPacket pkt;
                build_board_packet(&pkt, -1);
                send(client_fd, &pkt, sizeof(pkt), 0);
            } else if (code == CMD_SAVE_RECORD) {
                SubmitRecordPacket sub;
                recv(client_fd, &sub, sizeof(sub), 0);

                RankEntry new_entry;
                memset(&new_entry, 0, sizeof(RankEntry));
                strncpy(new_entry.name, sub.name, MAX_NAME_LEN - 1);
                new_entry.score = sub.score;
                new_entry.clear_time_sec = sub.clear_time_sec;
                new_entry.remaining_hp = sub.remaining_hp;

                if (g_record_count < MAX_STORED_RECORDS) {
                    g_records[g_record_count++] = new_entry;
                } else {
                    g_records[MAX_STORED_RECORDS - 1] = new_entry;
                }

                qsort(g_records, g_record_count, sizeof(RankEntry), compare_rank);
                save_records_to_file();

                int my_rank = -1;
                for (int i = 0; i < g_record_count; i++) {
                    if (strcmp(g_records[i].name, new_entry.name) == 0 &&
                        g_records[i].score == new_entry.score &&
                        g_records[i].clear_time_sec == new_entry.clear_time_sec) {
                        my_rank = i + 1;
                        break;
                    }
                }

                printf("🏅 신규 기록 등록: %s | %d점 | %.2f초 (전체 %d위)\n",
                       new_entry.name, new_entry.score, new_entry.clear_time_sec, my_rank);

                RankBoardPacket pkt;
                build_board_packet(&pkt, my_rank);
                send(client_fd, &pkt, sizeof(pkt), 0);
            }
        }
        close(client_fd);
    }
    return 0;
}
