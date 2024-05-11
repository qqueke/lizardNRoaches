#ifndef LIZARD_N_ROACH_H
#define LIZARD_N_ROACH_H

#include <math.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zmq.h>

#define MAX_CLIENTS 26
#define MAX_ROACHES 300
#define VICTORY_POINTS 50

#define WINDOW_SIZE 30
#define SCORES_WIN_HEIGHT (26 + 1)
#define SCORES_WIN_WIDTH 12

#define LIZARD_LABEL "LIZARD"
#define ROACH_LABEL "ROACH"
#define DISPLAY_LABEL "DISPLAY"
#define SCORE_LABEL "SCORE"

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
    int score;
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
    int roach_id;
    direction_t direction;
    int token;
} Roach_Message_t;

/*------------------- Roach_t and List structs -----------------*/

typedef struct Roach_t
{
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

/*------------------- Lizard_Client_t and List structs -----------------*/

typedef struct Lizard_Client_t
{
    char ch;
    int token;
    int pos_x;
    int pos_y;
    direction_t body_direction;
    int score;
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
    struct Roach_Client_t *next;
} Roach_Client_t;

typedef struct Roach_Client_List_t
{
    Roach_Client_t *head;
} Roach_Client_List_t;

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

void free_board(Board_t *array);

/*-------------------  Roaches functions -----------------*/

Roaches_List_t *create_roaches_list();

void new_roach_position(Board_t *board, int *pos_x, int *pos_y);

Roach_t *create_roach(Board_t *board, int id, int digit);

void insert_roach(Roaches_List_t *list, Roach_t *roach);

Roach_t *find_roach_by_id(Roaches_List_t *list, int id);

void remove_roach_by_id(Roaches_List_t *list, int id);

void free_roaches(Roaches_List_t *list);

/*------------------- Dead Roaches functions -----------------*/

Dead_Roaches_List_t *create_dead_roaches_list();

Dead_Roach_t *create_dead_roach(int id);

void insert_dead_roach(Dead_Roaches_List_t *list, int id);

void revive_roaches(WINDOW *win, void *publisher, Board_t *board, Dead_Roaches_List_t *dead_roaches, Roaches_List_t *roaches);

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

// Returns  '\0' when (out of) borders
//           ' ' when no lizard found
//           lizard_ch if found
char get_existent_lizard_head(Board_t *array, int list_index);

Lizard_Client_t *find_lizard_by_ch(Lizard_Client_List_t *list, char ch);

void new_body_direction(Lizard_Client_t *lizard, direction_t movement_direction);

void draw_lizard_body(void *publisher, WINDOW *my_win, Board_t *board, Lizard_Client_t *lizard);

void erase_lizard_body(void *publisher, WINDOW *my_win, Board_t *board, Lizard_Client_t *lizard);

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

void print_lizards_scores(WINDOW *win, void *publisher, Lizard_Client_List_t *list);

#endif // LIZARD_N_ROACH_H