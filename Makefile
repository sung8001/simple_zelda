CC       = gcc
CFLAGS   = -Wall -std=c99 -D_XOPEN_SOURCE=700 -Iinclude
LDFLAGS  = -lncursesw

CLIENT_TARGET = zelda_game
SERVER_TARGET = zelda_rank_server

CLIENT_SRCS   = src/main.c src/ui_render.c src/map_engine.c src/battle_system.c src/save_manager.c
SERVER_SRCS   = server/rank_server.c

all: $(CLIENT_TARGET) $(SERVER_TARGET)

$(CLIENT_TARGET): $(CLIENT_SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(SERVER_TARGET): $(SERVER_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(CLIENT_TARGET) $(SERVER_TARGET) saves/*.bin saves/*.dat

.PHONY: all clean
