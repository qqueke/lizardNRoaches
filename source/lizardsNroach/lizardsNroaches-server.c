#include "../src/lizardNroach.h"
#include "../src/client_server.pb-c.h"
#include "../src/client_server.h"
#include "../src/global.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <zmq.h>

#define N_THREADS 6

void *context;

/* No Mutex required */
WINDOW *board_win;
WINDOW *score_win;
WINDOW *animals_win;
char game_state[WINDOW_SIZE * WINDOW_SIZE];
/* ------------------ */

/* High Contention - 1 Mutex per structure required */
Board_t *board;
// pthread_mutex_t board_mtx = PTHREAD_MUTEX_INITIALIZER;

Lizard_Client_List_t *lizard_clients;
// pthread_mutex_t lizard_mtx = PTHREAD_MUTEX_INITIALIZER;

Roaches_List_t *roaches;
Dead_Roaches_List_t *dead_roaches_list;
// pthread_mutex_t roach_mtx = PTHREAD_MUTEX_INITIALIZER;
/* ------------------ */

/* Low Contention - 1 Mutex for all structures required */
Alphabet_List_t *alphabet_list;
// pthread_mutex_t alphabet_mtx = PTHREAD_MUTEX_INITIALIZER;
/* ------------------ */
int n_lizards = 0; // number of active lizard clients
int n_roaches = 0; // number of active roaches
int n_wasps = 0;   // number of active wasps

