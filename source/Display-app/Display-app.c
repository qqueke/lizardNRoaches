#include "../src/lizardNroach.h"
#include "../src/message.pb-c.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <address:port> \n", argv[0]);
        printf("Example: %s 127.0.0.1:5556 \n", argv[0]);
        return 1;
    }

    int size;

    void *context = zmq_ctx_new();
    if (context == NULL)
        exit(EXIT_FAILURE);

    // Connect to the server using ZMQ_SUB
    void *subscriber = zmq_socket(context, ZMQ_SUB);
    if (subscriber == NULL)
        exit(EXIT_FAILURE);

    char endpoint[256];
    sprintf(endpoint, "tcp://%s", argv[1]);

    size = zmq_connect(subscriber, endpoint);
    verify_status(context, subscriber, size);

    size = zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, DISPLAY_LABEL, strlen(DISPLAY_LABEL));
    verify_status(context, subscriber, size);

    size = zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, SNAPSHOT_LABEL, strlen(SNAPSHOT_LABEL));
    verify_status(context, subscriber, size);

    size = zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, SCORE_LABEL, strlen(SCORE_LABEL));
    verify_status(context, subscriber, size);

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

    /* creates a scores window */
    WINDOW *score_win = create_scores_window(0, WINDOW_SIZE + 5);

    /* Buffer for message labels*/
    char type[256];


    char game_state [WINDOW_SIZE*WINDOW_SIZE];

    //Display_Message_t cursor;

    int msg_len;
    zmq_msg_t zmq_msg;
    zmq_msg_init (&zmq_msg);
    ProtoDisplayMessage * proto_msg;


    while (1)
    {
        size = zmq_recv(subscriber, type, 255, 0);
        verify_status(context, subscriber, size);

        if (size > 255)
            size = 255;

        type[size] = '\0';

        if (strcmp(type, DISPLAY_LABEL) == 0)
        {
            msg_len = zmq_recvmsg(subscriber, &zmq_msg, 0);
            verify_status(context, subscriber, msg_len);

            void * msg_data = zmq_msg_data (&zmq_msg);
            proto_msg = proto__display__message__unpack(NULL, msg_len, msg_data);

            // Update the server display
            wmove(my_win, proto_msg->pos_x, proto_msg->pos_y);
            waddch(my_win, proto_msg->ch | A_BOLD);
            wrefresh(my_win);
        }
        else if (strcmp(type, SNAPSHOT_LABEL) == 0)
        {

            size = zmq_recv(subscriber, &game_state, (WINDOW_SIZE*WINDOW_SIZE)*sizeof(char), 0);
            verify_status(context, subscriber, size);

            display_game_state(my_win, game_state);
        }
        else
        {
            // Clear the entire window except for the first row
            for (int position = 1; position < SCORES_WIN_HEIGHT; position++)
            {
                wmove(score_win, position, 0); // Move to the beginning of each row
                wclrtoeol(score_win);          // Clear from the current position to the end of the line
            }

            // Check for additional message parts
            int more;
            size_t more_size = sizeof(more);

            int position = 1;
            do
            {
                // Receive and process the message
                char message[256];
                size = zmq_recv(subscriber, message, 255, 0);
                verify_status(context, subscriber, size);

                if (size > 255)
                    size = 255;

                message[size] = '\0';

                // Check for more message parts
                zmq_getsockopt(subscriber, ZMQ_RCVMORE, &more, &more_size);

                // Do not print if it is the last message
                if (more)
                {
                    if (has_colors() && position < 4)
                        wattron(score_win, COLOR_PAIR(position));
                    mvwaddstr(score_win, position, 0, message);
                    if (has_colors() && position < 4)
                        wattroff(score_win, COLOR_PAIR(position));
                }

                position++;
            } while (more);

            wrefresh(score_win);
        }
    }

    delwin(my_win);
    delwin(score_win);

    endwin(); /* End curses mode */

    size = zmq_close(subscriber);
    if (size != 0)
        exit(EXIT_FAILURE);

    size = zmq_ctx_destroy(context);
    if (size != 0)
        exit(EXIT_FAILURE);
    return 0;
}
