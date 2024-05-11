#include "lizardNroach.h"

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
    if (!(list_index >= 0 && list_index < array->size))
    {
        printf("Error adding a new element.\n");
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
    // free(current);
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
        // Isto significa que n existe lizard no tile
        if (lizard == ' ')
            break;
    }
}

Roach_t *create_roach(Board_t *board, int id, int digit)
{
    Roach_t *roach = (Roach_t *)malloc(sizeof(Roach_t));
    if (roach == NULL)
    {
        printf("Memory allocation failed.\n");
        exit(1);
    }

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
    roach->next = list->head;
    list->head = roach;
    return;
}

Roach_t *find_roach_by_id(Roaches_List_t *list, int id)
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

void remove_roach_by_id(Roaches_List_t *list, int id)
{
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
        return;
    }
    // If it is not found return
    else
        return;
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
    Dead_Roach_t *droach = create_dead_roach(id);

    if (list->head == NULL)
        list->head = droach;
    else
        list->tail->next = droach;

    list->tail = droach;
    return;
}

void revive_roaches(WINDOW *win, void *publisher, Board_t *board, Dead_Roaches_List_t *dead_roaches, Roaches_List_t *roaches)
{
    Dead_Roach_t *current = dead_roaches->head;

    while (current != NULL && ((time(NULL) - current->time_eaten) >= 5))
    {
        Roach_t *roach = find_roach_by_id(roaches, current->id);
        roach->is_dead = false;

        // Calculate new posiiton
        int new_pos_x, new_pos_y;
        new_roach_position(board, &new_pos_x, &new_pos_y);
        roach->pos_x = new_pos_x;
        roach->pos_y = new_pos_y;

        // Add roach to the board
        int list_index = roach->pos_x * WINDOW_SIZE + roach->pos_y;
        add_to_board(board, list_index, roach->id, roach->digit);

        dead_roaches->head = current->next;
        free(current);
        current = dead_roaches->head;

        // Update server display
        wmove(win, roach->pos_x, roach->pos_y);
        waddch(win, roach->digit | A_BOLD);
        wrefresh(win);

        // Update the display-app
        Display_Message_t d_message;
        d_message.ch = roach->digit;
        d_message.pos_x = roach->pos_x;
        d_message.pos_y = roach->pos_y;

        // Send information to n display-app
        int size = zmq_send(publisher, DISPLAY_LABEL, strlen(DISPLAY_LABEL), ZMQ_SNDMORE);
        if (size == -1)
            exit(EXIT_FAILURE);
        size = zmq_send(publisher, &d_message, sizeof(Display_Message_t), 0);
        if (size == -1)
            exit(EXIT_FAILURE);
    }
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

int eat_roaches(Board_t *array, int list_index, Roaches_List_t *roaches, Dead_Roaches_List_t *Dead_Roaches_list)
{
    if (!(list_index >= 0 && list_index < array->size))
    {
        printf("Error.\n");
        return ' ';
    }

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
            Roach_t *roach = find_roach_by_id(roaches, index);
            roach->is_dead = true;
            insert_dead_roach(Dead_Roaches_list, roach->id);

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

    return sum;
}

void remove_roach_client_by_id(Roach_Client_List_t *list, int token)
{
    Roach_Client_t *current = list->head;
    Roach_Client_t *prev = NULL;

    // Search for the node with the specified token
    while (current != NULL && current->token != token)
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
        // Isto significa que n existe lizard no tile
        if (lizard == ' ')
            break;
    }

    lizard->body_direction = random() % 4;
    lizard->score = 0;
    lizard->next = NULL;
    return lizard;
}

void insert_lizard_at_end(Lizard_Client_List_t *list, Lizard_Client_t *lizard)
{
    if (list->head == NULL)
        list->head = lizard;
    else
        list->tail->next = lizard;

    list->tail = lizard;
    return;
}

void sort_lizard_list(Lizard_Client_List_t *list, char ch)
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
        return;

    // Lizard is already in its place
    if ((prev == NULL || prev->score >= current->score) && (current->next == NULL || current->score >= current->next->score))
        return;

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
}

