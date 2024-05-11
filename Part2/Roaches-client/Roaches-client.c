// #include "../src/lizardNroach.h"
#include "../src/clients.h"
#include "../src/client_server.pb-c.h"
#include "../src/client_server.h"
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>

#define N_THREADS 2

void *context;
void *requester;

int stop = 0;

void *thread_getch()
{
    while (1)
    {
        int sch = getch();
        if (sch == 'q')
            break;
    }

    stop = 1;

    return NULL;
}

void *thread_control_roaches()
{
    int size;

    srandom(time(NULL));

    // Generate a random number of roaches
    int num_roaches = random() % 10 + 1;
    int *roaches_digit = (int *)malloc(num_roaches * sizeof(int));

    for (int i = 0; i < num_roaches; ++i)
        roaches_digit[i] = random() % 5 + 1;

    // Send connection message
    ClientType client_type = CLIENT_TYPE__ROACH;
    MessageType msg_type = MESSAGE_TYPE__CONNECT;
    size = zmq_send_HeaderMessage(requester, client_type, msg_type);
    verify_status(context, requester, size);

    size = zmq_send_RoachConnectReq(requester, roaches_digit, num_roaches);
    verify_status(context, requester, size);

    // Recv & process connection response
    BotConnectResp *resp = zmq_recv_BotConnectResp(requester);
    if (resp == NULL)
    {
        endwin();
        zmq_close(requester);
        zmq_ctx_destroy(context);
        printf("\nInternal Error.\n");
        exit(EXIT_FAILURE);
    }

    int client_id = resp->client_id;
    if (client_id == -1)
    {
        endwin();
        zmq_close(requester);
        zmq_ctx_destroy(context);
        printf("\nSorry! Server is full.\n");
        exit(EXIT_FAILURE);
    }

    int token = resp->token;

    if ((int)resp->n_bot_id != num_roaches)
    {
        num_roaches = resp->n_bot_id;
        int *temp_roaches_digit = (int *)realloc(roaches_digit, num_roaches * sizeof(int));
        if (temp_roaches_digit != NULL)
            roaches_digit = temp_roaches_digit;
    }

    int roaches_id[num_roaches];
    for (int i = 0; i < num_roaches; ++i)
    {
        roaches_id[i] = resp->bot_id[i];
    }

    bot__connect__resp__free_unpacked(resp, NULL);

    // Movements
    msg_type = MESSAGE_TYPE__MOVEMENT;
    int index;
    int sleep_delay;
    int counter = 0;
    bool timeout = false;
    while (!stop)
    {
        sleep_delay = random() % 700000;
        usleep(sleep_delay);
        index = random() % num_roaches;
        Direction direction = random() % 4;

        // Send movement message
        size = zmq_send_HeaderMessage(requester, client_type, msg_type);
        if (size == -1)
            continue;
        size = zmq_send_BotMovementReq(requester, roaches_id[index], token, direction, client_id);
        if (size == -1)
            continue;

        // Recv answer from server
        BotMovementResp *resp = zmq_recv_BotMovementResp(requester);

        // If it indeed moved, display the movement
        if (resp->resp == RESPONSE__SUCCESS)
        {
            char move[50];
            switch (direction)
            {
            case DIRECTION__LEFT:
                strcpy(move, "to the left");
                break;
            case DIRECTION__RIGHT:
                strcpy(move, "to the right");
                break;
            case DIRECTION__DOWN:
                strcpy(move, "up");
                break;
            case DIRECTION__UP:
                strcpy(move, "down");
                break;
            default:
                strcpy(move, "");
                break;
            }
            move(index, 0); // Move to the beginning of each row
            clrtoeol();     // Clear from the current position to the end of the line
            mvprintw(index, 0, "%d Roach %d: moved %s", counter, index, move);

            counter++;
            refresh();
            bot__movement__resp__free_unpacked(resp, NULL);
        }
        else if (resp->resp == RESPONSE__TIMEOUT)
        {
            timeout = true;
            bot__movement__resp__free_unpacked(resp, NULL);
            break;
        }
    }

    if (!timeout)
    {
        msg_type = MESSAGE_TYPE__DISCONNECT;
        // Send disconnect message
        size = zmq_send_HeaderMessage(requester, client_type, msg_type);
        verify_status(context, requester, size);
        size = zmq_send_BotDisconnectReq(requester, client_id, token);
        verify_status(context, requester, size);

        // Recv disconnect response
        BotDisconnectResp *resp = zmq_recv_BotDisconnectResp(requester);

        // Do something with the response ?
        bot__disconnect__resp__free_unpacked(resp, NULL);
    }

    free(roaches_digit);

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <address:port> \n", argv[0]);
        printf("Example: %s 127.0.0.1:5557 \n", argv[0]);
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
    char endpoint[256];
    sprintf(endpoint, "tcp://%s", argv[1]);
    int size = zmq_connect(requester, endpoint);
    verify_status(context, requester, size);

    printf("Connected!\n");

    initscr();            /* Start curses mode 		*/
    cbreak();             /* Line buffering disabled	*/
    keypad(stdscr, TRUE); /* We get F1, F2 etc..		*/
    noecho();             /* Don't echo() while we do getch */
    curs_set(0);          /* Set cursor invisible   */
    refresh();

    pthread_t worker_id[N_THREADS];

    pthread_create(&worker_id[0], NULL, thread_control_roaches, NULL);
    pthread_create(&worker_id[1], NULL, thread_getch, NULL);

    for (int i = 0; i < N_THREADS; i++)
        pthread_join(worker_id[i], NULL);

    endwin(); /* End curses mode*/

    size = zmq_close(requester);
    if (size != 0)
        exit(EXIT_FAILURE);

    size = zmq_ctx_destroy(context);
    if (size != 0)
        exit(EXIT_FAILURE);

    printf("Disconnected!\n");

    return 0;
}
