#ifndef EVENT_FIFO_H
#define EVENT_FIFO_H
#include <stdint.h>
#include <FreeRTOS_AVR.h>

typedef struct __event_confirm
{
    uint8_t caller_handle;
    uint8_t confirm_status;
} event_confirm;

typedef struct __event_indication
{
    uint8_t caller_handle;
    uint8_t indication_status;
    uint8_t data[116];
} event_indication;

/* This store the node's memory addr. */
typedef struct __event_node_handle_ptr
{
    uint8_t *head;
    uint8_t mem_index;
} event_node_handle;

typedef struct __event_fifo_handle
{
    event_node_handle *elements;/* Store elements free index ptr */
    void  *real_element_mem;/* Store real elements mem*/
    uint8_t front;
    uint8_t rear;
    uint8_t size;
    uint8_t cur_size;
    uint8_t element_size;
    uint16_t mem_used;
    SemaphoreHandle_t fifo_lock;
} event_fifo_handle;


enum event_type {
    EVENT_TYPE_CONFIRM = 0,
    EVENT_TYPE_INDICATION,
};

//#define EVENT_FIFO_TEST


inline void event_fifo_mark_mem_used(event_fifo_handle * p_fifo, uint8_t mem_index);
inline void event_fifo_mark_mem_released(event_fifo_handle * p_fifo, uint8_t mem_index);
inline uint8_t event_fifo_is_mem_used(event_fifo_handle * p_fifo, uint8_t mem_index);
void event_fifo_init(event_fifo_handle * p_fifo,  event_node_handle *mem_base_addr, void *real_element_mem, uint8_t size, uint8_t element_size);
inline void event_fifo_copy_node(event_node_handle *dst, event_node_handle *src);
inline bool event_fifo_is_full(event_fifo_handle * p_fifo);
inline bool event_fifo_is_empty(event_fifo_handle * p_fifo);
bool event_fifo_in(event_fifo_handle * p_fifo, void *data);
bool event_fifo_out(event_fifo_handle * p_fifo, void *data);
bool event_fifo_find_node(event_fifo_handle * p_fifo, uint8_t caller_handle, uint8_t *elements_index);
bool event_fifo_fetch_node(event_fifo_handle * p_fifo, uint8_t caller_handle, void * data);
void event_fifo_traverse(event_fifo_handle * p_fifo);
#ifdef EVENT_FIFO_TEST
int fifo_main_test();
#endif

#endif