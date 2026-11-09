#include "../include/zelda_game.h"
#include "../include/zelda_net.h"

void process_ranking_request(int client_fd) {
    RankBoardPacket packet;
    packet.code = CMD_RES_RANK;
    packet.entry_count = 0;

    int fd = open("saves/online_rank.dat", O_RDONLY);
    if (fd >= 0) {
        read(fd, &packet.entry_count, sizeof(int));
        read(fd, packet.entries, sizeof(RankEntry) * packet.entry_count);
        close(fd);
    }

    send(client_fd, &packet, sizeof(packet), 0);
}

void process_score_submission(int client_fd) {
    SubmitRecordPacket req;
    if (recv(client_fd, &req, sizeof(req), 0) <= 0) return;

    RankBoardPacket packet;
    packet.entry_count = 0;

    int fd = open("saves/online_rank.dat", O_RDWR | O_CREAT, 0644);
    if (fd >= 0) {
        read(fd, &packet.entry_count, sizeof(int));
        read(fd, packet.entries, sizeof(RankEntry) * packet.entry_count);

        if (packet.entry_count < 5) {
            strncpy(packet.entries[packet.entry_count].name, req.name, MAX_NAME_LEN - 1);
            packet.entries[packet.entry_count].clear_turns = req.clear_turns;
            packet.entries[packet.entry_count].remaining_hp = req.remaining_hp;
            packet.entry_count++;
        }

        lseek(fd, 0, SEEK_SET);
        write(fd, &packet.entry_count, sizeof(int));
        write(fd, packet.entries, sizeof(RankEntry) * packet.entry_count);
        close(fd);
    }
}

int main(int argc, char *argv[]) {
    int port = (argc == 2) ? atoi(argv[1]) : 9190;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    printf("🏆 [Zelda Rank Server] 9190 포트에서 대기 중...\n");

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) continue;

        CommandCode code;
        if (recv(client_fd, &code, sizeof(code), MSG_PEEK) > 0) {
            if (code == CMD_REQ_RANK) {
                recv(client_fd, &code, sizeof(code), 0);
                process_ranking_request(client_fd);
            } else if (code == CMD_SAVE_RECORD) {
                process_score_submission(client_fd);
            }
        }
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
