#include "ma.h"
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

typedef struct {
    bool is_alive;

    moore_t *a_in, *a_out;
    size_t in, out;

    // Position in the outgoing connections list
    size_t pos;
} ma_connection_t;

typedef struct {
    ma_connection_t **connections;
    size_t lth, size;
} ma_connection_list_t;
    
struct moore {
    size_t n, m, s;
    uint64_t *state;
    transition_function_t t;
    output_function_t y;
    
    struct {
        uint64_t *value;

        // Incoming connections
        ma_connection_t *source;
    } input;
    
    struct {
        uint64_t *value;

        // Outgoing connections
        ma_connection_list_t dest;
    } output;
};

/* ========== ========== ========== ========== */

#define BLOCKS(x) (((x) - 1) / 64 + 1)

void identity_function(uint64_t *out, uint64_t const *state, size_t m, size_t) {
    memcpy(out, state, sizeof(uint64_t) * BLOCKS(m));
}

#define FAIL_INCR -1
#define FAIL_DECR -2

/**
 * @brief Resizes the connection list.
 * 
 * If the list is NULL, it will be initialized with the new size.
 * If the new size is 0, the list will be freed.
 * 
 * @param list The connection list to resize.
 * @param new_size The new size of the list.
 * 
 * @return 0 if the list was resized successfully
 * 
 * FAIL_INCR/FAIL_DECR if the list could not be extended/shrunk
 */
int REALLOC(ma_connection_list_t *list, size_t new_size) {
    if (list == NULL) {
        if (new_size != 0) {
            list->connections = (ma_connection_t **)malloc(
                sizeof(ma_connection_t *) * new_size
            );

            if (list->connections == NULL) {
                return FAIL_INCR;
            }

            list->size = new_size;
        }

        return 0;
    }

    if (new_size == 0) {
        free(list->connections);

        list->connections = NULL;
        list->lth = list->size = 0;

        return 0;
    }

    ma_connection_t **new_connections = (ma_connection_t **)realloc(
        list->connections, 
        sizeof(ma_connection_t *) * new_size
    );

    if (new_connections == NULL) {
        return (list->size < new_size) ? FAIL_INCR : FAIL_DECR;
    }

    list->connections = new_connections;
    list->size = new_size;

    return 0;
}

/* ========== ========== ========== ========== */

