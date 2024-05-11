// #include "../src/lizardNroach.h"
#include "../src/clients.h"
#include "../src/client_server.pb-c.h"
#include "../src/client_server.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>

#define N_THREADS 2

void *context, *requester, *subscriber;

WINDOW *board_win, *score_win, *in_win;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

int stop = 0;

void *thread_display(void *arg)
{
    int size;
    int l;

    DisplayType_t type;

    type = INCREMENTAL_UPDATE;
    size = zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, &type, sizeof(DisplayType_t));
    verify_status(context, subscriber, size);

    type = GAME_SNAPSHOT;
    size = zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, &type, sizeof(DisplayType_t));
    verify_status(context, subscriber, size);

    type = SCORE_SNAPSHOT;
    size = zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, &type, sizeof(DisplayType_t));
    verify_status(context, subscriber, size);

    char game_state[WINDOW_SIZE * WINDOW_SIZE];

    // Display_Message_t cursor;

    int msg_len;
    zmq_msg_t zmq_msg;
    zmq_msg_init(&zmq_msg);
    ProtoDisplayMessage *d_message;

    while (!stop)
    {
        size = zmq_recv(subscriber, &type, sizeof(DisplayType_t), 0);
        if (size == -1)
            continue;

        if (type == INCREMENTAL_UPDATE)
        {
            msg_len = zmq_recvmsg(subscriber, &zmq_msg, 0);
            if (size == -1)
                continue;

            void *msg_data = zmq_msg_data(&zmq_msg);
            d_message = proto__display__message__unpack(NULL, msg_len, msg_data);

            // Update the server display
            l = pthread_mutex_lock(&mtx);
            if (l != 0)
                exit(EXIT_FAILURE);

            wmove(board_win, d_message->pos_x, d_message->pos_y);
            waddch(board_win, d_message->ch | A_BOLD);
            wrefresh(board_win);

            l = pthread_mutex_unlock(&mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
        }
        else if (type == GAME_SNAPSHOT)
        {

            size = zmq_recv(subscriber, &game_state, (WINDOW_SIZE * WINDOW_SIZE) * sizeof(char), 0);
            if (size == -1)
                continue;

            l = pthread_mutex_lock(&mtx);
            if (l != 0)
                exit(EXIT_FAILURE);

            display_game_state(board_win, game_state);

            l = pthread_mutex_unlock(&mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
        }
        else if (type == SCORE_SNAPSHOT)
        {
            // Clear the entire window except for the first row
            l = pthread_mutex_lock(&mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
            for (int position = 1; position < SCORES_WIN_HEIGHT; position++)
            {
                wmove(score_win, position, 0); // Move to the beginning of each row
                wclrtoeol(score_win);          // Clear from the current position to the end of the line
            }
            l = pthread_mutex_unlock(&mtx);
            if (l != 0)
                exit(EXIT_FAILURE);

            // Check for additional message parts
            int more;
            size_t more_size = sizeof(more);

            int position = 1;
            do
            {
                // Receive and process the message
                char message[256];
                size = zmq_recv(subscriber, message, 255, 0);
                if (size == -1)
                    continue;

                if (size > 255)
                    size = 255;

                message[size] = '\0';

                // Check for more message parts
                zmq_getsockopt(subscriber, ZMQ_RCVMORE, &more, &more_size);

                // Do not print if it is the last message
                if (more)
                {

                    l = pthread_mutex_lock(&mtx);
                    if (l != 0)
                        exit(EXIT_FAILURE);

                    if (has_colors() && position < 4)
                        wattron(score_win, COLOR_PAIR(position));
                    mvwaddstr(score_win, position, 0, message);
                    if (has_colors() && position < 4)
                        wattroff(score_win, COLOR_PAIR(position));

                    l = pthread_mutex_unlock(&mtx);
                    if (l != 0)
                        exit(EXIT_FAILURE);
                }

                position++;
            } while (more);

            l = pthread_mutex_lock(&mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
            wrefresh(score_win);
            l = pthread_mutex_unlock(&mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
        }
    }

    return NULL;
}

void *thread_input(void *arg)
{
    int score;
    int sch;
    int size;
    int l;

    // Connect to the server using ZMQ_REQ
    // size = zmq_connect(requester, "tcp://127.0.0.1:5555");
    // verify_status(context, requester, size);

    srandom(time(NULL));

    // Send connection message
    ClientType client_type = CLIENT_TYPE__LIZARD;
    MessageType msg_type = MESSAGE_TYPE__CONNECT;
    size = zmq_send_HeaderMessage(requester, client_type, msg_type);
    verify_status(context, requester, size);
    size = zmq_send_LizardConnectReq(requester);
    verify_status(context, requester, size);

    // Recv response
    LizardConnectResp *resp = zmq_recv_LizardConnectResp(requester);

    char ch = resp->ch.data[0];
    if (ch == '!')
        verify_status(context, requester, -1);
    int token = resp->token;

    lizard__connect__resp__free_unpacked(resp, NULL);

    l = pthread_mutex_lock(&mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    mvwprintw(in_win, 0, 0, "You are connected as player '%c'.", ch);
    mvwprintw(in_win, 1, 0, "You can now start moving...");
    wrefresh(in_win);

    l = pthread_mutex_unlock(&mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    int n = 0;
    while (1)
    {
        sch = getch();

        l = pthread_mutex_lock(&mtx);
        if (l != 0)
            exit(EXIT_FAILURE);

        Direction direction;
        switch (sch)
        {
        case KEY_LEFT:
            wmove(in_win, 1, 0); // Move to the beginning of each row
            wclrtoeol(in_win);   // Clear from the current position to the end of the line
            mvwprintw(in_win, 1, 0, "%d Left arrow is pressed", n);
            direction = DIRECTION__LEFT;
            break;
        case KEY_RIGHT:
            wmove(in_win, 1, 0); // Move to the beginning of each row
            wclrtoeol(in_win);   // Clear from the current position to the end of the line
            mvwprintw(in_win, 1, 0, "%d Right arrow is pressed", n);
            direction = DIRECTION__RIGHT;
            break;
        case KEY_DOWN:
            wmove(in_win, 1, 0); // Move to the beginning of each row
            wclrtoeol(in_win);   // Clear from the current position to the end of the line
            mvwprintw(in_win, 1, 0, "%d Down arrow is pressed", n);
            direction = DIRECTION__DOWN;
            break;
        case KEY_UP:
            wmove(in_win, 1, 0); // Move to the beginning of each row
            wclrtoeol(in_win);   // Clear from the current position to the end of the line
            mvwprintw(in_win, 1, 0, "%d Up arrow is pressed", n);
            direction = DIRECTION__UP;
            break;
        case 'q':
            sch = 27;
            break;
        default:
            sch = 'x';
            break;
        }

        wrefresh(in_win); /* Print it on to the real screen */

        l = pthread_mutex_unlock(&mtx);
        if (l != 0)
            exit(EXIT_FAILURE);

        if (sch == 27)
            break;
        else if (sch == 'x')
            continue;

        n++;

        // Send movement message
        msg_type = MESSAGE_TYPE__MOVEMENT;
        size = zmq_send_HeaderMessage(requester, client_type, msg_type);
        if (size == -1)
            continue;

        size = zmq_send_LizardMovementReq(requester, ch, token, direction);
        if (size == -1)
            continue;

        // Recv answer from server
        LizardMovementResp *resp = zmq_recv_LizardMovementResp(requester);

        if (resp->resp == RESPONSE__FAILURE || resp->resp == RESPONSE__TIMEOUT)
        {
            stop = 1;
            score = -1;
            return (void *)(long)score;
        }
    }

    // Send disconnect message
    msg_type = MESSAGE_TYPE__DISCONNECT;
    size = zmq_send_HeaderMessage(requester, client_type, msg_type);
    verify_status(context, requester, size);

    size = zmq_send_LizardDisconnectReq(requester, ch, token);
    verify_status(context, requester, size);

    // Recv final
    LizardDisconnectResp *disc_resp = zmq_recv_LizardDisconnectResp(requester);
    score = disc_resp->score;
    lizard__disconnect__resp__free_unpacked(disc_resp, NULL);

    stop = 1;
    return (void *)(long)score;
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        printf("Usage: %s <address> <port REQ> <port SUB> \n", argv[0]);
        printf("Example: %s 127.0.0.1 5555 5556\n", argv[0]);
        return 1;
    }

    context = zmq_ctx_new();
    if (context == NULL)
        exit(EXIT_FAILURE);

    // Connect to the server using ZMQ_REQ
    requester = zmq_socket(context, ZMQ_REQ);
    if (requester == NULL)
        exit(EXIT_FAILURE);

    // Connect to the REQ-REP socket
    char endpointREQ[256];
    sprintf(endpointREQ, "tcp://%s:%s", argv[1], argv[2]);
    int size = zmq_connect(requester, endpointREQ);
    verify_status(context, requester, size);

    // Connect to the server using ZMQ_SUB
    subscriber = zmq_socket(context, ZMQ_SUB);
    if (subscriber == NULL)
        exit(EXIT_FAILURE);

    char endpointSUB[256];
    sprintf(endpointSUB, "tcp://%s:%s", argv[1], argv[3]);
    size = zmq_connect(subscriber, endpointSUB);
    verify_status(context, subscriber, size);


    initscr();            /* Start curses mode 		*/
    cbreak();             /* Line buffering disabled	*/
    keypad(stdscr, TRUE); /* We get F1, F2 etc..		*/
    noecho();             /* Don't echo() while we do getch */
    curs_set(0);          /* Set cursor invisible   */
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
    refresh();

    /* creates a window and draws a border */
    board_win = newwin(WINDOW_SIZE, WINDOW_SIZE, 0, 0);
    box(board_win, 0, 0);
    wrefresh(board_win);

    /* creates a scores window */
    score_win = create_scores_window(0, WINDOW_SIZE + 5);

    /* creates a window to display the inputs */
    in_win = newwin(2, 50, WINDOW_SIZE + 1, 0);
    wrefresh(in_win);

    // This will create 6 threads. 4 for lizards, 1 for bots and 1 for publishing
    long int worker_nbr;
    pthread_t worker_id[N_THREADS];
    for (worker_nbr = 0; worker_nbr < N_THREADS; worker_nbr++)
    {

        switch (worker_nbr)
        {
        case 0:
            pthread_create(&worker_id[worker_nbr], NULL, thread_display, (void *)worker_nbr);
            break;
        case 1:
            pthread_create(&worker_id[worker_nbr], NULL, thread_input, (void *)worker_nbr);
            break;
        default:
            break;
        }
    }

    void *thread_ret;
    long score;
    for (int i = 0; i < N_THREADS; i++)
    {
        pthread_join(worker_id[i], &thread_ret);
        if (i == 1)
            score = (long)thread_ret;
    }

    delwin(board_win);
    delwin(score_win);
    delwin(in_win);
    endwin(); /* End curses mode*/

    size = zmq_close(subscriber);
    if (size != 0)
        exit(EXIT_FAILURE);

    size = zmq_close(requester);
    if (size != 0)
        exit(EXIT_FAILURE);

    size = zmq_ctx_destroy(context);
    if (size != 0)
        exit(EXIT_FAILURE);

    printf("Final Score: %ld\n", score);

    return 0;
}
