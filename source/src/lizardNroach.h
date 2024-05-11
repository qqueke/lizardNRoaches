#ifndef LIZARD_N_ROACH_H
#define LIZARD_N_ROACH_H

#include <math.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>
// #include <pthread.h>

#include "global.h"

#define MAX_CLIENTS 26
#define MAX_BOTS 300
#define VICTORY_POINTS 50

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

/*------------------- Board Structs -----------------*/

typedef struct Board_Tile_t
{
    int id;
    char ch;
    struct Board_Tile_t *next;
} Board_Tile_t;

typedef struct Board_List_t
{
    Board_Tile_t *head;
} Board_List_t;

typedef struct Board_t
{
    int size;
    Board_List_t **lists;
} Board_t;

/*------------------- Display messages -----------------*/

typedef struct Display_Message_t
{
    char ch;
    int pos_x;
    int pos_y;
} Display_Message_t;

/*------------------- Lizard messages -----------------*/

typedef struct Lizard_Message_t
{
    int type; // 0 if conn, 1 otherwise
    char ch;  // primary identifier
    int token;
    direction_t direction;
} Lizard_Message_t;

/*------------------- Roach_t messages -----------------*/

typedef struct Roach_Message_t
{
    int type; // 0 if conn, 1 movement, -1 quit, 2 request client id
    int id;
    int token;
    int roach_id;
    direction_t direction;
} Roach_Message_t;

/*------------------- Roach_t and List structs -----------------*/

typedef struct Roach_t
{
    int client_id;
    int id;
    char digit;
    int pos_x;
    int pos_y;
    bool is_dead;
    struct Roach_t *next;
} Roach_t;

typedef struct Roaches_List_t
{
    Roach_t *head;
} Roaches_List_t;

/*------------------- Dead_Roach_t and List structs -----------------*/

typedef struct Dead_Roach_t
{
    int id;
    time_t time_eaten;
    struct Dead_Roach_t *next;
} Dead_Roach_t;

typedef struct Dead_Roaches_List_t
{
    Dead_Roach_t *head;
    Dead_Roach_t *tail;
} Dead_Roaches_List_t;

/*------------------- Wasp_t messages -----------------*/

typedef struct Wasp_Message_t
{
    int type; // 0 if conn, 1 movement, -1 quit, 2 request client id
    int id;
    int token;
    int wasp_id;
    direction_t direction;
} Wasp_Message_t;

/*------------------- Roach_t and List structs -----------------*/

typedef struct Wasp_t
{
    int client_id;
    int id;
    char digit;
    int pos_x;
    int pos_y;
    struct Wasp_t *next;
} Wasp_t;

typedef struct Wasps_List_t
{
    Wasp_t *head;
} Wasps_List_t;

/*------------------- Lizard_Client_t and List structs -----------------*/

typedef struct Lizard_Client_t
{
    char ch;
    int token;
    int pos_x;
    int pos_y;
    direction_t body_direction;
    int score;
    time_t time;
    struct Lizard_Client_t *next;
} Lizard_Client_t;

typedef struct Lizard_Client_List_t
{
    Lizard_Client_t *head;
    Lizard_Client_t *tail;
} Lizard_Client_List_t;

/*------------------- Roach_Client_t and List structs -----------------*/

typedef struct Roach_Client_t
{
    int id;
    int token;
    time_t time;
    struct Roach_Client_t *next;
} Roach_Client_t;

typedef struct Roach_Client_List_t
{
    Roach_Client_t *head;
} Roach_Client_List_t;
/*------------------- Wasp_Client_t and List structs -----------------*/

typedef struct Wasp_Client_t
{
    int id;
    int token;
    time_t time;
    struct Wasp_Client_t *next;
} Wasp_Client_t;

typedef struct Wasp_Client_List_t
{
    Wasp_Client_t *head;
} Wasp_Client_List_t;
/*------------------- Character and List structs -----------------*/

typedef struct Character_t
{
    char ch;
    struct Character_t *next;
} Character_t;

