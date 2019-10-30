#pragma once


typedef struct go2_queue go2_queue_t;


#ifdef __cplusplus
extern "C" {
#endif

go2_queue_t* go2_queue_create(int capacity);
int go2_queue_count_get(go2_queue_t* queue);
void go2_queue_push(go2_queue_t* queue, void* value);
void* go2_queue_pop(go2_queue_t* queue);
void go2_queue_destroy(go2_queue_t* queue);

#ifdef __cplusplus
}
#endif