moore_t * ma_create_full(size_t n, size_t m, size_t s, 
                         transition_function_t t,  output_function_t y, 
                         uint64_t const *q) {
    if (m == 0 || s == 0 || t == NULL || y == NULL || q == NULL) {
        errno = EINVAL;
        return NULL;
    }

    moore_t *a = (moore_t *)malloc(sizeof(moore_t));

    if (a == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    a->n = n;
    a->m = m;
    a->s = s;
    
    a->t = t;
    a->y = y;
    
    a->state = (uint64_t *)malloc(sizeof(uint64_t) * BLOCKS(s));
    
    if (a->state == NULL) {
        free(a);
        
        errno = ENOMEM;
        return NULL;
    }
    
    memcpy(a->state, q, sizeof(uint64_t) * BLOCKS(s));
    
    if (n == 0) {
        a->input.value = NULL;
        a->input.source = NULL;
    }
    else {
        a->input.value = (uint64_t *)calloc(BLOCKS(n), sizeof(uint64_t));
        a->input.source = (ma_connection_t *)calloc(n, sizeof(ma_connection_t));

        if (a->input.value == NULL || a->input.source == NULL) {
            free(a->state);
            free(a->input.value);
            free(a->input.source);
            free(a);
            
            errno = ENOMEM;
            return NULL;
        }
    }

    a->output.value = (uint64_t *)malloc(sizeof(uint64_t) * BLOCKS(m));
    a->output.dest.connections = NULL;
    a->output.dest.lth = a->output.dest.size = 0;
    
    if (a->output.value == NULL) {
        ma_delete(a);
        
        errno = ENOMEM;
        return NULL;
    }
    
    // Update the output value
    a->y(a->output.value, a->state, m, s);

    return a;
}

moore_t * ma_create_simple(size_t n, size_t s, transition_function_t t) {
    if (s == 0 || t == NULL) {
        errno = EINVAL;
        return NULL;
    }

    uint64_t *state = (uint64_t *)calloc(BLOCKS(s), sizeof(uint64_t));
    
    if (state == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    moore_t *a = ma_create_full(n, s, s, t, identity_function, state);

    free(state);

    return a;
}

void ma_delete(moore_t *a) {
    if (a == NULL) {
        return;
    }

    free(a->state);
    
    // Mark all outgoing connections as "dead"
    for (size_t i = 0; i < a->output.dest.lth; i++) {
        ma_connection_t *connection = a->output.dest.connections[i];
        connection->a_in->input.source[connection->in].is_alive = false;
    }

    // Disconnect all incoming connections
    for (size_t i = 0; i < a->n; i++) {
        if (a->input.source[i].is_alive == true) {
            ma_disconnect(a, i, 1);
        }
    }
    
    free(a->input.value);
    free(a->input.source);

    free(a->output.value);
    free(a->output.dest.connections);
    
    free(a);
}

ma_connection_list_t *MA_DISCONNECT_KEEP_LIST = NULL;
size_t MA_DISCONNECT_KEEP_SIZE = 0;

int ma_connect(moore_t *a_in, size_t in, 
               moore_t *a_out, size_t out, 
               size_t num) {
    if (a_in == NULL || a_out == NULL || num == 0) {
        errno = EINVAL;
        return -1;
    }

    if ((__uint128_t)in + num > a_in->n || (__uint128_t)out + num > a_out->m) {
        errno = EINVAL;
        return -1;
    }

    ma_connection_list_t *dest = &a_out->output.dest;
    
    // Allocate memory for new connections
    if (REALLOC(dest, dest->lth + num) == FAIL_INCR) {
        errno = ENOMEM;
        return -1;
    }

    // Flags needed in ma_disconnect
    MA_DISCONNECT_KEEP_LIST = dest;
    MA_DISCONNECT_KEEP_SIZE = num;
    
    // Disconnect current connections
    ma_disconnect(a_in, in, num);

    MA_DISCONNECT_KEEP_LIST = NULL;
    // MA_DISCONNECT_KEEP_SIZE = 0;

    for (size_t i = 0; i < num; i++) {
        ma_connection_t *connection = &a_in->input.source[in + i];

        connection->is_alive = true;

        connection->a_in = a_in, connection->a_out = a_out;
        connection->in = in + i, connection->out = out + i;

        connection->pos = dest->lth++;
        dest->connections[connection->pos] = connection;
    }

    return 0;
}

int ma_disconnect(moore_t *a_in, size_t in, size_t num) {
    if (a_in == NULL || num == 0 || (__uint128_t)in + num > a_in->n) {
        errno = EINVAL;
        return -1;
    }

    for (size_t i = 0; i < num; i++) {
        ma_connection_t *connection = &a_in->input.source[in + i];
        
        if (connection->is_alive == true) {
            ma_connection_list_t *dest = &connection->a_out->output.dest;

            // "Remove" connection by swapping it with the last one
            dest->connections[connection->pos] = dest->connections[--dest->lth];

            // Update the position of swapped connection
            dest->connections[connection->pos]->pos = connection->pos;

            // Do not resize if called from ma_connect
            if (dest != MA_DISCONNECT_KEEP_LIST) {
                REALLOC(dest, dest->lth);
            }
    
            connection->is_alive = false;
        }
    }

    // Resize with size margin if called from ma_connect
    if (MA_DISCONNECT_KEEP_LIST != NULL) {
        REALLOC(
            MA_DISCONNECT_KEEP_LIST, 
            MA_DISCONNECT_KEEP_LIST->lth + MA_DISCONNECT_KEEP_SIZE
        );
    }

    return 0;
}

int ma_set_input(moore_t *a, uint64_t const *input) {
    if (a == NULL || input == NULL || a->n == 0) {
        errno = EINVAL;
        return -1;
    }

    for (size_t i = 0; i < a->n; i++) {
        if (a->input.source[i].is_alive == false) {
            size_t block = i / 64;
            size_t mask = (uint64_t)1 << (i % 64);
            
            // Set i-th bit of a's input the corresponding bit in the source
            a->input.value[block] &= ~mask;
            a->input.value[block] |= input[block] & mask;
        }
    }
    
    return 0;
}

int ma_set_state(moore_t *a, uint64_t const *state) {
    if (a == NULL || state == NULL) {
        errno = EINVAL;
        return -1;
    }

    memcpy(a->state, state, sizeof(uint64_t) * BLOCKS(a->s));
    a->y(a->output.value, a->state, a->m, a->s);

    return 0;
}

uint64_t const * ma_get_output(moore_t const *a) {
    if (a == NULL) {
        errno = EINVAL;
        return NULL;
    }

    return a->output.value;
}

int ma_step(moore_t *at[], size_t num) {
    if (at == NULL || num == 0) {
        errno = EINVAL;
        return -1;
    }

    for (size_t i = 0; i < num; i++) {
        if (at[i] == NULL) {
            errno = EINVAL;
            return -1;
        }
    }

    // Allocate an array of pointers to where new states will be stored
    uint64_t **state_cpy = (uint64_t **)malloc(sizeof(uint64_t *) * num);

    if (state_cpy == NULL) {
        free(state_cpy);
        
        errno = ENOMEM;
        return -1;
    }

    for (size_t i = 0; i < num; i++) {
        state_cpy[i] = (uint64_t *)malloc(sizeof(uint64_t) * BLOCKS(at[i]->s));

        if (state_cpy[i] == NULL) {
            // Free all previously allocated copies
            for (size_t j = 0; j < i; j++) {
                free(state_cpy[j]);
            }

            free(state_cpy);
            
            errno = ENOMEM;
            return -1;
        }

        memcpy(state_cpy[i], at[i]->state, sizeof(uint64_t) * BLOCKS(at[i]->s));
    }

    for (size_t i = 0; i < num; i++) {
        for (size_t j = 0; j < at[i]->n; j++) {
            if (at[i]->input.source[j].is_alive == false) {
                continue;
            }

            size_t block = j / 64;
            size_t mask = (uint64_t)1 << (j % 64);

            size_t sblock = at[i]->input.source[j].out / 64;
            size_t smask = (uint64_t)1 << (at[i]->input.source[j].out % 64);
            
            // Set j-th bit of at[i]'s input to 0
            at[i]->input.value[block] &= ~mask;

            // Set j-th bit of at[i]'s input to the corresponding 
            // bit in the source
            if (at[i]->input.source[j].a_out->output.value[sblock] & smask) {
                at[i]->input.value[block] |= mask;
            }
        }
    }

    for (size_t i = 0; i < num; i++) {
        // Call the transition function
        at[i]->t(
            state_cpy[i], 
            at[i]->input.value, at[i]->state, 
            at[i]->n, at[i]->s
        );
        
        memcpy(at[i]->state, state_cpy[i], sizeof(uint64_t) * BLOCKS(at[i]->s));
        free(state_cpy[i]);
        
        // Update the output value
        at[i]->y(at[i]->output.value, at[i]->state, at[i]->m, at[i]->s);
    }

    free(state_cpy);

    return 0;
}
