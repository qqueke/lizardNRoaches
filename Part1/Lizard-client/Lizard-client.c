#include "lizardNroach.h"
#include <unistd.h>
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

    int score;
    int sch;
    int size;

    int token = 0;
    int ch = 'c';

    void *context = zmq_ctx_new();
    if (context == NULL)
        exit(EXIT_FAILURE);

    // Create ZMQ_REQ socket
    void *requester = zmq_socket(context, ZMQ_REQ);
    if (requester == NULL)
        exit(EXIT_FAILURE);

    char endpoint[256];
    sprintf(endpoint, "tcp://%s", argv[1]);

    // Connect to the server using ZMQ_REQ
    size = zmq_connect(requester, endpoint);
    verify_status(context, requester, size);

    // Send label message
    size = zmq_send(requester, LIZARD_LABEL, strlen(LIZARD_LABEL), ZMQ_SNDMORE);
    verify_status(context, requester, size);

    srandom(time(NULL));
    // Format connection message
    Lizard_Message_t msg;
    msg.type = 0;
    msg.ch = ch;
    msg.token = token;
    msg.direction = random() % 4;

    // Send connection message
    size = zmq_send(requester, &msg, sizeof(Lizard_Message_t), 0);
    verify_status(context, requester, size);

    // Recv lizard char
    size = zmq_recv(requester, &ch, sizeof(char), 0);
    verify_status(context, requester, size);

    if (ch == '?')
    {
        printf("Server full! Not accepting any more players ...");
        verify_status(context, requester, -1);
    }

    // Recv the client token
    size = zmq_recv(requester, &token, sizeof(int), 0);
    verify_status(context, requester, size);

    // Format movement messages
    msg.type = 1;
    msg.ch = ch;
    msg.token = token;

    initscr();            /* Start curses mode 		*/
    cbreak();             /* Line buffering disabled	*/
    keypad(stdscr, TRUE); /* We get F1, F2 etc..		*/
    noecho();             /* Don't echo() while we do getch */
    curs_set(0);          /* Set cursor invisible   */

    // Initial message
    mvprintw(0, 0, "You are connected as player '%c'. You can now start moving...", ch);

    int n = 0;
    while (1)
    {

        sch = getch();
        switch (sch)
        {
        case KEY_LEFT:
            erase();
            mvprintw(0, 0, "%d Left arrow is pressed", n);
            msg.direction = LEFT;
            break;
        case KEY_RIGHT:
            erase();
            mvprintw(0, 0, "%d Right arrow is pressed", n);
            msg.direction = RIGHT;
            break;
        case KEY_DOWN:
            erase();
            mvprintw(0, 0, "%d Down arrow is pressed", n);
            msg.direction = DOWN;
            break;
        case KEY_UP:
            erase();
            mvprintw(0, 0, "%d Up arrow is pressed", n);
            msg.direction = UP;
            break;
        case 'q':
            sch = 27;
            break;
        default:
            sch = 'x';
            break;
        }

        if (sch == 27)
            break;

        n++;
        refresh(); /* Print it on to the real screen */

        // Send label message
        size = zmq_send(requester, LIZARD_LABEL, strlen(LIZARD_LABEL), ZMQ_SNDMORE);
        verify_status(context, requester, size);

        // Movement message
        size = zmq_send(requester, &msg, sizeof(Lizard_Message_t), 0);
        verify_status(context, requester, size);

        // Recv answer from server
        size = zmq_recv(requester, &score, sizeof(score), 0);
        verify_status(context, requester, size);

        // If it indeed moved, display the movement
        /* if (n == 1)
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
            move(0, 0); // Move to the beginning of each row
            clrtoeol();   // Clear from the current position to the end of the line
            mvprintw(0, 0, "%d: You moved %s", counter, move);

            n++;
            refresh();
        } */

        // Display do score
        mvprintw(1, 0, "Player '%c' score: %d", msg.ch, score);
        refresh();
    }

    endwin(); /* End curses mode*/

    msg.type = -1;
    size = zmq_send(requester, LIZARD_LABEL, strlen(LIZARD_LABEL), ZMQ_SNDMORE);
    verify_status(context, requester, size);

    size = zmq_send(requester, &msg, sizeof(Lizard_Message_t), 0);
    verify_status(context, requester, size);

    size = zmq_recv(requester, &score, sizeof(int), 0);
    verify_status(context, requester, size);

    printf("Final Score: %d\n", score);

    size = zmq_close(requester);
    if (size != 0)
        exit(EXIT_FAILURE);

    size = zmq_ctx_destroy(context);
    if (size != 0)
        exit(EXIT_FAILURE);

    return 0;
}