typedef struct Alphabet_List_t
{
    Character_t *head;
} Alphabet_List_t;

/*-------------------  Auxiliary functions -----------------*/

void new_position(int *x, int *y, direction_t direction);

void verify_status(void *context, void *socket, int size);

/*-------------------  Board functions -----------------*/

Board_t *create_board(int size);

void add_to_board(Board_t *array, int list_index, int id, char ch);

void remove_from_board(Board_t *array, int list_index, char *ch);

void remove_roaches_from_board(void *publisher, Board_t *array, Roaches_List_t *roach_list, int client_id);

void free_board(Board_t *array);

void copy_game_state(WINDOW *win, char game_state[WINDOW_SIZE * WINDOW_SIZE]);

void fill_game_state(Board_t *array, WINDOW *win, char game_state[WINDOW_SIZE * WINDOW_SIZE]);

void display_game_state(WINDOW *win, char game_state[WINDOW_SIZE * WINDOW_SIZE]);
/*-------------------  Roaches functions -----------------*/

Roaches_List_t *create_roaches_list();

void new_roach_position(Board_t *board, int *pos_x, int *pos_y);

Roach_t *create_roach(Board_t *board, int client_id, int id, int digit);

void insert_roach(Roaches_List_t *list, Roach_t *roach);

Roach_t *find_roach_by_id(Roaches_List_t *list, int id);

Roaches_List_t *find_roaches_by_client_id(Roaches_List_t *list, int client_id);

void remove_roach_by_id(Roaches_List_t *list, int id);

void free_roaches(Roaches_List_t *list);
/*-------------------  Wasps functions -----------------*/
Wasps_List_t *create_wasps_list();

void new_wasp_position(Board_t *board, int *pos_x, int *pos_y);

Wasp_t *create_wasp(Board_t *board, int client_id, int id);

void insert_wasp(Wasps_List_t *list, Wasp_t *wasp);

Wasp_t *find_wasp_by_id(Wasps_List_t *list, int id);

Wasps_List_t *find_wasps_by_client_id(Wasps_List_t *list, int client_id);

void remove_roaches_by_client_id(Roaches_List_t *list, int *n_roaches, int client_id);

void remove_wasps_by_client_id(Wasps_List_t *list, int *n_wasps, int client_id);

void remove_wasp_by_id(Wasps_List_t *list, int id);

void remove_wasps_from_board(void *publisher, Board_t *array, Wasps_List_t *wasp_list, int client_id);

void free_wasps(Wasps_List_t *list);

/*------------------- Wasp Client functions -----------------*/

Wasp_Client_List_t *create_wasp_client_list();

Wasp_Client_t *create_wasp_client(int id, int token);

void insert_wasp_client(Wasp_Client_List_t *list, Wasp_Client_t *wasp_client);

Wasp_Client_t *find_wasp_client_by_id(Wasp_Client_List_t *list, int id);

void remove_wasp_client_by_id(Wasp_Client_List_t *list, int token);

void free_wasp_clients(Wasp_Client_List_t *list);

/*------------------- Dead Roaches functions -----------------*/

Dead_Roaches_List_t *create_dead_roaches_list();

Dead_Roach_t *create_dead_roach(int id);

void insert_dead_roach(Dead_Roaches_List_t *list, int id);

void insert_dead_roach_lf(Dead_Roaches_List_t *list, int id);

void revive_roaches(void *publisher, Board_t *board, Dead_Roaches_List_t *dead_roaches, Roaches_List_t *roaches);

void free_dead_roaches(Dead_Roaches_List_t *list);

/*------------------- Roach Client functions -----------------*/

Roach_Client_List_t *create_roach_client_list();

Roach_Client_t *create_roach_client(int id, int token);

void insert_roach_client(Roach_Client_List_t *list, Roach_Client_t *roach_client);

Roach_Client_t *find_roach_client_by_id(Roach_Client_List_t *list, int id);

void remove_roach_client_by_id(Roach_Client_List_t *list, int token);

void free_roach_clients(Roach_Client_List_t *list);

/*------------------- Board_t functions -----------------*/