Lizard_Client_t *find_lizard_by_ch(Lizard_Client_List_t *list, char ch)
{
    Lizard_Client_t *current = list->head;

    while (current != NULL)
    {
        if (current->ch == ch)
        {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

char get_existent_lizard_head(Board_t *array, int list_index)
{
    if (list_index <= WINDOW_SIZE || list_index % WINDOW_SIZE == 0 || list_index % WINDOW_SIZE == WINDOW_SIZE - 1 || list_index >= array->size - WINDOW_SIZE)
    {
        return '\0';
    }

    Board_Tile_t *current = array->lists[list_index]->head;

    // Search for a lizard in the position given by list_index
    while (current != NULL)
    {
        if (current->ch != ' ' && current->ch != '\0' && current->ch != '*' && current->ch != '.' && current->ch != '1' && current->ch != '2' && current->ch != '3' && current->ch != '4' && current->ch != '5')
            return current->ch;

        current = current->next;
    }

    return ' ';
}

void remove_lizard_by_ch(Lizard_Client_List_t *list, char ch)
{
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

        free(current);
        return;
    }
    // If it is not found return
    else
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

void draw_lizard_body(void *publisher, WINDOW *my_win, Board_t *board, Lizard_Client_t *lizard)
{
    int body_size = 5;
    int size;
    Display_Message_t tmp;
    char body_ch = lizard->score >= VICTORY_POINTS ? '*' : '.';
    // char body_ch = '*';
    tmp.ch = body_ch;
    tmp.pos_x = lizard->pos_x;
    tmp.pos_y = lizard->pos_y;
    tmp.score = lizard->score;

    int list_index;

    switch (lizard->body_direction)
    {
    case UP:
        tmp.pos_x--;
        while (body_size && (0 < tmp.pos_x) && (tmp.pos_x < WINDOW_SIZE - 1))
        {
            body_size--;
            list_index = tmp.pos_x * WINDOW_SIZE + tmp.pos_y;
            add_to_board(board, list_index, lizard->ch, tmp.ch);

            // Update server display
            wmove(my_win, tmp.pos_x, tmp.pos_y);
            waddch(my_win, tmp.ch | A_BOLD);
            wrefresh(my_win);

            // Send information to the display-app
            size = zmq_send(publisher, DISPLAY_LABEL, strlen(DISPLAY_LABEL), ZMQ_SNDMORE);
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
            add_to_board(board, list_index, lizard->ch, tmp.ch);

            // Update server display
            wmove(my_win, tmp.pos_x, tmp.pos_y);
            waddch(my_win, tmp.ch | A_BOLD);
            wrefresh(my_win);

            // Send information to the display-app
            size = zmq_send(publisher, DISPLAY_LABEL, strlen(DISPLAY_LABEL), ZMQ_SNDMORE);
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
            add_to_board(board, list_index, lizard->ch, tmp.ch);

            // Update server display
            wmove(my_win, tmp.pos_x, tmp.pos_y);
            waddch(my_win, tmp.ch | A_BOLD);
            wrefresh(my_win);

            // Send information to the display-app
            size = zmq_send(publisher, DISPLAY_LABEL, strlen(DISPLAY_LABEL), ZMQ_SNDMORE);
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
            add_to_board(board, list_index, lizard->ch, tmp.ch);

            // Update server display
            wmove(my_win, tmp.pos_x, tmp.pos_y);
            waddch(my_win, tmp.ch | A_BOLD);
            wrefresh(my_win);

            // Send information to the display-app
            size = zmq_send(publisher, DISPLAY_LABEL, strlen(DISPLAY_LABEL), ZMQ_SNDMORE);
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

void erase_lizard_body(void *publisher, WINDOW *my_win, Board_t *board, Lizard_Client_t *lizard)
{
    int body_size = 5; // lizard->score;
    char body_ch = lizard->score >= VICTORY_POINTS ? '*' : '.';
    // char body_ch = '*';
    int size;
    Display_Message_t tmp;
    tmp.ch = body_ch;
    tmp.pos_x = lizard->pos_x;
    tmp.pos_y = lizard->pos_y;
    tmp.score = lizard->score;

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

            // Update server display
            wmove(my_win, tmp.pos_x, tmp.pos_y);
            waddch(my_win, tmp.ch | A_BOLD);
            wrefresh(my_win);

            // Send information to the display-app
            size = zmq_send(publisher, DISPLAY_LABEL, strlen(DISPLAY_LABEL), ZMQ_SNDMORE);
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

            // Update server display
            wmove(my_win, tmp.pos_x, tmp.pos_y);
            waddch(my_win, tmp.ch | A_BOLD);
            wrefresh(my_win);

            // Send information to the display-app
            size = zmq_send(publisher, DISPLAY_LABEL, strlen(DISPLAY_LABEL), ZMQ_SNDMORE);
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

            // Update server display
            wmove(my_win, tmp.pos_x, tmp.pos_y);
            waddch(my_win, tmp.ch | A_BOLD);
            wrefresh(my_win);

            // Send information to the display-app
            size = zmq_send(publisher, DISPLAY_LABEL, strlen(DISPLAY_LABEL), ZMQ_SNDMORE);
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

            // Update server display
            wmove(my_win, tmp.pos_x, tmp.pos_y);
            waddch(my_win, tmp.ch | A_BOLD);
            wrefresh(my_win);

            // Send information to the display-app
            size = zmq_send(publisher, DISPLAY_LABEL, strlen(DISPLAY_LABEL), ZMQ_SNDMORE);
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
    ch->next = list->head;
    list->head = ch;
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
    return list->head->ch;
}

void remove_character(Alphabet_List_t *list, char ch)
{
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
        return;
    }

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

void print_lizards_scores(WINDOW *win, void *publisher, Lizard_Client_List_t *list)
{
    // Clear the entire window except for the first row
    for (int i = 1; i < SCORES_WIN_HEIGHT; ++i)
    {
        wmove(win, i, 0); // Move to the beginning of each row
        wclrtoeol(win);   // Clear from the current position to the end of the line
    }

    // Update display-app score display
    int size;
    size = zmq_send(publisher, SCORE_LABEL, strlen(SCORE_LABEL), ZMQ_SNDMORE);
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

        // Update server score display
        if (has_colors() && position < 4)
            wattron(win, COLOR_PAIR(position));

        mvwaddstr(win, position, 0, score);

        if (has_colors() && position < 4)
            wattroff(win, COLOR_PAIR(position));

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

    wrefresh(win);
}
