#include "lizardNroach.h"
// extern pthread_mutex_t alphabet_mtx;

/*-------------------  Auxiliary functions -----------------*/

void new_position(int *x, int *y, direction_t direction)
{
    switch (direction)
    {
    case UP:
        (*x)--;
        if (*x == 0)
            *x = 1;
        break;
    case DOWN:
        (*x)++;
        if (*x == WINDOW_SIZE - 1)
            *x = WINDOW_SIZE - 2;
        break;
    case LEFT:
        (*y)--;
        if (*y == 0)
            *y = 1;
        break;
    case RIGHT:
        (*y)++;
        if (*y == WINDOW_SIZE - 1)
            *y = WINDOW_SIZE - 2;
        break;
    default:
        break;
    }
    return;
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

/*-------------------  Board functions -----------------*/

Board_t *create_board(int size)
{
    Board_t *array = (Board_t *)malloc(sizeof(Board_t));
    array->size = size;
    array->lists = (Board_List_t **)malloc(size * sizeof(Board_List_t *));

    for (int i = 0; i < size; ++i)
    {
        array->lists[i] = (Board_List_t *)malloc(sizeof(Board_List_t));
        array->lists[i]->head = NULL;
    }

    return array;
}

void add_to_board(Board_t *array, int list_index, int id, char ch)
{
    int l = pthread_mutex_lock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    if (!(list_index >= 0 && list_index < array->size))
    {
        printf("Error adding a new element.\n");
        l = pthread_mutex_unlock(&board_mtx);
        if (l != 0)
            exit(EXIT_FAILURE);
        return;
    }

    Board_List_t *list = array->lists[list_index];

    Board_Tile_t *node = (Board_Tile_t *)malloc(sizeof(Board_Tile_t));
    node->id = id;
    node->ch = ch;
    node->next = NULL;

    // Insert at the head
    if (list->head == NULL)
    {
        list->head = node;
    }
    else
    {
        node->next = list->head;
        list->head = node;
    }

    l = pthread_mutex_unlock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    return;
}

void add_to_board_lf(Board_t *array, int list_index, int id, char ch)
{

    if (!(list_index >= 0 && list_index < array->size))
    {
        printf("Error adding a new element.\n");

        return;
    }

    Board_List_t *list = array->lists[list_index];

    Board_Tile_t *node = (Board_Tile_t *)malloc(sizeof(Board_Tile_t));
    node->id = id;
    node->ch = ch;
    node->next = NULL;

    // Insert at the head
    if (list->head == NULL)
    {
        list->head = node;
    }
    else
    {
        node->next = list->head;
        list->head = node;
    }

    return;
}

void remove_from_board(Board_t *array, int list_index, char *ch)
{
    int l = pthread_mutex_lock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    if (!(list_index >= 0 && list_index < array->size))
    {
        printf("Error removing a new element.\n");
        l = pthread_mutex_unlock(&board_mtx);
        if (l != 0)
            exit(EXIT_FAILURE);
        return;
    }

    Board_List_t *list = array->lists[list_index];
    Board_Tile_t *current = list->head;
    Board_Tile_t *prev = NULL;

    // Search for the node with the specified character
    while (current != NULL)
    {
        if (current->ch == (*ch))
            break;

        prev = current;
        current = current->next;
    }

    // If the node is found, remove it
    if (current != NULL)
    {
        // printf("Encontrei e vou remover o '%c'\n", current->ch);
        if (prev == NULL)
        {
            // Node to be removed is the head
            // There might be other stuff ahead or not
            list->head = current->next;

            if (list->head != NULL)
                (*ch) = list->head->ch;
            else
                (*ch) = ' ';

            free(current);
            l = pthread_mutex_unlock(&board_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);

            return;
        }
        else
        {
            // Node to be removed is not the head
            prev->next = current->next;
            (*ch) = prev->ch;
            free(current);
            l = pthread_mutex_unlock(&board_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
            return;
        }
    }
    (*ch) = ' ';

    l = pthread_mutex_unlock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    return;
}

void remove_from_board_lf(Board_t *array, int list_index, char *ch)
{

    if (!(list_index >= 0 && list_index < array->size))
    {
        printf("Error removing a new element.\n");
        return;
    }

    Board_List_t *list = array->lists[list_index];
    Board_Tile_t *current = list->head;
    Board_Tile_t *prev = NULL;

    // Search for the node with the specified character
    while (current != NULL)
    {
        if (current->ch == (*ch))
            break;

        prev = current;
        current = current->next;
    }

    // If the node is found, remove it
    if (current != NULL)
    {
        // printf("Encontrei e vou remover o '%c'\n", current->ch);
        if (prev == NULL)
        {
            // Node to be removed is the head
            // There might be other stuff ahead or not
            list->head = current->next;

            if (list->head != NULL)
                (*ch) = list->head->ch;
            else
                (*ch) = ' ';

            free(current);

            return;
        }
        else
        {
            // Node to be removed is not the head
            prev->next = current->next;
            (*ch) = prev->ch;
            free(current);

            return;
        }
    }
    (*ch) = ' ';

    return;
}

void remove_roaches_from_board(void *publisher, Board_t *array, Roaches_List_t *roach_list, int client_id)
{
    Board_List_t *list;
    Board_Tile_t *current;
    Board_Tile_t *prev = NULL;

    DisplayType_t type;
    Display_Message_t d_message;

    int l = pthread_mutex_lock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    l = pthread_mutex_lock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    Roach_t *current_roach = roach_list->head;

    // Search for the node with the specified token
    while (current_roach != NULL)
    {

        if (current_roach->client_id != client_id)
        {
            current_roach = current_roach->next;
            continue;
        }

        int size;
        int list_index = current_roach->pos_x * WINDOW_SIZE + current_roach->pos_y;
        d_message.pos_x = current_roach->pos_x;
        d_message.pos_y = current_roach->pos_y;

        list = array->lists[list_index];
        current = list->head;

        while (current != NULL)
        {
            if (current->id == current_roach->id)
                break;

            prev = current;
            current = current->next;
        }

        // If the node is found, remove it
        if (current != NULL)
        {
            // printf("Encontrei e vou remover o '%c'\n", current->ch);
            if (prev == NULL)
            {
                // Node to be removed is the head
                list->head = current->next;

                if (list->head != NULL)
                    d_message.ch = list->head->ch;
                else
                    d_message.ch = ' ';

                free(current);
            }
            else
            {
                // Node to be removed is not the head
                prev->next = current->next;
                d_message.ch = prev->ch;
                free(current);
            }

            type = INCREMENTAL_UPDATE;
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                exit(EXIT_FAILURE);

            size = zmq_send(publisher, &d_message, sizeof(Display_Message_t), 0);
            if (size == -1)
                exit(EXIT_FAILURE);
        }

        current_roach = current_roach->next;
    }

    l = pthread_mutex_unlock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    l = pthread_mutex_unlock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    return;
}

void remove_roaches_from_board_lf(void *publisher, Board_t *array, Roaches_List_t *roach_list, int client_id)
{
    Board_List_t *list;
    Board_Tile_t *current;
    Board_Tile_t *prev = NULL;

    DisplayType_t type;
    Display_Message_t d_message;

    Roach_t *current_roach = roach_list->head;

    // Search for the node with the specified token
    while (current_roach != NULL)
    {

        if (current_roach->client_id != client_id)
        {
            current_roach = current_roach->next;
            continue;
        }

        int size;
        int list_index = current_roach->pos_x * WINDOW_SIZE + current_roach->pos_y;
        d_message.pos_x = current_roach->pos_x;
        d_message.pos_y = current_roach->pos_y;

        list = array->lists[list_index];
        current = list->head;

        while (current != NULL)
        {
            if (current->id == current_roach->id)
                break;

            prev = current;
            current = current->next;
        }

        // If the node is found, remove it
        if (current != NULL)
        {
            // printf("Encontrei e vou remover o '%c'\n", current->ch);
            if (prev == NULL)
            {
                // Node to be removed is the head
                list->head = current->next;

                if (list->head != NULL)
                    d_message.ch = list->head->ch;
                else
                    d_message.ch = ' ';

                free(current);
            }
            else
            {
                // Node to be removed is not the head
                prev->next = current->next;
                d_message.ch = prev->ch;
                free(current);
            }

            type = INCREMENTAL_UPDATE;
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                exit(EXIT_FAILURE);

            size = zmq_send(publisher, &d_message, sizeof(Display_Message_t), 0);
            if (size == -1)
                exit(EXIT_FAILURE);
        }

        current_roach = current_roach->next;
    }
    return;
}

void remove_wasps_from_board(void *publisher, Board_t *array, Wasps_List_t *wasp_list, int client_id)
{

    int l = pthread_mutex_lock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    Board_List_t *list;
    Board_Tile_t *current;
    Board_Tile_t *prev = NULL;

    DisplayType_t type;
    Display_Message_t d_message;

    Wasp_t *current_wasp = wasp_list->head;

    // Search for the node with the specified token
    while (current_wasp != NULL)
    {

        if (current_wasp->client_id != client_id)
        {
            current_wasp = current_wasp->next;
            continue;
        }

        int size;
        int list_index = current_wasp->pos_x * WINDOW_SIZE + current_wasp->pos_y;
        d_message.pos_x = current_wasp->pos_x;
        d_message.pos_y = current_wasp->pos_y;

        list = array->lists[list_index];
        current = list->head;

        while (current != NULL)
        {
            if (current->id == current_wasp->id)
                break;

            prev = current;
            current = current->next;
        }

        // If the node is found, remove it
        if (current != NULL)
        {
            // printf("Encontrei e vou remover o '%c'\n", current->ch);
            if (prev == NULL)
            {
                // Node to be removed is the head
                list->head = current->next;

                if (list->head != NULL)
                    d_message.ch = list->head->ch;
                else
                    d_message.ch = ' ';

                free(current);
            }
            else
            {

                // mvprintw(current_wasp->id + 5, 0, "Pointer %p\n", (void *)current->next);
                // refresh();
                // Node to be removed is not the head
                prev->next = current->next;
                d_message.ch = prev->ch;
                free(current);
            }

            type = INCREMENTAL_UPDATE;
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                exit(EXIT_FAILURE);

            size = zmq_send(publisher, &d_message, sizeof(Display_Message_t), 0);
            if (size == -1)
                exit(EXIT_FAILURE);
        }

        current_wasp = current_wasp->next;
    }

    l = pthread_mutex_unlock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    return;
}

void remove_wasps_from_board_lf(void *publisher, Board_t *array, Wasps_List_t *wasp_list, int client_id)
{

    Board_List_t *list;
    Board_Tile_t *current;
    Board_Tile_t *prev = NULL;

    DisplayType_t type;
    Display_Message_t d_message;

    Wasp_t *current_wasp = wasp_list->head;

    // Search for the node with the specified token
    while (current_wasp != NULL)
    {

        if (current_wasp->client_id != client_id)
        {
            current_wasp = current_wasp->next;
            continue;
        }

        int size;
        int list_index = current_wasp->pos_x * WINDOW_SIZE + current_wasp->pos_y;
        d_message.pos_x = current_wasp->pos_x;
        d_message.pos_y = current_wasp->pos_y;

        list = array->lists[list_index];
        current = list->head;

        while (current != NULL)
        {
            if (current->id == current_wasp->id)
                break;

            prev = current;
            current = current->next;
        }

        // If the node is found, remove it
        if (current != NULL)
        {
            // printf("Encontrei e vou remover o '%c'\n", current->ch);
            if (prev == NULL)
            {
                // Node to be removed is the head
                list->head = current->next;

                if (list->head != NULL)
                    d_message.ch = list->head->ch;
                else
                    d_message.ch = ' ';

                free(current);
            }
            else
            {

                // mvprintw(current_wasp->id + 5, 0, "Pointer %p\n", (void *)current->next);
                // refresh();
                // Node to be removed is not the head
                prev->next = current->next;
                d_message.ch = prev->ch;
                free(current);
            }

            type = INCREMENTAL_UPDATE;
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                exit(EXIT_FAILURE);

            size = zmq_send(publisher, &d_message, sizeof(Display_Message_t), 0);
            if (size == -1)
                exit(EXIT_FAILURE);
        }

        current_wasp = current_wasp->next;
    }

    return;
}

void free_board(Board_t *array)
{
    for (int i = 0; i < array->size; ++i)
    {
        Board_Tile_t *current = array->lists[i]->head;
        Board_Tile_t *next;

        while (current != NULL)
        {
            next = current->next;
            free(current);
            current = next;
        }

        free(array->lists[i]);
    }

    free(array->lists);
    free(array);
    return;
}

// Function to copy the contents of a window into a 2D array
void copy_game_state(WINDOW *win, char game_state[WINDOW_SIZE * WINDOW_SIZE])
{
    int i, pos_x, pos_y;
    for (i = 0; i < WINDOW_SIZE * WINDOW_SIZE; i++)
    {
        pos_y = i % WINDOW_SIZE;
        pos_x = i / WINDOW_SIZE;
        game_state[i] = mvwinch(win, pos_x, pos_y) & A_CHARTEXT;
    }
}

void fill_game_state(Board_t *array, WINDOW *win, char game_state[WINDOW_SIZE * WINDOW_SIZE])
{
    char ch;
    int pos_x;
    int pos_y;

    for (int i = 0; i < WINDOW_SIZE * WINDOW_SIZE; ++i)
    {
        if (array->lists[i]->head == NULL)
            ch = ' ';
        else
            ch = array->lists[i]->head->ch;

        game_state[i] = ch;
        pos_y = i % WINDOW_SIZE;
        pos_x = (i - pos_y) / WINDOW_SIZE;

        wmove(win, pos_x, pos_y);
        waddch(win, ch | A_BOLD);
        wrefresh(win);
    }
    wrefresh(win);

    return;
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

/*-------------------  Roaches functions -----------------*/

Roaches_List_t *create_roaches_list()
{
    Roaches_List_t *list = (Roaches_List_t *)malloc(sizeof(Roaches_List_t));
    if (list == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    list->head = NULL;
    return list;
}

void new_roach_position(Board_t *board, int *pos_x, int *pos_y)
{
    while (1)
    {
        *pos_x = random() % (WINDOW_SIZE - 2) + 1;
        *pos_y = random() % (WINDOW_SIZE - 2) + 1;

        int list_index = *pos_x * WINDOW_SIZE + *pos_y;

        char lizard = get_existent_lizard_head(board, list_index);

        char wasp = get_existent_wasp(board, list_index);

        // Isto significa que n existe lizard no tile
        if (lizard == ' ' && wasp == ' ')
            break;
    }
}

void new_roach_position_lf(Board_t *board, int *pos_x, int *pos_y)
{
    while (1)
    {
        *pos_x = random() % (WINDOW_SIZE - 2) + 1;
        *pos_y = random() % (WINDOW_SIZE - 2) + 1;

        int list_index = *pos_x * WINDOW_SIZE + *pos_y;

        char lizard = get_existent_lizard_head_lf(board, list_index);

        char wasp = get_existent_wasp_lf(board, list_index);

        // Isto significa que n existe lizard no tile
        if (lizard == ' ' && wasp == ' ')
            break;
    }
}

Roach_t *create_roach(Board_t *board, int client_id, int id, int digit)
{
    Roach_t *roach = (Roach_t *)malloc(sizeof(Roach_t));
    if (roach == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }

    roach->client_id = client_id;
    roach->id = id;
    roach->is_dead = false;

    // Find something better than this
    switch (digit)
    {
    case 1:
        roach->digit = '1';
        break;
    case 2:
        roach->digit = '2';
        break;
    case 3:
        roach->digit = '3';
        break;
    case 4:
        roach->digit = '4';
        break;
    case 5:
        roach->digit = '5';
        break;
    }

    int new_pos_x, new_pos_y;
    new_roach_position(board, &new_pos_x, &new_pos_y);
    roach->pos_x = new_pos_x;
    roach->pos_y = new_pos_y;

    roach->next = NULL;
    return roach;
}

void insert_roach(Roaches_List_t *list, Roach_t *roach)
{
    int l = pthread_mutex_lock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    roach->next = list->head;
    list->head = roach;
    l = pthread_mutex_unlock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    return;
}

void insert_roach_lf(Roaches_List_t *list, Roach_t *roach)
{
    roach->next = list->head;
    list->head = roach;
    return;
}

void insert_wasp_lf(Wasps_List_t *list, Wasp_t *wasp)
{
    wasp->next = list->head;
    list->head = wasp;
    return;
}

Roach_t *find_roach_by_id(Roaches_List_t *list, int id)
{

    int l = pthread_mutex_lock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    Roach_t *current = list->head;

    while (current != NULL)
    {
        if (current->id == id)
        {
            l = pthread_mutex_unlock(&roach_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
            return current;
        }
        current = current->next;
    }

    l = pthread_mutex_unlock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    return NULL;
}

Roach_t *find_roach_by_id_lf(Roaches_List_t *list, int id)
{

    Roach_t *current = list->head;

    while (current != NULL)
    {
        if (current->id == id)
        {

            return current;
        }
        current = current->next;
    }

    return NULL;
}

Roaches_List_t *find_roaches_by_client_id(Roaches_List_t *list, int client_id)
{
    Roaches_List_t *client_roaches = create_roaches_list();

    int l = pthread_mutex_lock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    Roach_t *current = list->head;

    while (current != NULL)
    {
        if (current->client_id == client_id)
        {

            Roach_t *roach = (Roach_t *)malloc(sizeof(Roach_t));
            if (roach == NULL)
            {
                printf("Memory allocation failed.\n");
                exit(1);
            }

            roach->client_id = current->client_id;
            roach->id = current->id;
            roach->digit = current->digit;
            roach->pos_x = current->pos_x;
            roach->pos_y = current->pos_y;
            roach->next = NULL;

            insert_roach_lf(client_roaches, roach);
        }
        current = current->next;
    }

    l = pthread_mutex_unlock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    return client_roaches;
}

Wasps_List_t *find_wasps_by_client_id(Wasps_List_t *list, int client_id)
{
    Wasps_List_t *client_wasps = (Wasps_List_t *)malloc(sizeof(Wasps_List_t));
    if (client_wasps == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    client_wasps->head = NULL;

    Wasp_t *current = list->head;

    while (current != NULL)
    {
        if (current->client_id == client_id)
        {
            Wasp_t *wasp = (Wasp_t *)malloc(sizeof(Wasp_t));
            if (wasp == NULL)
            {
                printf("Memory allocation failed.\n");
                exit(1);
            }

            wasp->client_id = current->client_id;
            wasp->id = current->id;
            wasp->digit = '#';
            wasp->pos_x = current->pos_x;
            wasp->pos_y = current->pos_y;
            wasp->next = NULL;

            insert_wasp_lf(client_wasps, wasp);
        }
        current = current->next;
    }

    return client_wasps;
}

void remove_roaches_by_client_id(Roaches_List_t *list, int *n_roaches, int client_id)
{

    int l = pthread_mutex_lock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    Roach_t *current = list->head;
    Roach_t *prev = NULL;

    while (current != NULL)
    {
        // Remove this node
        if (current->client_id == client_id)
        {
            // Node to be removed is the head
            if (prev == NULL)
            {
                list->head = current->next;
                free(current);
                current = list->head;
            }
            // Node to be removed is not the head
            else
            {
                prev->next = current->next;
                free(current);
                current = prev->next;
            }

            l = pthread_mutex_lock(&num_roaches_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
            *n_roaches = (*n_roaches) - 1;
            l = pthread_mutex_unlock(&num_roaches_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
        }
        else
        {
            prev = current;
            current = current->next;
        }
    }

    l = pthread_mutex_unlock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    return;
}

void remove_dead_roach_by_id(Dead_Roaches_List_t *dead_roaches, int id)
{

    // Remove the dead roaches belonging to the client
    Dead_Roach_t *current = dead_roaches->head;
    Dead_Roach_t *prev = NULL;
    while (current != NULL)
    {

        if (current->id != id)
        {
            prev = current;
            current = current->next;
            continue;
        }
        else
        {
            if (prev == NULL)
            {
                // Node to be removed is the only the head
                dead_roaches->head = current->next;
                free(current);
                current = dead_roaches->head;
            }
            else
            {
                // Node to be removed is not the head
                prev->next = current->next;
                free(current);
                current = prev->next;
            }
            break;
        }
    }
}

void remove_roaches_and_dead_by_client_id(Roaches_List_t *list, Dead_Roaches_List_t *dead_roaches, int *n_roaches, int client_id)
{

    int l = pthread_mutex_lock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    Roach_t *current = list->head;
    Roach_t *prev = NULL;

    while (current != NULL)
    {
        // Remove this node
        if (current->client_id == client_id)
        {

            if (current->is_dead)
                remove_dead_roach_by_id(dead_roaches, current->id);

            // Node to be removed is the head
            if (prev == NULL)
            {
                list->head = current->next;
                free(current);
                current = list->head;
            }
            // Node to be removed is not the head
            else
            {
                prev->next = current->next;
                free(current);
                current = prev->next;
            }

            l = pthread_mutex_lock(&num_roaches_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
            *n_roaches = (*n_roaches) - 1;
            l = pthread_mutex_unlock(&num_roaches_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
        }
        else
        {
            prev = current;
            current = current->next;
        }
    }

    l = pthread_mutex_unlock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    return;
}

void remove_roaches_and_dead_by_client_id_lf(Roaches_List_t *list, Dead_Roaches_List_t *dead_roaches, int *n_roaches, int client_id)
{

    Roach_t *current = list->head;
    Roach_t *prev = NULL;

    while (current != NULL)
    {
        // Remove this node
        if (current->client_id == client_id)
        {

            if (current->is_dead)
                remove_dead_roach_by_id(dead_roaches, current->id);

            // Node to be removed is the head
            if (prev == NULL)
            {
                list->head = current->next;
                free(current);
                current = list->head;
            }
            // Node to be removed is not the head
            else
            {
                prev->next = current->next;
                free(current);
                current = prev->next;
            }

            int l = pthread_mutex_lock(&num_roaches_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
            *n_roaches = (*n_roaches) - 1;
            l = pthread_mutex_unlock(&num_roaches_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
        }
        else
        {
            prev = current;
            current = current->next;
        }
    }

    return;
}

void remove_wasps_by_client_id(Wasps_List_t *list, int *n_wasps, int client_id)
{

    Wasp_t *current = list->head;
    Wasp_t *prev = NULL;

    while (current != NULL)
    {
        // Remove this node
        if (current->client_id == client_id)
        {
            // Node to be removed is the head
            if (prev == NULL)
            {
                list->head = current->next;
                free(current);
                current = list->head;
            }
            // Node to be removed is not the head
            else
            {
                prev->next = current->next;
                free(current);
                current = prev->next;
            }

            int l = pthread_mutex_lock(&num_wasps_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
            *n_wasps = (*n_wasps) - 1;
            l = pthread_mutex_unlock(&num_wasps_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
        }
        else
        {
            prev = current;
            current = current->next;
        }
    }

    return;
}

void remove_roach_by_id(Roaches_List_t *list, int id)
{

    int l = pthread_mutex_lock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    Roach_t *current = list->head;
    Roach_t *prev = NULL;

    // Search for the node with the specified token
    while (current != NULL && current->id != id)
    {
        prev = current;
        current = current->next;
    }

    // If the node is found, remove it
    if (current != NULL)
    {
        // Node to be removed is the head
        if (prev == NULL)
            list->head = current->next;
        // Node to be removed is not the head
        else
            prev->next = current->next;

        free(current);

        l = pthread_mutex_unlock(&roach_mtx);
        if (l != 0)
            exit(EXIT_FAILURE);
        return;
    }
    // If it is not found return
    else
    {
        l = pthread_mutex_unlock(&roach_mtx);
        if (l != 0)
            exit(EXIT_FAILURE);
        return;
    }
}

void free_roaches(Roaches_List_t *list)
{
    Roach_t *current = list->head;
    Roach_t *next;
    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }
    free(list);
    return;
}

/*-------------------  Wasps functions -----------------*/

Wasps_List_t *create_wasps_list()
{
    Wasps_List_t *list = (Wasps_List_t *)malloc(sizeof(Wasps_List_t));
    if (list == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    list->head = NULL;
    return list;
}

void new_wasp_position(Board_t *board, int *pos_x, int *pos_y)
{
    while (1)
    {
        *pos_x = random() % (WINDOW_SIZE - 2) + 1;
        *pos_y = random() % (WINDOW_SIZE - 2) + 1;

        int list_index = *pos_x * WINDOW_SIZE + *pos_y;

        char animal = get_existent_animal(board, list_index);

        // Isto significa que n existe lizard nem roach no tile
        if (animal == ' ')
            break;
    }
}

Wasp_t *create_wasp(Board_t *board, int client_id, int id)
{
    Wasp_t *wasp = (Wasp_t *)malloc(sizeof(Wasp_t));
    if (wasp == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }

    wasp->client_id = client_id;
    wasp->id = id;
    wasp->digit = '#';

    int new_pos_x, new_pos_y;
    new_wasp_position(board, &new_pos_x, &new_pos_y);
    wasp->pos_x = new_pos_x;
    wasp->pos_y = new_pos_y;
    wasp->next = NULL;
    return wasp;
}

void insert_wasp(Wasps_List_t *list, Wasp_t *wasp)
{
    wasp->next = list->head;
    list->head = wasp;
    return;
}

Wasp_t *find_wasp_by_id(Wasps_List_t *list, int id)
{
    Wasp_t *current = list->head;

    while (current != NULL)
    {
        if (current->id == id)
        {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

void remove_wasp_by_id(Wasps_List_t *list, int id)
{
    Wasp_t *current = list->head;
    Wasp_t *prev = NULL;

    // Search for the node with the specified token
    while (current != NULL && current->id != id)
    {
        prev = current;
        current = current->next;
    }

    // If the node is found, remove it
    if (current != NULL)
    {
        // Node to be removed is the head
        if (prev == NULL)
            list->head = current->next;
        // Node to be removed is not the head
        else
            prev->next = current->next;

        free(current);
        return;
    }
    // If it is not found return
    else
        return;
}

void free_wasps(Wasps_List_t *list)
{
    Wasp_t *current = list->head;
    Wasp_t *next;
    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }
    free(list);
    return;
}

/*------------------- Wasp_t client stuff -----------------*/

Wasp_Client_List_t *create_wasp_client_list()
{
    Wasp_Client_List_t *list = (Wasp_Client_List_t *)malloc(sizeof(Wasp_Client_List_t));
    if (list == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    list->head = NULL;
    return list;
}

Wasp_Client_t *create_wasp_client(int id, int token)
{
    Wasp_Client_t *wasp_client = (Wasp_Client_t *)malloc(sizeof(Wasp_Client_t));
    if (wasp_client == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    wasp_client->id = id;
    wasp_client->token = token;

    wasp_client->next = NULL;
    return wasp_client;
}

void insert_wasp_client(Wasp_Client_List_t *list, Wasp_Client_t *wasp_client)
{
    wasp_client->next = list->head;
    list->head = wasp_client;
    return;
}

Wasp_Client_t *find_wasp_client_by_id(Wasp_Client_List_t *list, int id)
{
    Wasp_Client_t *current = list->head;

    while (current != NULL)
    {
        if (current->id == id)
        {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

void free_wasp_clients(Wasp_Client_List_t *list)
{
    Wasp_Client_t *current = list->head;
    Wasp_Client_t *next;

    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }

    free(list);
    return;
}

/*------------------- Dead roach stuff -----------------*/

Dead_Roaches_List_t *create_dead_roaches_list()
{
    Dead_Roaches_List_t *list = (Dead_Roaches_List_t *)calloc(1, sizeof(Dead_Roaches_List_t));
    if (list == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    return list;
}

Dead_Roach_t *create_dead_roach(int id)
{
    Dead_Roach_t *dead_roach = (Dead_Roach_t *)malloc(sizeof(Dead_Roach_t));
    if (dead_roach == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    dead_roach->id = id;
    dead_roach->time_eaten = time(NULL);
    dead_roach->next = NULL;
    return dead_roach;
}

void insert_dead_roach(Dead_Roaches_List_t *list, int id)
{

    int l = pthread_mutex_lock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    Dead_Roach_t *droach = create_dead_roach(id);

    if (list->head == NULL)
        list->head = droach;
    else
        list->tail->next = droach;

    list->tail = droach;

    l = pthread_mutex_unlock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    return;
}

void insert_dead_roach_lf(Dead_Roaches_List_t *list, int id)
{
    Dead_Roach_t *droach = create_dead_roach(id);

    if (list->head == NULL)
        list->head = droach;
    else
        list->tail->next = droach;

    list->tail = droach;

    return;
}

void revive_roaches(void *publisher, Board_t *board, Dead_Roaches_List_t *dead_roaches, Roaches_List_t *roaches)
{
    int l;
    int id = -1;
    char digit = ' ';
    int list_index;

    l = pthread_mutex_lock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    l = pthread_mutex_lock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    Dead_Roach_t *current = dead_roaches->head;

    while (current != NULL && ((time(NULL) - current->time_eaten) >= 5))
    {

        if (current != NULL)
            id = current->id;

        Roach_t *roach = find_roach_by_id_lf(roaches, id);

        // Por alguma razao a roach ja nao existe nos roaches (vamos apenas tirar das dead_roaches)
        if (roach == NULL)
        {
            dead_roaches->head = current->next;
            free(current);
            current = dead_roaches->head;
            continue;
        }

        // Calculate new posiiton
        int new_pos_x, new_pos_y;

        new_roach_position_lf(board, &new_pos_x, &new_pos_y);

        id = roach->id;
        digit = roach->digit;
        roach->pos_x = new_pos_x;
        roach->pos_y = new_pos_y;
        roach->is_dead = false;
        list_index = roach->pos_x * WINDOW_SIZE + roach->pos_y;

        add_to_board_lf(board, list_index, id, digit);

        dead_roaches->head = current->next;
        free(current);
        current = dead_roaches->head;

        // Update the display-app
        Display_Message_t d_message;
        d_message.ch = roach->digit;
        d_message.pos_x = roach->pos_x;
        d_message.pos_y = roach->pos_y;

        // Send information to n display-app

        DisplayType_t type = INCREMENTAL_UPDATE;
        int size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
        if (size == -1)
            exit(EXIT_FAILURE);

        size = zmq_send(publisher, &d_message, sizeof(Display_Message_t), 0);
        if (size == -1)
            exit(EXIT_FAILURE);
    }

    l = pthread_mutex_unlock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    l = pthread_mutex_unlock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    return;
}

void print_lizards_scores_lf(void *publisher, Lizard_Client_List_t *list)
{
    // Clear the entire window except for the first row
    // for (int i = 1; i < SCORES_WIN_HEIGHT; ++i)
    // {
    //     wmove(win, i, 0); // Move to the beginning of each row
    //     wclrtoeol(win);   // Clear from the current position to the end of the line
    // }

    // Update display-app score display
    DisplayType_t type = SCORE_SNAPSHOT;
    int size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
    if (size == -1)
        exit(EXIT_FAILURE);

    Lizard_Client_t *current = list->head;
    int position = 1;
    while (current != NULL)
    {
        int n = (int)log10(current->score);
        int nmax = SCORES_WIN_WIDTH - 7;

        char score[256];
        if (n >= nmax)
            sprintf(score, "%2d. %c +%d", position, current->ch, (int)pow(10, nmax) - 1);
        else
            sprintf(score, "%2d. %c  %*d", position, current->ch, nmax, current->score);

        // Send score to display-app
        size = zmq_send(publisher, score, strlen(score), ZMQ_SNDMORE);
        if (size == -1)
            exit(EXIT_FAILURE);

        current = current->next;
        position++;
    }

    // Send a last message to display-app
    size = zmq_send(publisher, "", strlen(""), 0);
    if (size == -1)
        exit(EXIT_FAILURE);

    return;
}

void Timeout_Lizards(void *pusher, Board_t *board, Lizard_Client_List_t *clients, int *n_lizards, Alphabet_List_t *alphabet_list)
{

    int l = pthread_mutex_lock(&lizard_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    l = pthread_mutex_lock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    // l = pthread_mutex_lock(&lizard_mtx);
    // if (l != 0)
    //     exit(EXIT_FAILURE);

    Lizard_Client_t *current = clients->head;

    Lizard_Client_t *prev = NULL;

    Display_Message_t d_message;

    while (current != NULL)
    {

        // time_t current_time = time(NULL);

        if ((time(NULL) - current->time) < 10)
        {
            current = current->next;
            continue;
        }

        char tmp = current->ch;
        int list_index = current->pos_x * WINDOW_SIZE + current->pos_y;

        // Remove the lizard head from previous position
        remove_from_board_lf(board, list_index, &tmp);

        // Update the display_app
        d_message.ch = tmp;
        d_message.pos_x = current->pos_x;
        d_message.pos_y = current->pos_y;

        //// Erase the previous lizard head from the window plot
        DisplayType_t type = INCREMENTAL_UPDATE;
        int size = zmq_send(pusher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
        if (size == -1)
            continue;

        size = zmq_send(pusher, &d_message, sizeof(Display_Message_t), 0);
        if (size == -1)
            continue;

        // Erase previous lizard body
        erase_lizard_body_lf(pusher, board, current);

        // Update the available alphabet list
        Character_t *character = create_character(current->ch);
        insert_character(alphabet_list, character);

        // Update the number of clients
        (*n_lizards) = (*n_lizards) - 1;

        // Update the scores
        // print_lizards_scores_lf(pusher, clients);

        // Node to be removed is the head
        if (prev == NULL)
        {
            // It is also the tail
            if (clients->tail == current)
            {
                clients->head = NULL;
                clients->tail = NULL;

                free(current);
                break;
            }

            // Node to be removed is the only the head
            clients->head = current->next;
            free(current);
            current = clients->head;
        }
        else
        {
            // It is also the tail
            if (clients->tail == current)
            {
                free(current);
                break;
            }

            // Node to be removed is not the head
            prev->next = current->next;
            free(current);
            current = prev->next;
        }
    }

    print_lizards_scores_lf(pusher, clients);

    l = pthread_mutex_unlock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    l = pthread_mutex_unlock(&lizard_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    return;
}

void Timeout_Roaches(void *pusher, Board_t *board, Roach_Client_List_t *clients, Roaches_List_t *roaches, Dead_Roaches_List_t *dead_roaches, int *n_roaches)
{

    int l = pthread_mutex_lock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    l = pthread_mutex_lock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    Roach_Client_t *current = clients->head;

    Roach_Client_t *prev = NULL;

    while (current != NULL)
    {

        // time_t current_time = time(NULL);

        if ((time(NULL) - current->time) < 10)
        {
            prev = current;
            current = current->next;
            continue;
        }
        // Remove roaches from board
        // remove_roaches_from_board(pusher, board, roaches, current->id);
        remove_roaches_from_board_lf(pusher, board, roaches, current->id);

        // Remove roaches from roaches list
        // remove_roaches_by_client_id(roaches, n_roaches, current->id);
        // remove_roaches_and_dead_by_client_id(roaches, dead_roaches, n_roaches, current->id);
        remove_roaches_and_dead_by_client_id_lf(roaches, dead_roaches, n_roaches, current->id);

        // Node to be removed is the head
        if (prev == NULL)
        {
            // Node to be removed is the only the head
            clients->head = current->next;
            free(current);
            current = clients->head;
        }
        else
        {
            // Node to be removed is not the head
            prev->next = current->next;
            free(current);
            current = prev->next;
        }
    }

    l = pthread_mutex_unlock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    l = pthread_mutex_unlock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    return;
}

void Timeout_Wasps(void *pusher, Board_t *board, Wasp_Client_List_t *clients, Wasps_List_t *wasps, int *n_wasps)
{

    int l = pthread_mutex_lock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    Wasp_Client_t *current = clients->head;

    Wasp_Client_t *prev = NULL;

    while (current != NULL)
    {

        // time_t current_time = time(NULL);

        if ((time(NULL) - current->time) < 10)
        {
            prev = current;
            current = current->next;
            continue;
        }

        // Remove wasps from board
        remove_wasps_from_board_lf(pusher, board, wasps, current->id);

        // Remove wasps from wasps list
        remove_wasps_by_client_id(wasps, n_wasps, current->id);

        // Remove the wasp client
        // remove_wasp_client_by_id(clients, current->id);

        // Node to be removed is the head
        if (prev == NULL)
        {
            // Node to be removed is the only the head
            clients->head = current->next;
            free(current);
            current = clients->head;
        }
        else
        {
            // Node to be removed is not the head
            prev->next = current->next;
            free(current);
            current = prev->next;
        }
    }

    l = pthread_mutex_unlock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    return;
}

void free_dead_roaches(Dead_Roaches_List_t *list)
{
    Dead_Roach_t *current = list->head;
    Dead_Roach_t *next;
    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }
    free(list);
    return;
}

/*------------------- Roach_t client stuff -----------------*/

Roach_Client_List_t *create_roach_client_list()
{
    Roach_Client_List_t *list = (Roach_Client_List_t *)malloc(sizeof(Roach_Client_List_t));
    if (list == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    list->head = NULL;
    return list;
}

Roach_Client_t *create_roach_client(int id, int token)
{
    Roach_Client_t *roach_client = (Roach_Client_t *)malloc(sizeof(Roach_Client_t));
    if (roach_client == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    roach_client->id = id;
    roach_client->token = token;

    roach_client->next = NULL;
    return roach_client;
}

void insert_roach_client(Roach_Client_List_t *list, Roach_Client_t *roach_client)
{
    roach_client->next = list->head;
    list->head = roach_client;
    return;
}

Roach_Client_t *find_roach_client_by_id(Roach_Client_List_t *list, int id)
{
    Roach_Client_t *current = list->head;

    while (current != NULL)
    {
        if (current->id == id)
        {

            return current;
        }
        current = current->next;
    }

    return NULL;
}

// aqui
int eat_roaches(Board_t *array, int list_index, Roaches_List_t *roaches, Dead_Roaches_List_t *Dead_Roaches_list)
{
    if (!(list_index >= 0 && list_index < array->size))
    {
        printf("Error.\n");
        return ' ';
    }

    int l = pthread_mutex_lock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    l = pthread_mutex_lock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    Board_List_t *list = array->lists[list_index];
    Board_Tile_t *current = list->head;
    Board_Tile_t *prev = NULL;

    int sum = 0;

    // Search for a lizard in the position given by list_index
    while (current != NULL)
    {
        if (current->ch == '1' || current->ch == '2' || current->ch == '3' || current->ch == '4' || current->ch == '5')
        {
            int index = current->id;

            Roach_t *roach = find_roach_by_id_lf(roaches, index);

            // Pode acontecer a roach ja nao existir

            if (roach != NULL)
            {

                roach->is_dead = true;
                int _roach_id = roach->id;

                insert_dead_roach_lf(Dead_Roaches_list, _roach_id);
            }

            sum += atoi(&current->ch);
            if (prev == NULL)
            {
                // Node to be removed is the head
                list->head = current->next;
                free(current);
                current = list->head;
            }
            else
            {
                // Node to be removed is not the head
                prev->next = current->next;
                free(current);
                current = prev->next;
            }
        }
        else
        {
            current = current->next;
            prev = current;
        }
    }

    l = pthread_mutex_unlock(&roach_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    l = pthread_mutex_unlock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    return sum;
}

void remove_roach_client_by_id(Roach_Client_List_t *list, int client_id)
{
    Roach_Client_t *current = list->head;
    Roach_Client_t *prev = NULL;

    // Search for the node with the specified token
    while (current != NULL && current->id != client_id)
    {
        prev = current;
        current = current->next;
    }

    // If the node is found, remove it
    if (current != NULL)
    {
        // Node to be removed is the head
        if (prev == NULL)
            list->head = current->next;
        // Node to be removed is not the head
        else
            prev->next = current->next;

        free(current);
        return;
    }
    // If it is not found return
    else
        return;
}

void remove_wasp_client_by_id(Wasp_Client_List_t *list, int client_id)
{
    Wasp_Client_t *current = list->head;
    Wasp_Client_t *prev = NULL;

    // Search for the node with the specified token
    while (current != NULL && current->id != client_id)
    {
        prev = current;
        current = current->next;
    }

    // If the node is found, remove it
    if (current != NULL)
    {
        // Node to be removed is the head
        if (prev == NULL)
            list->head = current->next;
        // Node to be removed is not the head
        else
            prev->next = current->next;

        free(current);
        return;
    }
    // If it is not found return
    else
        return;
}

void free_roach_clients(Roach_Client_List_t *list)
{
    Roach_Client_t *current = list->head;
    Roach_Client_t *next;

    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }

    free(list);
    return;
}

/*------------------- Lizard stuff -----------------*/

Lizard_Client_List_t *create_lizard_client_list()
{
    Lizard_Client_List_t *list = (Lizard_Client_List_t *)malloc(sizeof(Lizard_Client_List_t));
    if (list == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    list->head = NULL;
    list->tail = NULL;

    return list;
}

Lizard_Client_t *create_lizard(Board_t *board, char ch, int token)
{
    Lizard_Client_t *lizard = (Lizard_Client_t *)malloc(sizeof(Lizard_Client_t));
    if (lizard == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }

    lizard->ch = ch;
    lizard->token = token;

    while (1)
    {
        lizard->pos_x = random() % (WINDOW_SIZE - 2) + 1;
        lizard->pos_y = random() % (WINDOW_SIZE - 2) + 1;

        int list_index = lizard->pos_x * WINDOW_SIZE + lizard->pos_y;

        char lizard = get_existent_lizard_head(board, list_index);

        char wasp = get_existent_wasp(board, list_index);

        // Isto significa que n existe lizard no tile
        if (lizard == ' ' && wasp == ' ')
            break;
    }

    lizard->body_direction = random() % 4;
    lizard->score = 0;
    lizard->next = NULL;
    return lizard;
}

void insert_lizard_at_end(Lizard_Client_List_t *list, Lizard_Client_t *lizard)
{

    int l = pthread_mutex_lock(&lizard_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    if (list->head == NULL)
        list->head = lizard;
    else
        list->tail->next = lizard;

    list->tail = lizard;

    l = pthread_mutex_unlock(&lizard_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    return;
}

void sort_lizard_list(Lizard_Client_List_t *list, char ch)
{
    int l = pthread_mutex_lock(&lizard_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    Lizard_Client_t *prev = NULL;
    Lizard_Client_t *current = list->head;
    Lizard_Client_t *alter;

    // Retrieve position
    while (current != NULL && current->ch != ch)
    {
        prev = current;
        current = current->next;
    }

    // No elements in the list
    if (current == NULL)
    {

        l = pthread_mutex_unlock(&lizard_mtx);
        if (l != 0)
            exit(EXIT_FAILURE);

        return;
    }
    // Lizard is already in its place
    if ((prev == NULL || prev->score >= current->score) && (current->next == NULL || current->score >= current->next->score))
    {

        l = pthread_mutex_unlock(&lizard_mtx);
        if (l != 0)
            exit(EXIT_FAILURE);

        return;
    }
    // Find its position
    else
    {
        // Take current from the list
        if (prev == NULL)
            list->head = current->next;
        else
            prev->next = current->next;

        // Verify if current is the tail
        if (current->next == NULL)
            list->tail = prev;

        // Reposition alter = current
        alter = current;
        prev = NULL;
        current = list->head;

        while (current != NULL && current->score >= alter->score)
        {
            prev = current;
            current = current->next;
        }

        if (prev == NULL)
            list->head = alter;
        else
            prev->next = alter;

        alter->next = current;

        // Verify if alter is the tail
        if (alter->next == NULL)
            list->tail = alter;

        l = pthread_mutex_unlock(&lizard_mtx);
        if (l != 0)
            exit(EXIT_FAILURE);

        return;
    }

    l = pthread_mutex_unlock(&lizard_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    return;
}

void sort_lizard_list_lf(Lizard_Client_List_t *list, char ch)
{

    Lizard_Client_t *prev = NULL;
    Lizard_Client_t *current = list->head;
    Lizard_Client_t *alter;

    // Retrieve position
    while (current != NULL && current->ch != ch)
    {
        prev = current;
        current = current->next;
    }

    // No elements in the list
    if (current == NULL)
    {
        return;
    }
    // Lizard is already in its place
    if ((prev == NULL || prev->score >= current->score) && (current->next == NULL || current->score >= current->next->score))
    {

        return;
    }
    // Find its position
    else
    {
        // Take current from the list
        if (prev == NULL)
            list->head = current->next;
        else
            prev->next = current->next;

        // Verify if current is the tail
        if (current->next == NULL)
            list->tail = prev;

        // Reposition alter = current
        alter = current;
        prev = NULL;
        current = list->head;

        while (current != NULL && current->score >= alter->score)
        {
            prev = current;
            current = current->next;
        }

        if (prev == NULL)
            list->head = alter;
        else
            prev->next = alter;

        alter->next = current;

        // Verify if alter is the tail
        if (alter->next == NULL)
            list->tail = alter;

        return;
    }

    return;
}

Lizard_Client_t *find_lizard_by_ch(Lizard_Client_List_t *list, char ch)
{
    int l = pthread_mutex_lock(&lizard_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    Lizard_Client_t *current = list->head;

    while (current != NULL)
    {
        if (current->ch == ch)
        {
            l = pthread_mutex_unlock(&lizard_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
            return current;
        }
        current = current->next;
    }
    l = pthread_mutex_unlock(&lizard_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    return NULL;
}

Lizard_Client_t *find_lizard_by_ch_update(Lizard_Client_List_t *list, char ch)
{
    int l = pthread_mutex_lock(&lizard_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    Lizard_Client_t *current = list->head;

    while (current != NULL)
    {
        if (current->ch == ch)
        {
            current->time = time(NULL);
            l = pthread_mutex_unlock(&lizard_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
            return current;
        }
        current = current->next;
    }
    l = pthread_mutex_unlock(&lizard_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    return NULL;
}

char get_existent_lizard_head(Board_t *array, int list_index)
{
    if (list_index <= WINDOW_SIZE || list_index % WINDOW_SIZE == 0 || list_index % WINDOW_SIZE == WINDOW_SIZE - 1 || list_index >= array->size - WINDOW_SIZE)
    {
        return '\0';
    }
    int l = pthread_mutex_lock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    Board_Tile_t *current = array->lists[list_index]->head;

    // Search for a lizard in the position given by list_index
    while (current != NULL)
    {
        if (current == NULL)
            break;

        if (current->ch >= 'A' && current->ch <= 'Z')
        {
            char ch = current->ch;
            l = pthread_mutex_unlock(&board_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);

            return ch;
        }
        current = current->next;
    }
    l = pthread_mutex_unlock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    return ' ';
}

char get_existent_lizard_head_lf(Board_t *array, int list_index)
{
    if (list_index <= WINDOW_SIZE || list_index % WINDOW_SIZE == 0 || list_index % WINDOW_SIZE == WINDOW_SIZE - 1 || list_index >= array->size - WINDOW_SIZE)
    {
        return '\0';
    }

    Board_Tile_t *current = array->lists[list_index]->head;

    // Search for a lizard in the position given by list_index
    while (current != NULL)
    {
        if (current == NULL)
        {

            list_index++;
            list_index--;
            break;
        }
        if (current->ch >= 'A' && current->ch <= 'Z')
        {

            return current->ch;
        }
        current = current->next;
    }

    return ' ';
}

char get_existent_animal(Board_t *array, int list_index)
{
    if (list_index <= WINDOW_SIZE || list_index % WINDOW_SIZE == 0 || list_index % WINDOW_SIZE == WINDOW_SIZE - 1 || list_index >= array->size - WINDOW_SIZE)
    {
        return '\0';
    }
    int l = pthread_mutex_lock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    Board_Tile_t *current = array->lists[list_index]->head;

    // Search for a lizard in the position given by list_index
    while (current != NULL)
    {
        if (current->ch != ' ' && current->ch != '\0' && current->ch != '*' && current->ch != '.')
        {
            l = pthread_mutex_unlock(&board_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
            return current->ch;
        }

        current = current->next;
    }
    l = pthread_mutex_unlock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    return ' ';
}

char get_existent_wasp(Board_t *array, int list_index)
{
    if (list_index <= WINDOW_SIZE || list_index % WINDOW_SIZE == 0 || list_index % WINDOW_SIZE == WINDOW_SIZE - 1 || list_index >= array->size - WINDOW_SIZE)
    {
        return '\0';
    }
    int l = pthread_mutex_lock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    Board_Tile_t *current = array->lists[list_index]->head;

    // Search for a lizard in the position given by list_index
    while (current != NULL)
    {
        if (current->ch == '#')
        {
            l = pthread_mutex_unlock(&board_mtx);
            if (l != 0)
                exit(EXIT_FAILURE);
            return current->ch;
        }
        current = current->next;
    }
    l = pthread_mutex_unlock(&board_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    return ' ';
}

char get_existent_wasp_lf(Board_t *array, int list_index)
{
    if (list_index <= WINDOW_SIZE || list_index % WINDOW_SIZE == 0 || list_index % WINDOW_SIZE == WINDOW_SIZE - 1 || list_index >= array->size - WINDOW_SIZE)
    {
        return '\0';
    }

    Board_Tile_t *current = array->lists[list_index]->head;

    // Search for a lizard in the position given by list_index
    while (current != NULL)
    {
        if (current->ch == '#')
        {

            return current->ch;
        }
        current = current->next;
    }

    return ' ';
}

void remove_lizard_by_ch(Lizard_Client_List_t *list, char ch)
{

    int l = pthread_mutex_lock(&lizard_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    Lizard_Client_t *current = list->head;
    Lizard_Client_t *prev = NULL;

    // Search for the node with the specified character
    while (current != NULL && current->ch != ch)
    {
        prev = current;
        current = current->next;
    }

    // If the node is found, remove it
    if (current != NULL)
    {
        // Node to be removed is the head
        if (prev == NULL)
            list->head = current->next;
        // Node to be removed is not the head
        else
            prev->next = current->next;

        // Node to be removed is the tail
        if (list->tail == current)
            list->tail = prev;

        free(current);
    }

    l = pthread_mutex_unlock(&lizard_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    return;
}

void new_body_direction(Lizard_Client_t *lizard, direction_t movement_direction)
{
    switch (movement_direction)
    {
    case UP:
        lizard->body_direction = DOWN;
        break;
    case DOWN:
        lizard->body_direction = UP;
        break;
    case LEFT:
        lizard->body_direction = RIGHT;
        break;
    case RIGHT:
        lizard->body_direction = LEFT;
        break;
    default:
        break;
    }
    return;
}

void draw_lizard_body(void *publisher, Board_t *board, Lizard_Client_t *lizard)
{

    int l = pthread_mutex_lock(&lizard_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    if (lizard == NULL)
    {
        l = pthread_mutex_unlock(&lizard_mtx);
        if (l != 0)
            exit(EXIT_FAILURE);
        return;
    }
    if (lizard->score < 0)
    {
        l = pthread_mutex_unlock(&lizard_mtx);
        if (l != 0)
            exit(EXIT_FAILURE);
        return;
    }

    int body_size = 5;
    int size;
    Display_Message_t tmp;

    char body_ch = lizard->score >= VICTORY_POINTS ? '*' : '.';
    tmp.ch = body_ch;
    tmp.pos_x = lizard->pos_x;
    tmp.pos_y = lizard->pos_y;
    direction_t direction = lizard->body_direction;
    int list_index;

    l = pthread_mutex_unlock(&lizard_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    switch (direction)
    {
    case UP:
        tmp.pos_x--;
        while (body_size && (0 < tmp.pos_x) && (tmp.pos_x < WINDOW_SIZE - 1))
        {
            body_size--;
            list_index = tmp.pos_x * WINDOW_SIZE + tmp.pos_y;
            add_to_board(board, list_index, tmp.ch, tmp.ch);

            // Send information to the display-app
            DisplayType_t type = INCREMENTAL_UPDATE;
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                exit(EXIT_FAILURE);
            size = zmq_send(publisher, &tmp, sizeof(Display_Message_t), 0);
            if (size == -1)
                exit(EXIT_FAILURE);
            tmp.pos_x--;
        }

        break;
    case DOWN:
        tmp.pos_x++;
        while (body_size && (0 < tmp.pos_x) && (tmp.pos_x < WINDOW_SIZE - 1))
        {
            body_size--;
            list_index = tmp.pos_x * WINDOW_SIZE + tmp.pos_y;
            add_to_board(board, list_index, tmp.ch, tmp.ch);

            // Send information to the display-app
            DisplayType_t type = INCREMENTAL_UPDATE;
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                exit(EXIT_FAILURE);
            size = zmq_send(publisher, &tmp, sizeof(Display_Message_t), 0);
            if (size == -1)
                exit(EXIT_FAILURE);
            tmp.pos_x++;
        }

        break;
    case LEFT:
        tmp.pos_y--;
        while (body_size && (0 < tmp.pos_y) && (tmp.pos_y < WINDOW_SIZE - 1))
        {
            body_size--;
            list_index = tmp.pos_x * WINDOW_SIZE + tmp.pos_y;
            add_to_board(board, list_index, tmp.ch, tmp.ch);

            // Send information to the display-app
            DisplayType_t type = INCREMENTAL_UPDATE;
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                exit(EXIT_FAILURE);
            size = zmq_send(publisher, &tmp, sizeof(Display_Message_t), 0);
            if (size == -1)
                exit(EXIT_FAILURE);
            tmp.pos_y--;
        }

        break;
    case RIGHT:
        tmp.pos_y++;
        while (body_size && (0 < tmp.pos_y) && (tmp.pos_y < WINDOW_SIZE - 1))
        {
            body_size--;
            list_index = tmp.pos_x * WINDOW_SIZE + tmp.pos_y;
            add_to_board(board, list_index, tmp.ch, tmp.ch);

            // Send information to the display-app
            DisplayType_t type = INCREMENTAL_UPDATE;
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                exit(EXIT_FAILURE);
            size = zmq_send(publisher, &tmp, sizeof(Display_Message_t), 0);
            if (size == -1)
                exit(EXIT_FAILURE);
            tmp.pos_y++;
        }

        break;
    default:
        break;
    }
}

void erase_lizard_body(void *publisher, Board_t *board, Lizard_Client_t *lizard)
{
    int l = pthread_mutex_lock(&lizard_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    if (lizard == NULL)
    {
        l = pthread_mutex_unlock(&lizard_mtx);
        if (l != 0)
            exit(EXIT_FAILURE);
        return;
    }
    if (lizard->score < 0)
    {
        l = pthread_mutex_unlock(&lizard_mtx);
        if (l != 0)
            exit(EXIT_FAILURE);
        return;
    }

    int body_size = 5; // lizard->score;
    int size;
    Display_Message_t tmp;
    char body_ch = lizard->score >= VICTORY_POINTS ? '*' : '.';
    tmp.ch = body_ch;
    tmp.pos_x = lizard->pos_x;
    tmp.pos_y = lizard->pos_y;

    l = pthread_mutex_unlock(&lizard_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    int list_index;

    switch (lizard->body_direction)
    {
    case UP:
        tmp.pos_x--;
        while (body_size && (0 < tmp.pos_x) && (tmp.pos_x < WINDOW_SIZE - 1))
        {
            body_size--;
            // Remover do stack dos tiles e escrever oq estava la
            list_index = tmp.pos_x * WINDOW_SIZE + tmp.pos_y;
            remove_from_board(board, list_index, &tmp.ch);

            // Send information to the display-app
            DisplayType_t type = INCREMENTAL_UPDATE;
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                exit(EXIT_FAILURE);

            size = zmq_send(publisher, &tmp, sizeof(Display_Message_t), 0);
            if (size == -1)
                exit(EXIT_FAILURE);

            tmp.pos_x--;
            tmp.ch = body_ch;
        }

        break;
    case DOWN:
        tmp.pos_x++;
        while (body_size && (0 < tmp.pos_x) && (tmp.pos_x < WINDOW_SIZE - 1))
        {
            body_size--;
            // Remover do stack dos tiles e escrever oq estava la
            list_index = tmp.pos_x * WINDOW_SIZE + tmp.pos_y;
            remove_from_board(board, list_index, &tmp.ch);

            // Send information to the display-app
            DisplayType_t type = INCREMENTAL_UPDATE;
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                exit(EXIT_FAILURE);

            size = zmq_send(publisher, &tmp, sizeof(Display_Message_t), 0);
            if (size == -1)
                exit(EXIT_FAILURE);
            tmp.pos_x++;
            tmp.ch = body_ch;
        }

        break;
    case LEFT:
        tmp.pos_y--;
        while (body_size && (0 < tmp.pos_y) && (tmp.pos_y < WINDOW_SIZE - 1))
        {
            body_size--;
            // Remover do stack dos tiles e escrever oq estava la
            list_index = tmp.pos_x * WINDOW_SIZE + tmp.pos_y;
            remove_from_board(board, list_index, &tmp.ch);

            // Send information to the display-app
            DisplayType_t type = INCREMENTAL_UPDATE;
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                exit(EXIT_FAILURE);

            size = zmq_send(publisher, &tmp, sizeof(Display_Message_t), 0);
            if (size == -1)
                exit(EXIT_FAILURE);

            tmp.pos_y--;
            tmp.ch = body_ch;
        }

        break;
    case RIGHT:
        tmp.pos_y++;
        while (body_size && (0 < tmp.pos_y) && (tmp.pos_y < WINDOW_SIZE - 1))
        {
            body_size--;
            // Remover do stack dos tiles e escrever oq estava la
            list_index = tmp.pos_x * WINDOW_SIZE + tmp.pos_y;
            remove_from_board(board, list_index, &tmp.ch);

            // Send information to the display-app
            DisplayType_t type = INCREMENTAL_UPDATE;
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                exit(EXIT_FAILURE);
            size = zmq_send(publisher, &tmp, sizeof(Display_Message_t), 0);
            if (size == -1)
                exit(EXIT_FAILURE);
            tmp.pos_y++;
            tmp.ch = body_ch;
        }

        break;
    default:
        break;
    }
}

void erase_lizard_body_lf(void *publisher, Board_t *board, Lizard_Client_t *lizard)
{
    if (lizard == NULL)
    {
        return;
    }
    if (lizard->score < 0)
    {
        return;
    }

    int body_size = 5; // lizard->score;
    int size;
    Display_Message_t tmp;
    char body_ch = lizard->score >= VICTORY_POINTS ? '*' : '.';
    tmp.ch = body_ch;
    tmp.pos_x = lizard->pos_x;
    tmp.pos_y = lizard->pos_y;

    int list_index;

    switch (lizard->body_direction)
    {
    case UP:
        tmp.pos_x--;
        while (body_size && (0 < tmp.pos_x) && (tmp.pos_x < WINDOW_SIZE - 1))
        {
            body_size--;
            // Remover do stack dos tiles e escrever oq estava la
            list_index = tmp.pos_x * WINDOW_SIZE + tmp.pos_y;
            remove_from_board_lf(board, list_index, &tmp.ch);

            // Send information to the display-app
            DisplayType_t type = INCREMENTAL_UPDATE;
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                exit(EXIT_FAILURE);

            size = zmq_send(publisher, &tmp, sizeof(Display_Message_t), 0);
            if (size == -1)
                exit(EXIT_FAILURE);

            tmp.pos_x--;
            tmp.ch = body_ch;
        }

        break;
    case DOWN:
        tmp.pos_x++;
        while (body_size && (0 < tmp.pos_x) && (tmp.pos_x < WINDOW_SIZE - 1))
        {
            body_size--;
            // Remover do stack dos tiles e escrever oq estava la
            list_index = tmp.pos_x * WINDOW_SIZE + tmp.pos_y;
            remove_from_board_lf(board, list_index, &tmp.ch);

            // Send information to the display-app
            DisplayType_t type = INCREMENTAL_UPDATE;
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                exit(EXIT_FAILURE);

            size = zmq_send(publisher, &tmp, sizeof(Display_Message_t), 0);
            if (size == -1)
                exit(EXIT_FAILURE);
            tmp.pos_x++;
            tmp.ch = body_ch;
        }

        break;
    case LEFT:
        tmp.pos_y--;
        while (body_size && (0 < tmp.pos_y) && (tmp.pos_y < WINDOW_SIZE - 1))
        {
            body_size--;
            // Remover do stack dos tiles e escrever oq estava la
            list_index = tmp.pos_x * WINDOW_SIZE + tmp.pos_y;
            remove_from_board_lf(board, list_index, &tmp.ch);

            // Send information to the display-app
            DisplayType_t type = INCREMENTAL_UPDATE;
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                exit(EXIT_FAILURE);

            size = zmq_send(publisher, &tmp, sizeof(Display_Message_t), 0);
            if (size == -1)
                exit(EXIT_FAILURE);

            tmp.pos_y--;
            tmp.ch = body_ch;
        }

        break;
    case RIGHT:
        tmp.pos_y++;
        while (body_size && (0 < tmp.pos_y) && (tmp.pos_y < WINDOW_SIZE - 1))
        {
            body_size--;
            // Remover do stack dos tiles e escrever oq estava la
            list_index = tmp.pos_x * WINDOW_SIZE + tmp.pos_y;
            remove_from_board_lf(board, list_index, &tmp.ch);

            // Send information to the display-app
            DisplayType_t type = INCREMENTAL_UPDATE;
            size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
            if (size == -1)
                exit(EXIT_FAILURE);
            size = zmq_send(publisher, &tmp, sizeof(Display_Message_t), 0);
            if (size == -1)
                exit(EXIT_FAILURE);
            tmp.pos_y++;
            tmp.ch = body_ch;
        }

        break;
    default:
        break;
    }
}

void free_lizard_clients(Lizard_Client_List_t *list)
{
    Lizard_Client_t *current = list->head;
    Lizard_Client_t *next;
    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }
    free(list);
    return;
}

/*------------------- Alphabet struct -----------------*/

Alphabet_List_t *create_alphabet_list()
{
    Alphabet_List_t *list = (Alphabet_List_t *)malloc(sizeof(Alphabet_List_t));
    if (list == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    list->head = NULL;
    return list;
}

Character_t *create_character(char ch)
{
    Character_t *character = (Character_t *)malloc(sizeof(Character_t));
    if (character == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    character->ch = ch;
    character->next = NULL;

    return character;
}

void insert_character(Alphabet_List_t *list, Character_t *ch)
{
    int l = pthread_mutex_lock(&alphabet_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    ch->next = list->head;
    list->head = ch;

    l = pthread_mutex_unlock(&alphabet_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    return;
}

void initialize_alphabet_list(Alphabet_List_t *alphabet_list)
{
    Character_t *character;
    for (int i = 25; i >= 0; i--)
    {
        character = create_character('A' + i);
        insert_character(alphabet_list, character);
    }
    return;
}

char get_new_character(Alphabet_List_t *list)
{
    int l = pthread_mutex_lock(&alphabet_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    char ch = list->head->ch;

    l = pthread_mutex_unlock(&alphabet_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    return ch;
}

void remove_character(Alphabet_List_t *list, char ch)
{
    int l = pthread_mutex_lock(&alphabet_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    Character_t *current = list->head;
    Character_t *prev = NULL;

    // Search for the node with the specified token
    while (current != NULL && current->ch != ch)
    {
        prev = current;
        current = current->next;
    }

    // If the node is found, remove it
    if (current != NULL)
    {
        // Node to be removed is the head
        if (prev == NULL)
            list->head = current->next;
        // Node to be removed is not the head
        else
            prev->next = current->next;

        free(current);
    }
    l = pthread_mutex_unlock(&alphabet_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);
    return;
}

void free_alphabet(Alphabet_List_t *list)
{
    Character_t *current = list->head;
    Character_t *next;
    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }
    free(list);
    return;
}

/*------------------- Scores functions -----------------*/

WINDOW *create_scores_window(int x, int y)
{
    WINDOW *win = newwin(SCORES_WIN_HEIGHT, SCORES_WIN_WIDTH, x, y);
    wattron(win, A_BOLD | A_UNDERLINE);
    mvwprintw(win, 0, 0, "Scores:");
    wattroff(win, A_BOLD | A_UNDERLINE);
    wrefresh(win);
    return win;
}

void print_lizards_scores(void *publisher, Lizard_Client_List_t *list)
{
    // Clear the entire window except for the first row
    // for (int i = 1; i < SCORES_WIN_HEIGHT; ++i)
    // {
    //     wmove(win, i, 0); // Move to the beginning of each row
    //     wclrtoeol(win);   // Clear from the current position to the end of the line
    // }

    // Update display-app score display
    DisplayType_t type = SCORE_SNAPSHOT;
    int size = zmq_send(publisher, &type, sizeof(DisplayType_t), ZMQ_SNDMORE);
    if (size == -1)
        exit(EXIT_FAILURE);

    int l = pthread_mutex_lock(&lizard_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    Lizard_Client_t *current = list->head;
    int position = 1;
    while (current != NULL)
    {
        int n = (int)log10(current->score);
        int nmax = SCORES_WIN_WIDTH - 7;

        char score[256];
        if (n >= nmax)
            sprintf(score, "%2d. %c +%d", position, current->ch, (int)pow(10, nmax) - 1);
        else
            sprintf(score, "%2d. %c  %*d", position, current->ch, nmax, current->score);

        // Send score to display-app
        size = zmq_send(publisher, score, strlen(score), ZMQ_SNDMORE);
        if (size == -1)
            exit(EXIT_FAILURE);

        current = current->next;
        position++;
    }

    l = pthread_mutex_unlock(&lizard_mtx);
    if (l != 0)
        exit(EXIT_FAILURE);

    // Send a last message to display-app
    size = zmq_send(publisher, "", strlen(""), 0);
    if (size == -1)
        exit(EXIT_FAILURE);

    return;
}
