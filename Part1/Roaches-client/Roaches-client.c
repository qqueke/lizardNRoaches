#include "lizardNroach.h"
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <address:port> \n", argv[0]);
        return 1;
    }

    int n = 0;
    int size;

    int id;
    int token;
    srandom(time(NULL));

    // char ROACH_LABEL[] = "ROACH";

    void *context = zmq_ctx_new();
    if (context == NULL)
        exit(EXIT_FAILURE);

    // Connect to the server using ZMQ_REQ
    void *requester = zmq_socket(context, ZMQ_REQ);
    if (requester == NULL)
        exit(EXIT_FAILURE);

    char endpoint[256];
    sprintf(endpoint, "tcp://%s", argv[1]);

    // Connect to the REQ-REP socket
    size = zmq_connect(requester, endpoint);
    verify_status(context, requester, size);

    // Send label message
    size = zmq_send(requester, ROACH_LABEL, strlen(ROACH_LABEL), ZMQ_SNDMORE);
    verify_status(context, requester, size);

    // Generate a random number of roaches
    int num_roaches = random() % 10 + 1;

    // Send connection message
    Roach_Message_t msg;
    msg.type = 2;
    msg.id = 0;
    msg.roach_id = num_roaches;
    msg.direction = 0;
    msg.token = 0;

    size = zmq_send(requester, &msg, sizeof(msg), 0);
    verify_status(context, requester, size);

    // Recv the client id
    size = zmq_recv(requester, &id, sizeof(n), 0);
    verify_status(context, requester, size);

    if (id == -1)
    {
        zmq_close(requester);
        zmq_ctx_destroy(context);
        printf("Server is full\n");
        exit(EXIT_FAILURE);
    }

    // Recv the client token
    size = zmq_recv(requester, &token, sizeof(n), 0);
    verify_status(context, requester, size);

    // Declare all roaches that we're going to control
    msg.type = 0;
    msg.id = id;
    msg.token = token;

    int roach_digit;
    int roaches_id[num_roaches];

    for (int i = 0; i < num_roaches; ++i)
    {
        roach_digit = random() % 5;
        roach_digit++;

        msg.roach_id = roach_digit;

        // Send label message
        size = zmq_send(requester, ROACH_LABEL, strlen(ROACH_LABEL), ZMQ_SNDMORE);
        verify_status(context, requester, size);

        // Send the roach digit
        size = zmq_send(requester, &msg, sizeof(Roach_Message_t), 0);
        verify_status(context, requester, size);

        // Recv the roach id
        size = zmq_recv(requester, &id, sizeof(n), 0);
        verify_status(context, requester, size);

        roaches_id[i] = id;
    }

    msg.type = 1;

    int index;
    int sleep_delay;

    initscr();            /* Start curses mode 		*/
    cbreak();             /* Line buffering disabled	*/
    keypad(stdscr, TRUE); /* We get F1, F2 etc..		*/
    noecho();             /* Don't echo() while we do getch */
    curs_set(0);          /* Set cursor invisible   */

    int counter = 0;
    while (1)
    {
        sleep_delay = random() % 700000;
        usleep(sleep_delay);
        index = random() % num_roaches;

        // Roach in the proper index will send movement information
        msg.roach_id = roaches_id[index];
        msg.direction = random() % 4;

        // Send label and message for movement
        size = zmq_send(requester, ROACH_LABEL, strlen(ROACH_LABEL), ZMQ_SNDMORE);
        verify_status(context, requester, size);

        size = zmq_send(requester, &msg, sizeof(Roach_Message_t), 0);
        verify_status(context, requester, size);

        // Recv answer from server
        size = zmq_recv(requester, &n, sizeof(n), 0);
        verify_status(context, requester, size);

        // If it indeed moved, display the movement
        if (n == 1)
        {
            char move[50];
            switch (msg.direction)
            {
            case LEFT:
                strcpy(move, "to the left");
                break;
            case RIGHT:
                strcpy(move, "to the right");
                break;
            case DOWN:
                strcpy(move, "up");
                break;
            case UP:
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
        }
    }

    endwin(); /* End curses mode*/

    msg.type = -1;

    size = zmq_send(requester, ROACH_LABEL, strlen(ROACH_LABEL), ZMQ_SNDMORE);
    verify_status(context, requester, size);

    size = zmq_send(requester, &msg, sizeof(Roach_Message_t), 0);
    verify_status(context, requester, size);

    size = zmq_recv(requester, &n, sizeof(int), 0);
    verify_status(context, requester, size);

    size = zmq_close(requester);
    if (size != 0)
        exit(EXIT_FAILURE);
    size = zmq_ctx_destroy(context);
    if (size != 0)
        exit(EXIT_FAILURE);

    return 0;
}
