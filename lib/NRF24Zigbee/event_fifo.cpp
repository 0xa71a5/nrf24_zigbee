#include "event_fifo.h"
#include "NRF24Zigbee.h"

inline void event_fifo_mark_mem_used(event_fifo_handle * p_fifo, uint8_t mem_index)
{
    if (mem_index > 15)
        mem_index = 15;

    p_fifo->mem_used = p_fifo->mem_used | (1 << mem_index);
}

inline void event_fifo_mark_mem_released(event_fifo_handle * p_fifo, uint8_t mem_index)
{
    if (mem_index > 15)
        mem_index = 15;

    p_fifo->mem_used = p_fifo->mem_used & (~(1 << mem_index));
}

inline uint8_t event_fifo_is_mem_used(event_fifo_handle * p_fifo, uint8_t mem_index)
{
    return (p_fifo->mem_used >> mem_index) & 0x1;
}

void event_fifo_init(event_fifo_handle * p_fifo,  event_node_handle *mem_base_addr, void *real_element_mem, uint8_t size)
{
    p_fifo->elements = mem_base_addr;
    p_fifo->real_element_mem = real_element_mem;
    p_fifo->front = 0;
    p_fifo->rear = 0;
    p_fifo->size = size;
    p_fifo->cur_size = 0;
    p_fifo->mem_used = 0;
}

inline void event_fifo_copy_node(event_node_handle *dst, event_node_handle *src)
{
    memcpy(dst, src, sizeof(event_node_handle));
}

inline bool event_fifo_is_full(event_fifo_handle * p_fifo)
{
    return p_fifo->cur_size >= p_fifo->size;
}

inline bool event_fifo_is_empty(event_fifo_handle * p_fifo)
{
    return p_fifo->cur_size == 0;
}

bool event_fifo_in(event_fifo_handle * p_fifo, event_type e_type, void *data)
{
    int i = 0;
    event_node_handle node;

    if (event_fifo_is_full(p_fifo)) {
        event_fifo_out(p_fifo, EVENT_TYPE_CONFIRM, NULL);
    }

    for (; i < p_fifo->size; i ++) {
        if (!event_fifo_is_mem_used(p_fifo, i))
            break;
    }

    if (i != p_fifo->size) {
        /* We found some unused mem area */
        if (e_type == EVENT_TYPE_CONFIRM) {
            event_confirm *elements = (event_confirm *)p_fifo->real_element_mem; 
            memcpy((uint8_t *)&elements[i], (uint8_t *)data, sizeof(event_confirm));
            node.head = (uint8_t *)&elements[i];
        }
        else if (e_type == EVENT_TYPE_INDICATION) {
            event_indication *elements = (event_indication *)p_fifo->real_element_mem; 
            memcpy((uint8_t *)&elements[i], (uint8_t *)data, sizeof(event_indication));
            node.head = (uint8_t *)&elements[i];
        }
        node.mem_index = i;
        event_fifo_mark_mem_used(p_fifo, i);
    }
    else {
        /* This conditon shall not happend! */
        return false;
    }

    event_fifo_copy_node(&p_fifo->elements[p_fifo->rear], &node);
    p_fifo->rear = (p_fifo->rear+1) % p_fifo->size;
    p_fifo->cur_size ++;
    return true;
}

bool event_fifo_out(event_fifo_handle * p_fifo, event_type e_type, void *data)
{
    event_node_handle *node;

    if(event_fifo_is_empty(p_fifo)) {
        return false;
    }

    node = &p_fifo->elements[p_fifo->front];

    if (data) {
        if (e_type == EVENT_TYPE_CONFIRM) {
            memcpy((uint8_t *)data, node->head, sizeof(event_confirm));
        }
        else if (e_type == EVENT_TYPE_INDICATION) {
            memcpy((uint8_t *)data, node->head, sizeof(event_indication));
        }
    }

    event_fifo_mark_mem_released(p_fifo, node->mem_index);

    p_fifo->front = (p_fifo->front+1) % p_fifo->size;
    p_fifo->cur_size --;
    return true;
}

bool event_fifo_find_node(event_fifo_handle * p_fifo, event_type e_type, uint8_t caller_handle, uint8_t *elements_index)
{
    uint8_t i = p_fifo->front;
    uint8_t cur = 0;
    uint8_t this_caller_handle = 0xff;

    for (cur = 0; cur < p_fifo->cur_size; cur ++)
    {
        if (e_type == EVENT_TYPE_CONFIRM)
            this_caller_handle = ((event_confirm *)p_fifo->elements[i].head)->caller_handle;
        else if (e_type == EVENT_TYPE_INDICATION)
            this_caller_handle = ((event_indication *)p_fifo->elements[i].head)->caller_handle;

        if (this_caller_handle == caller_handle) {
            *elements_index = i;
            return true;
        }

        i = (i+1) % p_fifo->size;
    }
    return false;
}