// Returns the sum of the eaten roaches
int eat_roaches(Board_t *array, int list_index, Roaches_List_t *roaches, Dead_Roaches_List_t *Dead_Roaches_list);

/*------------------- Lizard Client functions -----------------*/

Lizard_Client_List_t *create_lizard_client_list();

Lizard_Client_t *create_lizard(Board_t *board, char ch, int token);

void insert_lizard_at_end(Lizard_Client_List_t *list, Lizard_Client_t *lizard);

void remove_lizard_by_ch(Lizard_Client_List_t *list, char ch);

void sort_lizard_list(Lizard_Client_List_t *list, char ch);

char get_existent_lizard_head(Board_t *array, int list_index);

char get_existent_animal(Board_t *array, int list_index);

char get_existent_wasp(Board_t *array, int list_index);

Lizard_Client_t *find_lizard_by_ch(Lizard_Client_List_t *list, char ch);

void new_body_direction(Lizard_Client_t *lizard, direction_t movement_direction);

void draw_lizard_body(void *publisher, Board_t *board, Lizard_Client_t *lizard);

void Timeout_Lizards(void *pusher, Board_t *board, Lizard_Client_List_t *clients, int *n_lizards, Alphabet_List_t *alphabet_list);

void erase_lizard_body(void *publisher, Board_t *board, Lizard_Client_t *lizard);

void free_lizard_clients(Lizard_Client_List_t *list);

/*------------------- Alphabet functions -----------------*/

Alphabet_List_t *create_alphabet_list();

Character_t *create_character(char ch);

void insert_character(Alphabet_List_t *list, Character_t *ch);

void initialize_alphabet_list(Alphabet_List_t *alphabet_list);

char get_new_character(Alphabet_List_t *list);

void remove_character(Alphabet_List_t *list, char ch);

void free_alphabet(Alphabet_List_t *list);

/*------------------- Scores functions -----------------*/

WINDOW *create_scores_window(int x, int y);

void print_lizards_scores(void *publisher, Lizard_Client_List_t *list);
void print_lizards_scores_lf(void *publisher, Lizard_Client_List_t *list);
Lizard_Client_t *find_lizard_by_ch_update(Lizard_Client_List_t *list, char ch);
void erase_lizard_body_lf(void *publisher, Board_t *board, Lizard_Client_t *lizard);
void Timeout_Roaches(void *pusher, Board_t *board, Roach_Client_List_t *clients, Roaches_List_t *roaches, Dead_Roaches_List_t *dead_roaches, int *n_roaches);
void remove_roaches_by_client_id_lf(Roaches_List_t *list, int *n_roaches, int client_id);
void remove_roaches_from_board_lf(void *publisher, Board_t *array, Roaches_List_t *roach_list, int client_id);
void Timeout_Wasps(void *pusher, Board_t *board, Wasp_Client_List_t *clients, Wasps_List_t *wasps, int *n_wasps);
void sort_lizard_list_lf(Lizard_Client_List_t *list, char ch);
void remove_roaches_and_dead_by_client_id(Roaches_List_t *list, Dead_Roaches_List_t *dead_roaches, int *n_roaches, int client_id);
void remove_dead_roach_by_id(Dead_Roaches_List_t *dead_roaches, int id);
void remove_roaches_and_dead_by_client_id_lf(Roaches_List_t *list, Dead_Roaches_List_t *dead_roaches, int *n_roaches, int client_id);
Roach_t *find_roach_by_id_lf(Roaches_List_t *list, int id);
char get_existent_lizard_head_lf(Board_t *array, int list_index);
void new_roach_position_lf(Board_t *board, int *pos_x, int *pos_y);
char get_existent_wasp_lf(Board_t *array, int list_index);
void remove_wasps_from_board_lf(void *publisher, Board_t *array, Wasps_List_t *wasp_list, int client_id);
void remove_from_board_lf(Board_t *array, int list_index, char *ch);
void print_lizards_scores_lf(void *publisher, Lizard_Client_List_t *list);

#endif // LIZARD_N_ROACH_H