void *thread_lizards(void *ptr)
{
    long int thread_number = (long int)ptr;
    thread_number++;

    void *responder = zmq_socket(context, ZMQ_REP);
    int rc = zmq_connect(responder, "inproc://back-end");
    if (rc != 0)
        exit(EXIT_FAILURE);

    void *pusher = zmq_socket(context, ZMQ_PUSH);
    rc = zmq_connect(pusher, "inproc://xxxx");
    if (rc != 0)
        exit(EXIT_FAILURE);

    int size;
    int l;
    int token = -1;
    int list_index;
    char tmp;
    int new_x;
    int new_y;
    char existent_lizard;
    char existent_wasp;

    int average_score;

    Lizard_Client_t *lizard;
    Lizard_Client_t *lizard2;

    Character_t *character;

    DisplayType_t type;
    Display_Message_t d_message;

    // int msg_len;
    // zmq_msg_t zmq_msg;
    // zmq_msg_init(&zmq_msg);

    Lizard_Client_List_t *lizard_clients_aux = lizard_clients;

    int timeout = 1000;
    rc = zmq_setsockopt(responder, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    while (1)
    {

        Timeout_Lizards(pusher, board, lizard_clients, &n_lizards, alphabet_list);

        // Receive message from the bots
        HeaderMessage *header;

        // int timeout = 1000; // 1 second

        // // Create a poll item for the receiving socket
        // zmq_pollitem_t items[] = {{responder, 0, ZMQ_POLLIN, 0}};

        // // Poll for incoming messages with a timeout
        // int rc = zmq_poll(items, 1, timeout);

        // Message received, proceed with zmq_recv
        header = zmq_recv_HeaderMessage(responder);
        if (header == NULL)
            continue;

        ClientType client_type = header->client_type;
        MessageType msg_type = header->msg_type;

        header__message__free_unpacked(header, NULL);

        if (client_type == CLIENT_TYPE__LIZARD)
        {
            // Lizard connection message
            if (msg_type == MESSAGE_TYPE__CONNECT)
            {
                LizardConnectReq *req = zmq_recv_LizardConnectReq(responder);
                lizard__connect__req__free_unpacked(req, NULL);
                if (n_lizards >= MAX_CLIENTS)
                {
                    // Send connection answer
                    size = zmq_send_LizardConnectResp(responder, '!', -1);
                    continue;
                }

                l = pthread_mutex_lock(&num_lizards_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                n_lizards++;

                l = pthread_mutex_unlock(&num_lizards_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                char curr_ch = get_new_character(alphabet_list);
                remove_character(alphabet_list, curr_ch);

                token = random();

                lizard = create_lizard(board, curr_ch, token);

                l = pthread_mutex_lock(&lizard_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);
                lizard->time = time(NULL);

                l = pthread_mutex_unlock(&lizard_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                insert_lizard_at_end(lizard_clients, lizard);

                // Add lizard to the board
                list_index = lizard->pos_x * WINDOW_SIZE + lizard->pos_y;

                // Thread Safe function
                add_to_board(board, list_index, lizard->ch, lizard->ch);

                // Update the display cursor
                d_message.ch = lizard->ch;
                d_message.pos_x = lizard->pos_x;
                d_message.pos_y = lizard->pos_y;

                // Send lizard head to the display-app
                type = INCREMENTAL_UPDATE;
                size = zmq_send(pusher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
                if (size == -1)
                    continue;

                size = zmq_send(pusher, &d_message, sizeof(Display_Message_t), 0);
                if (size == -1)
                    continue;

                // Send Lizard body (Thread-safe)
                draw_lizard_body(pusher, board, lizard);

                // Send to all the displays a snapshot (Ignore on the ones that do not need or just process it?)
                type = GAME_SNAPSHOT;
                zmq_send(pusher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
                zmq_send(pusher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);

                // Thread-Safe
                print_lizards_scores(pusher, lizard_clients);

                // Send connection answer
                size = zmq_send_LizardConnectResp(responder, curr_ch, token);
                if (size == -1)
                    continue;
            }
            // Lizzard movement message
            else if (msg_type == MESSAGE_TYPE__MOVEMENT)
            {
                LizardMovementReq *mov_req = zmq_recv_LizardMovementReq(responder);
                if (mov_req == NULL)
                {
                    lizard__movement__req__free_unpacked(mov_req, NULL);
                    continue;
                }

                char _ch = mov_req->ch.data[0];
                int _token = mov_req->token;
                direction_t _direction = (direction_t)mov_req->direction;
                lizard__movement__req__free_unpacked(mov_req, NULL);

                lizard = find_lizard_by_ch_update(lizard_clients, _ch);

                // Lizard does not exist OR token & ch are wrong
                if (lizard == NULL || lizard->token != _token)
                {
                    size = zmq_send_LizardMovementResp(responder, RESPONSE__FAILURE);
                    continue;
                }

                l = pthread_mutex_lock(&lizard_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);
                lizard->time = time(NULL);

                l = pthread_mutex_unlock(&lizard_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                new_x = lizard->pos_x;
                new_y = lizard->pos_y;

                new_position(&new_x, &new_y, _direction);

                list_index = new_x * WINDOW_SIZE + new_y;

                // Aqui resolver conflitos de lizard heads,

                existent_lizard = get_existent_lizard_head(board, list_index);

                if (existent_lizard != ' ' && existent_lizard != lizard->ch)
                {
                    // Find collided lizard
                    lizard2 = find_lizard_by_ch(lizard_clients, existent_lizard);
                    // Erase previous lizards' bodies (Thread-safe)
                    erase_lizard_body(pusher, board, lizard);
                    erase_lizard_body(pusher, board, lizard2);

                    l = pthread_mutex_lock(&lizard_mtx);
                    if (l != 0)
                        exit(EXIT_FAILURE);

                    if (lizard2 != NULL)
                    {
                        // Update lizards' scores
                        average_score = (lizard->score + lizard2->score) / 2;
                        lizard->score = average_score;
                        lizard2->score = average_score;
                    }

                    l = pthread_mutex_unlock(&lizard_mtx);
                    if (l != 0)
                        exit(EXIT_FAILURE);

                    // Update lizard body direction
                    new_body_direction(lizard, _direction);

                    // Draw the new lizards' bodies
                    draw_lizard_body(pusher, board, lizard);
                    // Lizard-safe
                    draw_lizard_body(pusher, board, lizard2);

                    // Update server scores
                    sort_lizard_list(lizard_clients, lizard->ch);

                    l = pthread_mutex_lock(&lizard_mtx);
                    if (l != 0)
                        exit(EXIT_FAILURE);

                    if (lizard2 != NULL)
                    {
                        sort_lizard_list_lf(lizard_clients, lizard2->ch);
                    }

                    l = pthread_mutex_unlock(&lizard_mtx);
                    if (l != 0)
                        exit(EXIT_FAILURE);

                    // Aqui
                    print_lizards_scores(pusher, lizard_clients);

                    // Update display_app scores
                    // Update the lizard1 score
                    d_message.ch = lizard->ch;
                    d_message.pos_x = lizard->pos_x;
                    d_message.pos_y = lizard->pos_y;

                    type = INCREMENTAL_UPDATE;
                    size = zmq_send(pusher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
                    size = zmq_send(pusher, &d_message, sizeof(Display_Message_t), 0);
                    if (size == -1)
                        continue;

                    l = pthread_mutex_lock(&lizard_mtx);
                    if (l != 0)
                        exit(EXIT_FAILURE);

                    if (lizard2 != NULL)
                    {
                        // Update lizards' scores
                        // Update the lizard2 head display
                        d_message.ch = lizard2->ch;
                        d_message.pos_x = lizard2->pos_x;
                        d_message.pos_y = lizard2->pos_y;

                        type = INCREMENTAL_UPDATE;
                        size = zmq_send(pusher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
                        size = zmq_send(pusher, &d_message, sizeof(Display_Message_t), 0);

                        l = pthread_mutex_unlock(&lizard_mtx);
                        if (l != 0)
                            exit(EXIT_FAILURE);

                        size = zmq_send_LizardMovementResp(responder, RESPONSE__NOTHING);
                        continue;
                    }
                }

                existent_wasp = get_existent_wasp(board, list_index);

                if (existent_wasp != ' ')
                {
                    // Erase previous lizard body
                    erase_lizard_body(pusher, board, lizard);

                    l = pthread_mutex_lock(&lizard_mtx);
                    if (l != 0)
                        exit(EXIT_FAILURE);

                    // Update lizards' scores
                    int new_score = (lizard->score - 10);
                    lizard->score = new_score;

                    l = pthread_mutex_unlock(&lizard_mtx);
                    if (l != 0)
                        exit(EXIT_FAILURE);

                    if (size == -1)
                        continue;

                    // Draw the new lizards' bodies
                    draw_lizard_body(pusher, board, lizard);

                    // Update server scores
                    sort_lizard_list(lizard_clients, lizard->ch);

                    // Aqui
                    print_lizards_scores(pusher, lizard_clients);

                    // Update display_app scores
                    d_message.ch = lizard->ch;
                    d_message.pos_x = lizard->pos_x;
                    d_message.pos_y = lizard->pos_y;

                    type = INCREMENTAL_UPDATE;
                    size = zmq_send(pusher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
                    size = zmq_send(pusher, &d_message, sizeof(Display_Message_t), 0);

                    size = zmq_send_LizardMovementResp(responder, RESPONSE__NOTHING);
                    continue;
                }

                // Esta variavel vai conter o proximo char a escrever na board
                tmp = lizard->ch;
                list_index = lizard->pos_x * WINDOW_SIZE + lizard->pos_y;

                // Remove the lizard head from previous position
                remove_from_board(board, list_index, &tmp);

                // Update the display cursor
                d_message.ch = tmp;
                d_message.pos_x = lizard->pos_x;
                d_message.pos_y = lizard->pos_y;

                // Erase the previous lizard head from the window plot
                type = INCREMENTAL_UPDATE;
                size = zmq_send(pusher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
                size = zmq_send(pusher, &d_message, sizeof(Display_Message_t), 0);

                // Erase previous lizard body
                erase_lizard_body(pusher, board, lizard);

                // Eat all roaches in the tile where lizard is going
                list_index = new_x * WINDOW_SIZE + new_y;

                int roaches_digit = eat_roaches(board, list_index, roaches, dead_roaches_list);

                if (roaches_digit)
                {
                    l = pthread_mutex_lock(&lizard_mtx);
                    if (l != 0)
                        exit(EXIT_FAILURE);

                    lizard->score += roaches_digit;

                    l = pthread_mutex_unlock(&lizard_mtx);
                    if (l != 0)
                        exit(EXIT_FAILURE);

                    sort_lizard_list(lizard_clients, lizard->ch);

                    print_lizards_scores(pusher, lizard_clients);
                }

                // Update new head position
                lizard->pos_x = new_x;
                lizard->pos_y = new_y;

                // Update new body direction
                new_body_direction(lizard, _direction);

                // Aqui
                //  Draw the new lizard body
                draw_lizard_body(pusher, board, lizard);

                // Add new lizard head position to the board
                add_to_board(board, list_index, lizard->ch, lizard->ch);

                // Update the display cursor
                d_message.ch = lizard->ch;
                d_message.pos_x = lizard->pos_x;
                d_message.pos_y = lizard->pos_y;

                // Draw the new lizard head
                type = INCREMENTAL_UPDATE;
                size = zmq_send(pusher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
                size = zmq_send(pusher, &d_message, sizeof(Display_Message_t), 0);

                // Reply to the lizard client
                size = zmq_send_LizardMovementResp(responder, RESPONSE__SUCCESS);
                if (size == -1)
                    continue;
            }
            // Disconnect message
            else if (msg_type == MESSAGE_TYPE__DISCONNECT)
            {
                LizardDisconnectReq *disc_req = zmq_recv_LizardDisconnectReq(responder);
                if (disc_req == NULL)
                    continue;

                char _ch = disc_req->ch.data[0];
                int _token = disc_req->token;

                lizard__disconnect__req__free_unpacked(disc_req, NULL);

                lizard = find_lizard_by_ch_update(lizard_clients, _ch);

                // Lizard n existe ou o token esta errado
                if (lizard == NULL || lizard->token != _token)
                {
                    // Reply to the lizard client
                    size = zmq_send_LizardDisconnectResp(responder, -1);
                    continue;
                }

                l = pthread_mutex_lock(&lizard_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                lizard->time = time(NULL);

                l = pthread_mutex_unlock(&lizard_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                tmp = lizard->ch;
                list_index = lizard->pos_x * WINDOW_SIZE + lizard->pos_y;

                // Remove the lizard head from previous position
                remove_from_board(board, list_index, &tmp);

                // Update the display_app
                d_message.ch = tmp;
                d_message.pos_x = lizard->pos_x;
                d_message.pos_y = lizard->pos_y;

                //// Erase the previous lizard head from the window plot
                type = INCREMENTAL_UPDATE;
                size = zmq_send(pusher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
                size = zmq_send(pusher, &d_message, sizeof(Display_Message_t), 0);

                // Erase previous lizard body
                erase_lizard_body(pusher, board, lizard);

                // Reply to the lizard client
                l = pthread_mutex_lock(&lizard_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                int _score = lizard->score;

                l = pthread_mutex_unlock(&lizard_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                size = zmq_send_LizardMovementResp(responder, _score);
                if (size == -1)
                    continue;

                // Eliminate all traces
                // Update the available alphabet list
                character = create_character(lizard->ch);

                insert_character(alphabet_list, character);

                l = pthread_mutex_lock(&num_lizards_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                // Update the number of clients
                n_lizards--;

                l = pthread_mutex_unlock(&num_lizards_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                // Remove it from the lizards list
                remove_lizard_by_ch(lizard_clients, lizard->ch);

                // Update the scores
                print_lizards_scores(pusher, lizard_clients);
            }
        }
    }
    return NULL;
}

void *thread_bots(void *ptr)
{
    long int thread_number = (long int)ptr;
    thread_number++;

    void *responder = zmq_socket(context, ZMQ_REP);
    int rc = zmq_bind(responder, "tcp://*:5557");
    if (rc != 0)
        exit(EXIT_FAILURE);

    void *pusher = zmq_socket(context, ZMQ_PUSH);
    rc = zmq_connect(pusher, "inproc://xxxx");
    if (rc != 0)
        exit(EXIT_FAILURE);

    int size;
    // int error = -1;
    // int token;
    int list_index;
    char tmp;
    int new_x;
    int new_y;
    char existent_lizard;
    char existent_animal;
    int l;

    // int roach_answer = 0;
    int roach_client_id = 0;
    int roach_id = 0;

    // List for Roach_t Clients
    Roach_Client_List_t *roach_clients = create_roach_client_list();

    Roach_Client_t *roach_client;
    Roach_t *roach;
    // Roaches_List_t *roaches_list;

    int wasp_id = 0;
    // int wasp_answer = 0;
    int wasp_client_id = 0;

    Wasp_Client_List_t *wasp_clients = create_wasp_client_list();
    Wasp_Client_t *wasp_client;

    Wasps_List_t *wasps = create_wasps_list();
    Wasp_t *wasp;
    // Wasps_List_t *wasps_list;

    DisplayType_t type;
    Display_Message_t d_message;

    Lizard_Client_t *lizard;

    // Character_t *character;

    int timeout = 1000;
    rc = zmq_setsockopt(responder, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

    while (1)
    {

        Timeout_Roaches(pusher, board, roach_clients, roaches, dead_roaches_list, &n_roaches);

        Timeout_Wasps(pusher, board, wasp_clients, wasps, &n_wasps);

        revive_roaches(pusher, board, dead_roaches_list, roaches);

        // Receive message from the bots
        HeaderMessage *header;
        // size = zmq_recv(responder, &client_type, sizeof(ClientType_t), 0);
        // if (size == -1)
        //     continue;

        // // Set the timeout in milliseconds
        // int timeout = 1000; // 1 second

        // // Create a poll item for the receiving socket
        // zmq_pollitem_t items[] = {{responder, 0, ZMQ_POLLIN, 0}};

        // // Poll for incoming messages with a timeout
        // int rc = zmq_poll(items, 1, timeout);

        // Message received, proceed with zmq_recv
        header = zmq_recv_HeaderMessage(responder);
        if (header == NULL)
            continue;

        ClientType client_type = header->client_type;
        MessageType msg_type = header->msg_type;

        header__message__free_unpacked(header, NULL);

        if (client_type == CLIENT_TYPE__ROACH)
        {
            // Roach_t creation message
            if (msg_type == MESSAGE_TYPE__CONNECT)
            {
                RoachConnectReq *req = zmq_recv_RoachConnectReq(responder);

                l = pthread_mutex_lock(&num_roaches_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                int _n_roaches = n_roaches;

                l = pthread_mutex_unlock(&num_roaches_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                l = pthread_mutex_lock(&num_wasps_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);
                int _n_wasps = n_wasps;
                l = pthread_mutex_unlock(&num_wasps_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                if ((_n_roaches + _n_wasps) == MAX_BOTS)
                {
                    roach__connect__req__free_unpacked(req, NULL);
                    size = zmq_send_BotConnectResp(responder, NULL, 0, 0, -1);
                    continue;
                }

                int temp_num_roaches = ((int)req->n_roach_digit < (MAX_BOTS - _n_roaches - _n_wasps)) ? (int)req->n_roach_digit : (MAX_BOTS - _n_roaches - _n_wasps);
                l = pthread_mutex_lock(&num_roaches_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);
                n_roaches += temp_num_roaches;
                l = pthread_mutex_unlock(&num_roaches_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                int temp_token = random();

                // Create new entry for roach clients list
                int temp_client_id = roach_client_id++;
                roach_client = create_roach_client(temp_client_id, temp_token);
                roach_client->time = time(NULL);

                insert_roach_client(roach_clients, roach_client);

                // Create roaches
                int temp_roaches_id[temp_num_roaches];
                for (int i = 0; i < temp_num_roaches; i++)
                {
                    // Create and insert a new Roach_t object
                    roach = create_roach(board, roach_client->id, roach_id, req->roach_digit[i]);
                    temp_roaches_id[i] = roach->id;
                    roach_id++;

                    insert_roach(roaches, roach);

                    // Add roach to the board
                    list_index = roach->pos_x * WINDOW_SIZE + roach->pos_y;

                    add_to_board(board, list_index, roach->id, roach->digit);

                    // Update the display cursor
                    d_message.ch = roach->digit;
                    d_message.pos_x = roach->pos_x;
                    d_message.pos_y = roach->pos_y;

                    // Send information to the display-app
                    type = INCREMENTAL_UPDATE;
                    size = zmq_send(pusher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
                    if (size == -1)
                        continue;
                    size = zmq_send(pusher, &d_message, sizeof(Display_Message_t), 0);
                    if (size == -1)
                        continue;
                }

                roach__connect__req__free_unpacked(req, NULL);

                // Reply to the roach client
                size = zmq_send_BotConnectResp(responder, temp_roaches_id, temp_num_roaches, temp_token, temp_client_id);
                if (size == -1)
                    continue;
            }
            // Roach_t movement
            else if (msg_type == MESSAGE_TYPE__MOVEMENT)
            {
                BotMovementReq *req = zmq_recv_BotMovementReq(responder);
                int temp_client_id = req->client_id;
                int temp_token = req->token;
                int temp_roach_id = req->bot_id;
                enum _Direction temp_direction = req->direction;
                bot__movement__req__free_unpacked(req, NULL);

                roach_client = find_roach_client_by_id(roach_clients, temp_client_id);
                // Roach_client does not exist OR token & id are wrong
                if (roach_client == NULL)
                {
                    size = zmq_send_BotMovementResp(responder, RESPONSE__FAILURE);
                    continue;
                }

                else if (roach_client->token != temp_token)
                {
                    size = zmq_send_BotMovementResp(responder, RESPONSE__FAILURE);
                    continue;
                }

                roach_client->time = time(NULL);
                roach = find_roach_by_id(roaches, temp_roach_id);

                if (roach == NULL)
                {
                    size = zmq_send_BotMovementResp(responder, RESPONSE__FAILURE);
                }

                else if (!roach->is_dead)
                {
                    new_x = roach->pos_x;
                    new_y = roach->pos_y;

                    new_position(&new_x, &new_y, (direction_t)temp_direction);

                    tmp = roach->digit;
                    list_index = new_x * WINDOW_SIZE + new_y;

                    // Verificar se existe um lizard/wasp para onde queremos ir
                    existent_lizard = get_existent_lizard_head(board, list_index);

                    char existent_wasp = get_existent_wasp(board, list_index);

                    if (existent_lizard != ' ' || existent_wasp != ' ')
                    {
                        size = zmq_send_BotMovementResp(responder, RESPONSE__NOTHING);
                        continue;
                    }
                    else
                    {
                        size = zmq_send_BotMovementResp(responder, RESPONSE__SUCCESS);
                    }

                    // Remove roach from previous position in the board
                    list_index = roach->pos_x * WINDOW_SIZE + roach->pos_y;

                    remove_from_board(board, list_index, &tmp);

                    // Update the display cursor
                    d_message.ch = tmp;
                    d_message.pos_x = roach->pos_x;
                    d_message.pos_y = roach->pos_y;

                    // Erase roach from previous position
                    type = INCREMENTAL_UPDATE;
                    size = zmq_send(pusher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
                    if (size == -1)
                        continue;

                    size = zmq_send(pusher, &d_message, sizeof(Display_Message_t), 0);
                    if (size == -1)
                        continue;

                    // Update new roach position
                    roach->pos_x = new_x;
                    roach->pos_y = new_y;

                    // Add roach to its new position in the board
                    list_index = roach->pos_x * WINDOW_SIZE + roach->pos_y;

                    add_to_board(board, list_index, roach->id, roach->digit);

                    // Update the display cursor
                    d_message.ch = roach->digit;
                    d_message.pos_x = roach->pos_x;
                    d_message.pos_y = roach->pos_y;

                    // Draw the roach on its new position
                    type = INCREMENTAL_UPDATE;
                    size = zmq_send(pusher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
                    if (size == -1)
                        continue;
                    size = zmq_send(pusher, &d_message, sizeof(Display_Message_t), 0);
                    if (size == -1)
                        continue;
                }
                else
                {
                    // Send message to roach client
                    size = zmq_send_BotMovementResp(responder, RESPONSE__NOTHING);
                    if (size == -1)
                        continue;
                }
            }
            // Roach_t disconnect
            else if (msg_type == MESSAGE_TYPE__DISCONNECT)
            {
                BotDisconnectReq *req = zmq_recv_BotDisconnectReq(responder);
                int temp_client_id = req->client_id;
                int temp_token = req->token;
                bot__disconnect__req__free_unpacked(req, NULL);

                l = pthread_mutex_lock(&board_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);
                l = pthread_mutex_lock(&roach_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                roach_client = find_roach_client_by_id(roach_clients, temp_client_id);

                // Authentication
                if (roach_client == NULL)
                {
                    size = zmq_send_BotDisconnectResp(responder, RESPONSE__FAILURE);

                    l = pthread_mutex_unlock(&roach_mtx);
                    if (l != 0)
                        exit(EXIT_FAILURE);

                    l = pthread_mutex_unlock(&board_mtx);
                    if (l != 0)
                        exit(EXIT_FAILURE);

                    continue;
                }

                else if (roach_client->token != temp_token)
                {
                    size = zmq_send_BotDisconnectResp(responder, RESPONSE__FAILURE);
                    l = pthread_mutex_unlock(&roach_mtx);
                    if (l != 0)
                        exit(EXIT_FAILURE);

                    l = pthread_mutex_unlock(&board_mtx);
                    if (l != 0)
                        exit(EXIT_FAILURE);
                    continue;
                }
                roach_client->time = time(NULL);

                // Remove roaches from board
                remove_roaches_from_board_lf(pusher, board, roaches, roach_client->id);

                // Remove roaches from roaches list
                // remove_roaches_by_client_id(roaches, &n_roaches, roach_client->id);
                remove_roaches_and_dead_by_client_id_lf(roaches, dead_roaches_list, &n_roaches, roach_client->id);

                // Remove the roach client
                remove_roach_client_by_id(roach_clients, roach_client->id);

                l = pthread_mutex_unlock(&roach_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                l = pthread_mutex_unlock(&board_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                size = zmq_send_BotDisconnectResp(responder, RESPONSE__SUCCESS);

                if (size == -1)
                    continue;
            }
        }
        else if (client_type == CLIENT_TYPE__WASP)
        {
            // Wasp_t creation message
            if (msg_type == MESSAGE_TYPE__CONNECT)
            {
                WaspConnectReq *req = zmq_recv_WaspConnectReq(responder);

                l = pthread_mutex_lock(&num_roaches_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);
                int _n_roaches = n_roaches;
                l = pthread_mutex_unlock(&num_roaches_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                l = pthread_mutex_lock(&num_wasps_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);
                int _n_wasps = n_wasps;
                l = pthread_mutex_unlock(&num_wasps_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                if ((_n_roaches + _n_wasps) == MAX_BOTS)
                {
                    wasp__connect__req__free_unpacked(req, NULL);
                    size = zmq_send_BotConnectResp(responder, NULL, 0, 0, -1);
                    continue;
                }

                int temp_num_wasps = ((int)req->num_wasps < (MAX_BOTS - _n_roaches - _n_wasps)) ? (int)req->num_wasps : (MAX_BOTS - _n_roaches - _n_wasps);
                l = pthread_mutex_lock(&num_wasps_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);
                n_wasps += temp_num_wasps;
                l = pthread_mutex_unlock(&num_wasps_mtx);
                if (l != 0)
                    exit(EXIT_FAILURE);

                int temp_token = random();

                // Create new entry for wasp clients list
                int temp_client_id = wasp_client_id++;
                wasp_client = create_wasp_client(temp_client_id, temp_token);
                insert_wasp_client(wasp_clients, wasp_client);
                wasp_client->time = time(NULL);

                // Create wasps
                int temp_wasps_id[temp_num_wasps];
                for (int i = 0; i < temp_num_wasps; i++)
                {
                    // Create and insert a new Wasp_t object
                    wasp = create_wasp(board, wasp_client->id, wasp_id);
                    temp_wasps_id[i] = wasp->id;
                    wasp_id++;

                    insert_wasp(wasps, wasp);

                    // Add wasp to the board
                    list_index = wasp->pos_x * WINDOW_SIZE + wasp->pos_y;

                    add_to_board(board, list_index, wasp->id, wasp->digit);

                    // Update the display cursor
                    d_message.ch = wasp->digit;
                    d_message.pos_x = wasp->pos_x;
                    d_message.pos_y = wasp->pos_y;

                    // Send information to the display-app
                    type = INCREMENTAL_UPDATE;
                    size = zmq_send(pusher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
                    if (size == -1)
                        continue;
                    size = zmq_send(pusher, &d_message, sizeof(Display_Message_t), 0);
                    if (size == -1)
                        continue;
                }

                wasp__connect__req__free_unpacked(req, NULL);

                // Reply to the wasp client
                size = zmq_send_BotConnectResp(responder, temp_wasps_id, temp_num_wasps, temp_token, temp_client_id);
                if (size == -1)
                    continue;
            }
            // Wasp_t movement
            else if (msg_type == MESSAGE_TYPE__MOVEMENT)
            {
                BotMovementReq *req = zmq_recv_BotMovementReq(responder);
                int temp_client_id = req->client_id;
                int temp_token = req->token;
                int temp_wasp_id = req->bot_id;
                enum _Direction temp_direction = req->direction;
                bot__movement__req__free_unpacked(req, NULL);

                wasp_client = find_wasp_client_by_id(wasp_clients, temp_client_id);

                // mvprintw(wasp_client->id + 5, 0, "WTF");
                // refresh();

                // Wasp_client does not exist OR token is wrong
                if (wasp_client == NULL)
                {
                    size = zmq_send_BotMovementResp(responder, RESPONSE__FAILURE);
                    continue;
                }
                else if (wasp_client->token != temp_token)
                {
                    size = zmq_send_BotMovementResp(responder, RESPONSE__FAILURE);
                    continue;
                }
                wasp_client->time = time(NULL);

                // mvprintw(wasp_client->id + 5, 0, "WTH");
                // refresh();

                wasp = find_wasp_by_id(wasps, temp_wasp_id);

                if (wasp == NULL)
                {
                    size = zmq_send_BotMovementResp(responder, RESPONSE__FAILURE);
                    continue;
                }

                new_x = wasp->pos_x;
                new_y = wasp->pos_y;

                new_position(&new_x, &new_y, (direction_t)temp_direction);

                tmp = wasp->digit;
                list_index = new_x * WINDOW_SIZE + new_y;

                // Verificar se existe um lizard para onde queremos ir
                existent_lizard = get_existent_lizard_head(board, list_index);

                if (existent_lizard != ' ')
                {
                    // Find collided lizard
                    lizard = find_lizard_by_ch(lizard_clients, existent_lizard);
                    if (lizard == NULL)
                    {
                        size = zmq_send_BotMovementResp(responder, RESPONSE__FAILURE);
                        continue;
                    }

                    // Erase previous lizard body
                    erase_lizard_body(pusher, board, lizard);

                    l = pthread_mutex_lock(&lizard_mtx);
                    if (l != 0)
                        continue;

                    // Update lizards' scores
                    int new_score = (lizard->score - 10);
                    lizard->score = new_score;

                    l = pthread_mutex_unlock(&lizard_mtx);
                    if (l != 0)
                        continue;

                    // Draw the new lizards' bodies
                    draw_lizard_body(pusher, board, lizard);

                    // Update server scores
                    sort_lizard_list(lizard_clients, lizard->ch);

                    // Aqui
                    print_lizards_scores(pusher, lizard_clients);

                    // Update display_app scores
                    d_message.ch = lizard->ch;
                    d_message.pos_x = lizard->pos_x;
                    d_message.pos_y = lizard->pos_y;

                    type = INCREMENTAL_UPDATE;
                    size = zmq_send(pusher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
                    if (size == -1)
                        continue;
                    size = zmq_send(pusher, &d_message, sizeof(Display_Message_t), 0);
                    if (size == -1)
                        continue;
                    size = zmq_send_BotMovementResp(responder, RESPONSE__NOTHING);
                    continue;
                }

                existent_animal = get_existent_animal(board, list_index);

                if (existent_animal != ' ')
                {
                    size = zmq_send_BotMovementResp(responder, RESPONSE__NOTHING);
                    continue;
                }

                // Remove wasp from previous position in the board
                list_index = wasp->pos_x * WINDOW_SIZE + wasp->pos_y;

                remove_from_board(board, list_index, &tmp);

                // Update the display cursor
                d_message.ch = tmp;
                d_message.pos_x = wasp->pos_x;
                d_message.pos_y = wasp->pos_y;

                // Erase wasp from previous position
                type = INCREMENTAL_UPDATE;
                size = zmq_send(pusher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
                if (size == -1)
                    continue;

                size = zmq_send(pusher, &d_message, sizeof(Display_Message_t), 0);
                if (size == -1)
                    continue;

                // Update new wasp position
                wasp->pos_x = new_x;
                wasp->pos_y = new_y;

                // Add wasp to its new position in the board
                list_index = wasp->pos_x * WINDOW_SIZE + wasp->pos_y;

                add_to_board(board, list_index, wasp->id, wasp->digit);

                // Update the display cursor
                d_message.ch = wasp->digit;
                d_message.pos_x = wasp->pos_x;
                d_message.pos_y = wasp->pos_y;

                // Draw the wasp on its new position
                type = INCREMENTAL_UPDATE;
                size = zmq_send(pusher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
                if (size == -1)
                    continue;

                size = zmq_send(pusher, &d_message, sizeof(Display_Message_t), 0);
                if (size == -1)
                    continue;

                // Send message to wasp client
                size = zmq_send_BotMovementResp(responder, RESPONSE__SUCCESS);
                if (size == -1)
                    continue;
            }
            // Wasp_t disconnect
            else if (msg_type == MESSAGE_TYPE__DISCONNECT)
            {
                BotDisconnectReq *req = zmq_recv_BotDisconnectReq(responder);
                int temp_client_id = req->client_id;
                int temp_token = req->token;
                bot__disconnect__req__free_unpacked(req, NULL);

                wasp_client = find_wasp_client_by_id(wasp_clients, temp_client_id);

                // Authentication
                if (wasp_client == NULL)
                {
                    size = zmq_send_BotDisconnectResp(responder, RESPONSE__FAILURE);
                    continue;
                }
                else if (wasp_client->token != temp_token)
                {
                    size = zmq_send_BotDisconnectResp(responder, RESPONSE__FAILURE);
                    continue;
                }
                wasp_client->time = time(NULL);

                // Remove wasps from board wasps_list
                remove_wasps_from_board(pusher, board, wasps, wasp_client->id);

                // Remove wasps from wasps list
                remove_wasps_by_client_id(wasps, &n_wasps, wasp_client->id);

                // Remove the wasp client
                remove_wasp_client_by_id(wasp_clients, wasp_client->id);

                size = zmq_send_BotDisconnectResp(responder, RESPONSE__SUCCESS);
                if (size == -1)
                    continue;
            }
        }
    }

    free_roach_clients(roach_clients);
    free_wasp_clients(wasp_clients);
    free_wasps(wasps);

    return NULL;
}

void *thread_publish(void *ptr)
{

    long int thread_number = (long int)ptr;
    thread_number++;

    // Bind a ZMQ_PUB socket
    void *publisher = zmq_socket(context, ZMQ_PUB);
    if (publisher == NULL)
        exit(EXIT_FAILURE);

    int pub = zmq_bind(publisher, "tcp://*:5556");
    if (pub != 0)
        exit(EXIT_FAILURE);

    // Socket for inproc communication
    void *sock_recv = zmq_socket(context, ZMQ_PULL);
    int rc = zmq_bind(sock_recv, "inproc://xxxx");
    if (rc != 0)
        exit(EXIT_FAILURE);

    int size;
    int l;
    int last_n_lizards = 0;
    int last_n_roaches = 0;
    int last_n_wasps = 0;

    DisplayType_t type;
    Display_Message_t cursor;

    int msg_len;
    ProtoDisplayMessage proto_msg = PROTO__DISPLAY__MESSAGE__INIT;

    while (1)
    {
        size = zmq_recv(sock_recv, &type, sizeof(DisplayType_t), 0);
        if (size == -1)
            continue;

        if (type == INCREMENTAL_UPDATE)
        {
            // Receive the message from inproc
            size = zmq_recv(sock_recv, &cursor, sizeof(Display_Message_t), 0);
            if (size == -1)
                continue;

            // Pack the content in a protobuf
            proto_msg.ch = cursor.ch;
            proto_msg.pos_x = cursor.pos_x;
            proto_msg.pos_y = cursor.pos_y;

            msg_len = proto__display__message__get_packed_size(&proto_msg);
            uint8_t *msg_buf = malloc(msg_len);
            proto__display__message__pack(&proto_msg, msg_buf);

            // Update the server display
            wmove(board_win, cursor.pos_x, cursor.pos_y);
            waddch(board_win, cursor.ch | A_BOLD);
            wrefresh(board_win);

            // Forward the message to the Display Managers
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                continue;
            size = zmq_send(publisher, msg_buf, msg_len, 0);
            if (size == -1)
                continue;

            free(msg_buf);

            /**
             * Update animals window, if necessary
             */
            l = pthread_mutex_lock(&num_lizards_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
            int _n_lizards = n_lizards;
            l = pthread_mutex_unlock(&num_lizards_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);

            l = pthread_mutex_lock(&num_roaches_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
            int _n_roaches = n_roaches;
            l = pthread_mutex_unlock(&num_roaches_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);

            l = pthread_mutex_lock(&num_wasps_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
            int _n_wasps = n_wasps;
            l = pthread_mutex_unlock(&num_wasps_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);

            if (last_n_lizards != _n_lizards)
            {
                last_n_lizards = _n_lizards;
                wmove(animals_win, 1, 0);
                wclrtoeol(animals_win);
                mvwprintw(animals_win, 1, 0, "Lizards: %d", last_n_lizards);
                wrefresh(animals_win);
            }
            if (last_n_roaches != _n_roaches)
            {
                last_n_roaches = _n_roaches;
                wmove(animals_win, 2, 0);
                wclrtoeol(animals_win);
                wmove(animals_win, 3, 0);
                wclrtoeol(animals_win);
                mvwprintw(animals_win, 2, 0, "Bots: %d", last_n_roaches + last_n_wasps);
                mvwprintw(animals_win, 3, 0, "  - Roaches: %d", last_n_roaches);
                wrefresh(animals_win);
            }
            if (last_n_wasps != _n_wasps)
            {
                last_n_wasps = _n_wasps;
                wmove(animals_win, 2, 0);
                wclrtoeol(animals_win);
                wmove(animals_win, 4, 0);
                wclrtoeol(animals_win);
                mvwprintw(animals_win, 2, 0, "Bots: %d", last_n_roaches + last_n_wasps);
                mvwprintw(animals_win, 4, 0, "  - Wasps: %d", last_n_wasps);
                wrefresh(animals_win);
            }
        }
        else if (type == GAME_SNAPSHOT)
        {

            copy_game_state(board_win, game_state);
            // fill_game_state(board, board_win, game_state);

            // //Forward the message to the Display managers
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                continue;

            size = zmq_send(publisher, &game_state, (WINDOW_SIZE * WINDOW_SIZE) * sizeof(char), 0);
            if (size == -1)
                continue;
        }
        else if (type == SCORE_SNAPSHOT)
        {
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                continue;
            for (int i = 1; i < SCORES_WIN_HEIGHT; ++i)
            {
                wmove(score_win, i, 0); // Move to the beginning of each row
                wclrtoeol(score_win);   // Clear from the current position to the end of the line
            }

            int more;
            int position = 1;
            size_t more_size = sizeof(more);

            char message[256];

            do
            {
                // Receive and process the message
                size = zmq_recv(sock_recv, message, 255, 0);
                if (size == -1)
                    continue;

                if (size > 255)
                    size = 255;

                message[size] = '\0';

                // Check for more message parts
                zmq_getsockopt(sock_recv, ZMQ_RCVMORE, &more, &more_size);

                // Do not print if it is the last message
                if (more)
                {
                    if (has_colors() && position < 4)
                        wattron(score_win, COLOR_PAIR(position));

                    mvwaddstr(score_win, position, 0, message);

                    if (has_colors() && position < 4)
                        wattroff(score_win, COLOR_PAIR(position));

                    size = zmq_send(publisher, message, strlen(message), ZMQ_SNDMORE);
                    if (size == -1)
                        continue;
                }

                position++;
            } while (more);

            size = zmq_send(publisher, message, strlen(message), 0);
            if (size == -1)
                continue;

            wrefresh(score_win);
        }
    }

    return NULL;
}

int main()
{
    context = zmq_ctx_new();
    if (context == NULL)
        exit(EXIT_FAILURE);

    //  Socket facing clients
    void *router = zmq_socket(context, ZMQ_ROUTER);
    int rc = zmq_bind(router, "tcp://*:5555");
    if (rc != 0)
        exit(EXIT_FAILURE);

    //  Socket facing services
    void *dealer = zmq_socket(context, ZMQ_DEALER);
    rc = zmq_bind(dealer, "inproc://back-end");
    if (rc != 0)
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
    board_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(board_win, 0, 0);
    wrefresh(board_win);

    /* creates a scores window and a list to store them*/
    score_win = create_scores_window(0, WINDOW_SIZE + 5);

    /* creates a window to list the number of animals */
    animals_win = newwin(5, WINDOW_SIZE, WINDOW_SIZE + 2, 0);
    mvwprintw(animals_win, 0, 0, "Number of animals connected:");
    mvwprintw(animals_win, 1, 0, "Lizards: %d", n_lizards);
    mvwprintw(animals_win, 2, 0, "Bots: %d", n_roaches + n_wasps);
    mvwprintw(animals_win, 3, 0, "  - Roaches: %d", n_roaches);
    mvwprintw(animals_win, 4, 0, "  - Wasps: %d", n_wasps);
    wrefresh(animals_win);

    // Game board to store information
    board = create_board(WINDOW_SIZE * WINDOW_SIZE);

    // List for Lizard clients
    lizard_clients = create_lizard_client_list();

    // List for Roaches
    roaches = create_roaches_list();

    // List for dead Roaches
    dead_roaches_list = create_dead_roaches_list();

    // List for assignable lizard chars
    alphabet_list = create_alphabet_list();
    initialize_alphabet_list(alphabet_list);

    // Game snapshot initialization
    copy_game_state(board_win, game_state);

    srandom(time(NULL));

    // This will create 6 threads. 4 for lizards, 1 for bots and 1 for publishing
    long int worker_nbr;
    for (worker_nbr = 0; worker_nbr < N_THREADS; worker_nbr++)
    {
        pthread_t worker;

        switch (worker_nbr)
        {
        case 0:
            pthread_create(&worker, NULL, thread_publish, (void *)worker_nbr);
            break;
        case 1:
            pthread_create(&worker, NULL, thread_bots, (void *)worker_nbr);
            break;
        default:
            pthread_create(&worker, NULL, thread_lizards, (void *)worker_nbr);
            break;
        }
    }

    //  Start the proxy
    zmq_proxy(router, dealer, NULL);

    /* End curses mode */
    delwin(score_win);
    delwin(board_win);
    endwin();

    // Free memory
    free_dead_roaches(dead_roaches_list);
    free_roaches(roaches);
    free_lizard_clients(lizard_clients);
    free_board(board);

    // Close sockets and delete context
    int size = zmq_close(router);
    if (size != 0)
        exit(EXIT_FAILURE);

    size = zmq_close(dealer);
    if (size != 0)
        exit(EXIT_FAILURE);

    size = zmq_ctx_shutdown(context);
    if (size != 0)
        exit(EXIT_FAILURE);

    return 0;
}
