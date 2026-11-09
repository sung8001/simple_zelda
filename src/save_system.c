#include "../include/zelda_game.h"
#include "../include/zelda_ui.h"

void execute_binary_save(const HeroState *hero, int slot_index) {
    mkdir("saves", 0755);
    char path[64];
    snprintf(path, sizeof(path), "saves/zelda_slot%d.bin", slot_index);
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd != -1) {
        write(fd, hero, sizeof(HeroState));
        close(fd);
    }
}

int open_save_selection_menu(HeroState *hero) {
    char path[64] = "saves/zelda_slot1.bin";
    int fd = open(path, O_RDONLY);
    if (fd != -1) {
        read(fd, hero, sizeof(HeroState));
        close(fd);
        print_dialog_text("세이브 시스템 💾", "슬롯 1 데이터를 성공적으로 불러왔습니다!");
        return 1;
    }
    print_dialog_text("세이브 시스템 ⚠️", "저장된 슬롯 1 파일이 없습니다.");
    return 0;
}
