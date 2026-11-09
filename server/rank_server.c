#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../include/zelda_game.h"
#include "../include/zelda_net.h"

static RankBoardPacket g_board = {0};

int main(int argc, char *argv[]) {
    int port = 9190;
    if (argc >= 2) port = atoi(argv[1]);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&sa, sizeof(sa));
    listen(server_fd, 5);

    printf("🏆 [Zelda Rank Server] %d 포트에서 대기 중...\n", port);

    while (1) {
        struct sockaddr_in ca;
        socklen_t len = sizeof(ca);
        int client_fd = accept(server_fd, (struct sockaddr*)&ca, &len);
        if (client_fd < 0) continue;

        CommandCode code;
        if (recv(client_fd, &code, sizeof(code), MSG_PEEK) > 0) {
            if (code == CMD_REQ_RANK) {
                recv(client_fd, &code, sizeof(code), 0);
                send(client_fd, &g_board, sizeof(g_board), 0);
            } else if (code == CMD_SAVE_RECORD) {
                SubmitRecordPacket sub;
                recv(client_fd, &sub, sizeof(sub), 0);
                if (g_board.entry_count < MAX_RANK_ENTRIES) {
                    strncpy(g_board.entries[g_board.entry_count].name, sub.name, MAX_NAME_LEN);
                    g_board.entries[g_board.entry_count].clear_turns = sub.clear_turns;
                    g_board.entries[g_board.entry_count].remaining_hp = sub.remaining_hp;
                    g_board.entry_count++;
                }
            }
        }
        close(client_fd);
    }
    return 0;
}