bool event_fifo_fetch_node(event_fifo_handle * p_fifo, event_type e_type, uint8_t caller_handle, void * data)
{
    uint8_t i = p_fifo->front;
    uint8_t cur = 0;
    uint8_t this_caller_handle = 0xff;
    uint8_t copy_size = 0;

    for (cur = 0; cur < p_fifo->cur_size; cur ++)
    {
        if (e_type == EVENT_TYPE_CONFIRM) {
            this_caller_handle = ((event_confirm *)p_fifo->elements[i].head)->caller_handle;
        }
        else if (e_type == EVENT_TYPE_INDICATION) {
            this_caller_handle = ((event_indication *)p_fifo->elements[i].head)->caller_handle;
        }

        if (this_caller_handle == caller_handle)
            break;

        i = (i+1) % p_fifo->size;
    }

    
    if (p_fifo->rear == 0)
        p_fifo->rear = p_fifo->size - 1;
    else
        p_fifo->rear --;
    //    printf("This is not first node, rear-- ,rear =%u\n", p_fifo->rear);


    if (cur != p_fifo->cur_size) {
        copy_size = e_type == EVENT_TYPE_CONFIRM ? sizeof(event_confirm) : sizeof(event_indication);
        memcpy((uint8_t *)data, p_fifo->elements[i].head, copy_size);
        event_fifo_mark_mem_released(p_fifo, p_fifo->elements[i].mem_index);

        /* Shift elements behand target */
        for (; cur < p_fifo->cur_size - 1; cur ++) {
            p_fifo->elements[i] = p_fifo->elements[(i + 1) % p_fifo->size];
            i = (i + 1) % p_fifo->size;
        }

        p_fifo->cur_size --;
        return true;
    }
    else {
        return false;
    }

}

void event_fifo_traverse(event_fifo_handle * p_fifo)
{
    uint8_t i = p_fifo->front;
    uint8_t cur = 0;

    for (cur = 0; cur < p_fifo->cur_size; cur ++)
    {
        printf("[0x%08X] <- ", p_fifo->elements[i].head);
        i = (i+1) % p_fifo->size;
    }
    printf("\n");
}

#ifdef EVENT_FIFO_TEST

#define CONFIRM_FIFO_SIZE 4
#define INDICATION_FIFO_SIZE 3

event_node_handle confirm_ptr_area[CONFIRM_FIFO_SIZE];
event_confirm confirm_big_mem[CONFIRM_FIFO_SIZE];

event_node_handle indication_ptr_area[INDICATION_FIFO_SIZE];
event_indication indication_big_mem[INDICATION_FIFO_SIZE];

int fifo_main_test()
{
    event_fifo_handle nwk_confirm_fifo;
    event_fifo_handle nwk_indication_fifo;

    event_fifo_init(&nwk_confirm_fifo, confirm_ptr_area, confirm_big_mem, CONFIRM_FIFO_SIZE);
    event_fifo_init(&nwk_indication_fifo, indication_ptr_area, indication_big_mem, INDICATION_FIFO_SIZE);

    for (int i = 0; i < 5; i ++) {
        event_confirm c;
        c.caller_handle = i;
        c.confirm_status = i+1;
        event_fifo_in(&nwk_confirm_fifo, EVENT_TYPE_CONFIRM, &c);
        event_fifo_traverse(&nwk_confirm_fifo);
        printf("\n");
    }

    event_confirm c;
    uint8_t target_caller_handle = 1;
    printf("\nFetch caller_handle %u\n", target_caller_handle);
    if (event_fifo_fetch_node(&nwk_confirm_fifo, EVENT_TYPE_CONFIRM, target_caller_handle, &c)) {
        printf("Fetch success, confirm_event -> caller_handle=%u confirm_status=%u\n", c.caller_handle, c.confirm_status);
    }
    printf("\n");


    printf("Try to fifo out\n");
    bool result = false;
    do {
        event_confirm c;
        result = event_fifo_out(&nwk_confirm_fifo, EVENT_TYPE_CONFIRM, &c);
        printf("event_confirm.caller_handle=%u .confirm_status=%u\n", c.caller_handle, c.confirm_status);
        event_fifo_traverse(&nwk_confirm_fifo);
        printf("\n");
    }
    while(result);


    printf("Done!\n");

    return 0;
}
#endif