#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <zmq.h>

#define WINDOW_SIZE 30
#define SCORES_WIN_HEIGHT (26 + 1)
#define SCORES_WIN_WIDTH 12

typedef enum MessageType_t
{
    CONNECT,
    MOVEMENT,
    DISCONNECT
} MessageType_t;

typedef enum ClientType_t
{
    LIZARD,
    ROACH,
    WASP
} ClientType_t;

typedef enum DisplayType_t
{
    INCREMENTAL_UPDATE,
    GAME_SNAPSHOT,
    SCORE_SNAPSHOT
} DisplayType_t;

typedef enum direction_t
{
    UP,
    DOWN,
    LEFT,
    RIGHT
} direction_t;

WINDOW *create_scores_window(int x, int y);

void verify_status(void *context, void *socket, int size);

void display_game_state(WINDOW *win, char game_state[WINDOW_SIZE * WINDOW_SIZE]);

#endif