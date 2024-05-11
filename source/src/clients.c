#include "clients.h"

WINDOW *create_scores_window(int x, int y)
{
    WINDOW *win = newwin(SCORES_WIN_HEIGHT, SCORES_WIN_WIDTH, x, y);
    wattron(win, A_BOLD | A_UNDERLINE);
    mvwprintw(win, 0, 0, "Scores:");
    wattroff(win, A_BOLD | A_UNDERLINE);
    wrefresh(win);
    return win;
}

void verify_status(void *context, void *socket, int size)
{
    if (size == -1)
    {
        zmq_close(socket);
        zmq_ctx_destroy(context);
        printf("Error\n");
        exit(EXIT_FAILURE);
    }
}

void display_game_state(WINDOW *win, char game_state[WINDOW_SIZE * WINDOW_SIZE])

{
    int pos_x;
    int pos_y;

    for (int i = 0; i < WINDOW_SIZE * WINDOW_SIZE; ++i)
    {
        if (i <= WINDOW_SIZE || i % WINDOW_SIZE == 0 || i % WINDOW_SIZE == WINDOW_SIZE - 1 || i >= WINDOW_SIZE * (WINDOW_SIZE - 1))
            continue;
        pos_y = i % WINDOW_SIZE;
        pos_x = (i - pos_y) / WINDOW_SIZE;

        mvwaddch(win, pos_x, pos_y, game_state[i] | A_BOLD);
    }
    wrefresh(win);
    return;
}
