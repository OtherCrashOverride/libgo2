#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct go2_queue
{
    int capacity;
    int count;
    void** data;
} go2_queue_t;



go2_queue_t* go2_queue_create(int capacity)
{
    go2_queue_t* result = malloc(sizeof(*result));
    if (!result)
    {
        printf("malloc failed.\n");
        return NULL;
    }

    memset(result, 0, sizeof(*result));


    result->capacity = capacity;
    result->data = malloc(capacity * sizeof(void*));
    if (!result)
    {
        printf("data malloc failed.\n");
        free(result);
        return NULL;
    }

    return result;
}

int go2_queue_count_get(go2_queue_t* queue)
{
    return queue->count;
}

void go2_queue_push(go2_queue_t* queue, void* value)
{
    if (queue->count < queue->capacity)
    {
        queue->data[queue->count] = value;
        queue->count++;
    }
    else
    {
        printf("queue is full.\n");
    }    
}

void* go2_queue_pop(go2_queue_t* queue)
{
    void* result;

    if (queue->count > 0)
    {
        result = queue->data[0];

        for (int i = 0; i < queue->count - 1; ++i)
        {
            queue->data[i] = queue->data[i + 1];
        }

        queue->count--;
    }
    else
    {
        printf("queue is empty.\n");
        result = NULL;
    }
    
    return result;
}

void go2_queue_destroy(go2_queue_t* queue)
{
    free(queue->data);
    free(queue);
}
