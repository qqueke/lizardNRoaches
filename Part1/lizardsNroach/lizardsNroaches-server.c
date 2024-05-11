#include "lizardNroach.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

int main()
{
    void *context = zmq_ctx_new();
    if (context == NULL)
        exit(EXIT_FAILURE);

    // Bind a ZMQ_REP socket
    void *responder = zmq_socket(context, ZMQ_REP);
    if (responder == NULL)
        exit(EXIT_FAILURE);

    int rep = zmq_bind(responder, "tcp://*:5555");
    if (rep != 0)
        exit(EXIT_FAILURE);

    // Bind a ZMQ_PUB socket
    void *publisher = zmq_socket(context, ZMQ_PUB);
    if (publisher == NULL)
        exit(EXIT_FAILURE);

    int pub = zmq_bind(publisher, "tcp://*:5556");

    if (pub != 0)
        exit(EXIT_FAILURE);

    // ncurses initialization
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
    if (has_colors())
    {
        // Start color mode
        start_color();

        // Define color pairs for gold, silver, and bronze
        const int COLOR_GOLD = 11;
        const int COLOR_SILVER = 12;
        const int COLOR_BRONZE = 13;
        init_color(COLOR_GOLD, 1000, 843, 0);    // Gold
        init_color(COLOR_SILVER, 743, 743, 743); // Silver
        init_color(COLOR_BRONZE, 804, 498, 196); // Bronze
        init_color(COLOR_BLACK, 0, 0, 0);        // Black

        // Initialize color pairs
        init_pair(1, COLOR_GOLD, COLOR_BLACK);   // Gold text on black background
        init_pair(2, COLOR_SILVER, COLOR_BLACK); // Silver text on black background
        init_pair(3, COLOR_BRONZE, COLOR_BLACK); // Bronze text on black background
    }

    /* creates a window and draws a border */
    WINDOW *my_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(my_win, 0, 0);
    wrefresh(my_win);

    /* creates a scores window and a list to store them*/
    WINDOW *score_win = create_scores_window(0, WINDOW_SIZE + 5);

    int size;          // message debug
    int n_clients = 0; // number of active clients
    int n_roaches = 0; // number of active roaches

    // Game board to store information
    Board_t *board = create_board(WINDOW_SIZE * WINDOW_SIZE);
    int list_index;

    // List for Lizard clients
    Lizard_Client_List_t *lizard_clients = create_lizard_client_list();
    Lizard_Client_t *lizard;
    Lizard_Client_t *lizard2;

    // List for Roach_t Clients
    Roach_Client_List_t *roach_clients = create_roach_client_list();
    Roach_Client_t *roach_client;

    // List for Roaches
    Roaches_List_t *roaches = create_roaches_list();
    Roach_t *roach;

    // List for dead Roaches
    Dead_Roaches_List_t *dead_roaches_list = create_dead_roaches_list();

    // List for assignable lizard chars
    Alphabet_List_t *alphabet_list = create_alphabet_list();
    initialize_alphabet_list(alphabet_list);
    Character_t *character;

    // Message to display
    Display_Message_t d_message;

    // Lizard and Roach_t messages
    Lizard_Message_t l_message;
    Roach_Message_t r_message;

    int roach_answer = 0;

    int roach_client_id = 0;
    int roach_id = 0;

    srandom(time(NULL));
    int token;

    char tmp;
    int new_x;
    int new_y;
    char existent_lizard;
    int average_score;

    int error = -1;

    char client_label[256];

    while (1)
    {
        // Revive roaches if needed
        revive_roaches(my_win, publisher, board, dead_roaches_list, roaches);

        // Receive client label
        size = zmq_recv(responder, client_label, 255, 0);
        if (size == -1)
            exit(EXIT_FAILURE);

        if (size > 255)
            size = 255;

        client_label[size] = '\0';

        if (strcmp(client_label, ROACH_LABEL) == 0)
        {
            size = zmq_recv(responder, &r_message, sizeof(Roach_Message_t), 0);

            if (size == -1)
                exit(EXIT_FAILURE);

            // Roach_t creation message
            if (r_message.type == 0)
            {
                roach_client = find_roach_client_by_id(roach_clients, r_message.id);

                // Authentication
                if (roach_client == NULL || roach_client->token != r_message.token)
                {
                    size = zmq_send(responder, &error, sizeof(int), 0);
                    if (size == -1)
                        exit(EXIT_FAILURE);
                    continue;
                }

                // Create and insert a new Roach_t object
                roach = create_roach(board, roach_id, r_message.roach_id);
                insert_roach(roaches, roach);

                // Add roach to the board
                list_index = roach->pos_x * WINDOW_SIZE + roach->pos_y;
                add_to_board(board, list_index, roach->id, roach->digit);

                // Update server display
                wmove(my_win, roach->pos_x, roach->pos_y);
                waddch(my_win, roach->digit | A_BOLD);
                wrefresh(my_win);

                // Update the display cursor
                d_message.ch = roach->digit;
                d_message.pos_x = roach->pos_x;
                d_message.pos_y = roach->pos_y;

                // Send information to the display-app
                size = zmq_send(publisher, DISPLAY_LABEL, strlen(DISPLAY_LABEL), ZMQ_SNDMORE);
                if (size == -1)
                    exit(EXIT_FAILURE);
                size = zmq_send(publisher, &d_message, sizeof(Display_Message_t), 0);
                if (size == -1)
                    exit(EXIT_FAILURE);

                // Reply to the roach id
                size = zmq_send(responder, &roach_id, sizeof(int), 0);
                if (size == -1)
                    exit(EXIT_FAILURE);

                roach_id++;
            }
            // Roach_t movement
            else if (r_message.type == 1)
            {
                roach_client = find_roach_client_by_id(roach_clients, r_message.id);

                // Roach_client does not exist OR token & id are wrong
                if (roach_client == NULL || !(roach_client->token == r_message.token && roach_client->id == r_message.id))
                {
                    size = zmq_send(responder, &error, sizeof(int), 0);
                    if (size == -1)
                        exit(EXIT_FAILURE);
                    continue;
                }

                roach = find_roach_by_id(roaches, r_message.roach_id);

                if (!roach->is_dead)
                {
                    new_x = roach->pos_x;
                    new_y = roach->pos_y;

                    new_position(&new_x, &new_y, r_message.direction);

                    tmp = roach->digit;
                    list_index = new_x * WINDOW_SIZE + new_y;

                    // Verificar se existe um lizard para onde queremos ir
                    existent_lizard = get_existent_lizard_head(board, list_index);

                    if (existent_lizard != ' ')
                    {
                        roach_answer = 0;
                        size = zmq_send(responder, &roach_answer, sizeof(int), 0);
                        if (size == -1)
                            exit(EXIT_FAILURE);
                        continue;
                    }

                    // Remove roach from previous position in the board
                    list_index = roach->pos_x * WINDOW_SIZE + roach->pos_y;
                    remove_from_board(board, list_index, &tmp);

                    // Update server display
                    wmove(my_win, roach->pos_x, roach->pos_y);
                    waddch(my_win, tmp | A_BOLD);
                    wrefresh(my_win);

                    // Update the display cursor
                    d_message.ch = tmp;
                    d_message.pos_x = roach->pos_x;
                    d_message.pos_y = roach->pos_y;

                    // Erase roach from previous position
                    size = zmq_send(publisher, DISPLAY_LABEL, strlen(DISPLAY_LABEL), ZMQ_SNDMORE);
                    if (size == -1)
                        exit(EXIT_FAILURE);
                    size = zmq_send(publisher, &d_message, sizeof(Display_Message_t), 0);
                    if (size == -1)
                        exit(EXIT_FAILURE);

                    // Update new roach position
                    roach->pos_x = new_x;
                    roach->pos_y = new_y;

                    // Add roach to its new position in the board
                    list_index = roach->pos_x * WINDOW_SIZE + roach->pos_y;
                    add_to_board(board, list_index, roach->id, roach->digit);

                    // Update server display
                    wmove(my_win, roach->pos_x, roach->pos_y);
                    waddch(my_win, roach->digit | A_BOLD);
                    wrefresh(my_win);

                    // Update the display cursor
                    d_message.ch = roach->digit;
                    d_message.pos_x = roach->pos_x;
                    d_message.pos_y = roach->pos_y;

                    // Draw the roach on its new position
                    size = zmq_send(publisher, DISPLAY_LABEL, strlen(DISPLAY_LABEL), ZMQ_SNDMORE);
                    if (size == -1)
                        exit(EXIT_FAILURE);
                    size = zmq_send(publisher, &d_message, sizeof(Display_Message_t), 0);
                    if (size == -1)
                        exit(EXIT_FAILURE);

                    roach_answer = 1;
                }
                else
                    roach_answer = 0;

                // Send message to roach client
                size = zmq_send(responder, &roach_answer, sizeof(int), 0);
                if (size == -1)
                    exit(EXIT_FAILURE);
            }

            // Requesting a client id and token
            else if (r_message.type == 2)
            {
                if ((n_roaches + r_message.roach_id) > MAX_ROACHES)
                {
                    size = zmq_send(responder, &error, sizeof(int), 0);
                    if (size == -1)
                        exit(EXIT_FAILURE);
                    continue;
                }

                n_roaches += r_message.roach_id;

                token = random();

                // Create new entry for roach clients list
                roach_client = create_roach_client(roach_client_id, token);
                insert_roach_client(roach_clients, roach_client);

                // Reply with id and token
                size = zmq_send(responder, &roach_client_id, sizeof(int), ZMQ_SNDMORE);
                if (size == -1)
                    exit(EXIT_FAILURE);
                size = zmq_send(responder, &token, sizeof(int), 0);
                if (size == -1)
                    exit(EXIT_FAILURE);
                roach_client_id++;
            }
        }
        else if (strcmp(client_label, LIZARD_LABEL) == 0)
        {
            size = zmq_recv(responder, &l_message, sizeof(Lizard_Message_t), 0);
            if (size == -1)
                exit(EXIT_FAILURE);

            // Lizard connection message
            if (l_message.type == 0)
            {
                if (n_clients >= MAX_CLIENTS)
                {
                    char c = '?';
                    size = zmq_send(responder, &c, sizeof(char), 0);
                    if (size == -1)
                        exit(EXIT_FAILURE);
                    continue;
                }
                n_clients++;

                char curr_ch = get_new_character(alphabet_list);
                remove_character(alphabet_list, curr_ch);

                token = random();

                lizard = create_lizard(board, curr_ch, token);

                insert_lizard_at_end(lizard_clients, lizard);

                print_lizards_scores(score_win, publisher, lizard_clients);

                // Add lizard to the board
                list_index = lizard->pos_x * WINDOW_SIZE + lizard->pos_y;
                add_to_board(board, list_index, lizard->ch, lizard->ch);

                // Update server display
                wmove(my_win, lizard->pos_x, lizard->pos_y);
                waddch(my_win, lizard->ch | A_BOLD);
                wrefresh(my_win);

                // Update the display cursor
                d_message.ch = lizard->ch;
                d_message.pos_x = lizard->pos_x;
                d_message.pos_y = lizard->pos_y;
                d_message.score = lizard->score;

                // Send information to the display-app
                size = zmq_send(publisher, DISPLAY_LABEL, strlen(DISPLAY_LABEL), ZMQ_SNDMORE);
                if (size == -1)
                    exit(EXIT_FAILURE);

                size = zmq_send(publisher, &d_message, sizeof(Display_Message_t), 0);
                if (size == -1)
                    exit(EXIT_FAILURE);

                draw_lizard_body(publisher, my_win, board, lizard);

                // Reply to the lizard client with ch, token
                size = zmq_send(responder, &curr_ch, sizeof(char), ZMQ_SNDMORE);
                if (size == -1)
                    exit(EXIT_FAILURE);

                size = zmq_send(responder, &token, sizeof(int), 0);
                if (size == -1)
                    exit(EXIT_FAILURE);
            }
            else if (l_message.type == 1)
            {
                lizard = find_lizard_by_ch(lizard_clients, l_message.ch);

                // Lizard does not exist OR token & ch are wrong
                if (lizard == NULL || !(lizard->token == l_message.token && lizard->ch == l_message.ch))
                {
                    size = zmq_send(responder, &error, sizeof(int), 0);
                    if (size == -1)
                        exit(EXIT_FAILURE);
                    continue;
                }

                new_x = lizard->pos_x;
                new_y = lizard->pos_y;

                new_position(&new_x, &new_y, l_message.direction);

                list_index = new_x * WINDOW_SIZE + new_y;

                // Aqui resolver conflitos de lizard heads,
                existent_lizard = get_existent_lizard_head(board, list_index);

                if (existent_lizard != ' ' && existent_lizard != lizard->ch)
                {
                    // Find collided lizard
                    lizard2 = find_lizard_by_ch(lizard_clients, existent_lizard);

                    // Erase previous lizards' bodies
                    erase_lizard_body(publisher, my_win, board, lizard);
                    erase_lizard_body(publisher, my_win, board, lizard2);

                    // Update lizards' scores
                    average_score = (lizard->score + lizard2->score) / 2;
                    lizard->score = average_score;
                    lizard2->score = average_score;
                    size = zmq_send(responder, &lizard->score, sizeof(int), 0);
                    if (size == -1)
                        exit(EXIT_FAILURE);

                    // Update lizard body direction
                    new_body_direction(lizard, l_message.direction);

                    // Draw the new lizards' bodies
                    draw_lizard_body(publisher, my_win, board, lizard);
                    draw_lizard_body(publisher, my_win, board, lizard2);

                    // Show the lizard head on top on server display
                    mvwaddch(my_win, lizard->pos_x, lizard->pos_y, lizard->ch | A_BOLD);
                    wrefresh(my_win);

                    // Update server scores
                    sort_lizard_list(lizard_clients, lizard->ch);
                    sort_lizard_list(lizard_clients, lizard2->ch);
                    print_lizards_scores(score_win, publisher, lizard_clients);

                    // Update display_app scores
                    // Update the lizard1 score
                    d_message.ch = lizard->ch;
                    d_message.pos_x = lizard->pos_x;
                    d_message.pos_y = lizard->pos_y;
                    d_message.score = lizard->score;
                    size = zmq_send(publisher, DISPLAY_LABEL, strlen(DISPLAY_LABEL), ZMQ_SNDMORE);
                    if (size == -1)
                        exit(EXIT_FAILURE);
                    size = zmq_send(publisher, &d_message, sizeof(Display_Message_t), 0);
                    if (size == -1)
                        exit(EXIT_FAILURE);

                    // Update the lizard2 score
                    d_message.ch = lizard2->ch;
                    d_message.pos_x = lizard2->pos_x;
                    d_message.pos_y = lizard2->pos_y;
                    d_message.score = lizard2->score;
                    size = zmq_send(publisher, DISPLAY_LABEL, strlen(DISPLAY_LABEL), ZMQ_SNDMORE);
                    if (size == -1)
                        exit(EXIT_FAILURE);
                    size = zmq_send(publisher, &d_message, sizeof(Display_Message_t), 0);
                    if (size == -1)
                        exit(EXIT_FAILURE);

                    continue;
                }

                // Esta variavel vai conter o proximo char a escrever na board
                tmp = lizard->ch;
                list_index = lizard->pos_x * WINDOW_SIZE + lizard->pos_y;

                // Remove the lizard head from previous position
                remove_from_board(board, list_index, &tmp);

                // Update server display
                wmove(my_win, lizard->pos_x, lizard->pos_y);
                waddch(my_win, tmp | A_BOLD);
                wrefresh(my_win);

                // Update the display cursor
                d_message.ch = tmp;
                d_message.pos_x = lizard->pos_x;
                d_message.pos_y = lizard->pos_y;

                // Erase the previous lizard head from the window plot
                size = zmq_send(publisher, DISPLAY_LABEL, strlen(DISPLAY_LABEL), ZMQ_SNDMORE);
                if (size == -1)
                    exit(EXIT_FAILURE);

                size = zmq_send(publisher, &d_message, sizeof(Display_Message_t), 0);
                if (size == -1)
                    exit(EXIT_FAILURE);

                // Erase previous lizard body
                erase_lizard_body(publisher, my_win, board, lizard);

                // Eat all roaches in the tile where lizard is going
                list_index = new_x * WINDOW_SIZE + new_y;
                int roaches_digit = eat_roaches(board, list_index, roaches, dead_roaches_list);

                if (roaches_digit)
                {
                    lizard->score += roaches_digit;
                    sort_lizard_list(lizard_clients, lizard->ch);
                    print_lizards_scores(score_win, publisher, lizard_clients);
                }

                // Update new head position
                lizard->pos_x = new_x;
                lizard->pos_y = new_y;

                // Update new body direction
                new_body_direction(lizard, l_message.direction);

                // Draw the new lizard body
                draw_lizard_body(publisher, my_win, board, lizard);

                // Add new lizard head position to the board
                add_to_board(board, list_index, lizard->ch, lizard->ch);

                // Update server display
                wmove(my_win, lizard->pos_x, lizard->pos_y);
                waddch(my_win, lizard->ch | A_BOLD);
                wrefresh(my_win);

                // Update the display cursor
                d_message.ch = lizard->ch;
                d_message.pos_x = lizard->pos_x;
                d_message.pos_y = lizard->pos_y;
                d_message.score = lizard->score;

                // Draw the new lizard head
                size = zmq_send(publisher, DISPLAY_LABEL, strlen(DISPLAY_LABEL), ZMQ_SNDMORE);
                if (size == -1)
                    exit(EXIT_FAILURE);
                size = zmq_send(publisher, &d_message, sizeof(Display_Message_t), 0);
                if (size == -1)
                    exit(EXIT_FAILURE);

                // Reply to the lizard client
                size = zmq_send(responder, &lizard->score, sizeof(int), 0);
                if (size == -1)
                    exit(EXIT_FAILURE);
            }
            // Disconnect message
            else if (l_message.type == -1)
            {
                lizard = find_lizard_by_ch(lizard_clients, l_message.ch);

                // Lizard n existe ou o token esta errado
                if (lizard == NULL || lizard->token != l_message.token)
                {
                    size = zmq_send(responder, &error, sizeof(int), 0);
                    if (size == -1)
                        exit(EXIT_FAILURE);
                    continue;
                }

                tmp = lizard->ch;
                list_index = lizard->pos_x * WINDOW_SIZE + lizard->pos_y;

                // Remove the lizard head from previous position
                remove_from_board(board, list_index, &tmp);

                // Update server display
                wmove(my_win, lizard->pos_x, lizard->pos_y);
                waddch(my_win, tmp | A_BOLD);
                wrefresh(my_win);

                // Update the display_app
                d_message.ch = tmp;
                d_message.pos_x = lizard->pos_x;
                d_message.pos_y = lizard->pos_y;

                //// Erase the previous lizard head from the window plot
                size = zmq_send(publisher, DISPLAY_LABEL, strlen(DISPLAY_LABEL), ZMQ_SNDMORE);
                if (size == -1)
                    exit(EXIT_FAILURE);
                size = zmq_send(publisher, &d_message, sizeof(Display_Message_t), 0);
                if (size == -1)
                    exit(EXIT_FAILURE);

                // Erase previous lizard body
                erase_lizard_body(publisher, my_win, board, lizard);

                // Reply to the lizard client
                size = zmq_send(responder, &lizard->score, sizeof(int), 0);
                if (size == -1)
                    exit(EXIT_FAILURE);

                // Eliminate all traces
                // Update the available alphabet list
                character = create_character(lizard->ch);
                insert_character(alphabet_list, character);

                // Update the number of clients
                n_clients--;

                // Remove it from the lizards list
                remove_lizard_by_ch(lizard_clients, lizard->ch);

                // Update the scores
                print_lizards_scores(score_win, publisher, lizard_clients);
            }
        }
    }

    delwin(score_win);
    delwin(my_win);
    endwin(); /* End curses mode */

    free_dead_roaches(dead_roaches_list);
    free_roaches(roaches);
    free_roach_clients(roach_clients);
    free_lizard_clients(lizard_clients);
    free_board(board);

    size = zmq_close(publisher);
    if (size != 0)
        exit(EXIT_FAILURE);

    size = zmq_close(responder);
    if (size != 0)
        exit(EXIT_FAILURE);

    size = zmq_ctx_shutdown(context);
    if (size != 0)
        exit(EXIT_FAILURE);

    return 0;
}
