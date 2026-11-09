CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_XOPEN_SOURCE=700
LIBS = -lncursesw

SRC = src/main.c src/ui_render.c src/battle_system.c src/map_engine.c src/save_system.c
HEADERS = include/zelda_game.h include/zelda_ui.h include/zelda_net.h

TARGETS = zelda_game zelda_rank_server

all: $(TARGETS)

zelda_game: $(SRC) $(HEADERS)
	@mkdir -p saves
	$(CC) $(CFLAGS) -o zelda_game $(SRC) $(LIBS)

zelda_rank_server: server/rank_server.c $(HEADERS)
	$(CC) $(CFLAGS) -o zelda_rank_server server/rank_server.c

clean:
	rm -f $(TARGETS) saves/*.bin
