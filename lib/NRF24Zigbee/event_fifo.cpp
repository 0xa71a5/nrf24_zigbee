#include "event_fifo.h"
#include "NRF24Zigbee.h"

#define fifo_use_mutex_lock

#ifdef fifo_use_mutex_lock
    #define mutex_take(lockname, timeout) xSemaphoreTake(lockname, timeout)
    #define mutex_give(lockname)          xSemaphoreGive(lockname)
#else
    #define mutex_take(lockname, timeout) 
    #define mutex_give(lockname)          
#endif

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

void event_fifo_init(event_fifo_handle * p_fifo,  event_node_handle *mem_base_addr, void *real_element_mem, uint8_t size, uint8_t element_size)
{
    p_fifo->elements = mem_base_addr;
    p_fifo->real_element_mem = real_element_mem;
    p_fifo->front = 0;
    p_fifo->rear = 0;
    p_fifo->size = size;
    p_fifo->cur_size = 0;
    p_fifo->mem_used = 0;
    p_fifo->element_size = element_size;
    p_fifo->fifo_lock = xSemaphoreCreateMutex();
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

bool event_fifo_in(event_fifo_handle * p_fifo, void *data)
{
    uint16_t i = 0;
    event_node_handle node;

    if (event_fifo_is_full(p_fifo)) {
        event_fifo_out(p_fifo, NULL);
    }

    mutex_take(p_fifo->fifo_lock, portMAX_DELAY );
    for (; i < p_fifo->size; i ++) {
        if (!event_fifo_is_mem_used(p_fifo, i))
            break;
    }

    if (i != p_fifo->size) {
        /* We found some unused mem area */
        node.head = p_fifo->real_element_mem + (uint16_t)p_fifo->element_size * i;
        memcpy(node.head, data, p_fifo->element_size);
        node.mem_index = i;
        event_fifo_mark_mem_used(p_fifo, i);
    }
    else {
        mutex_give(p_fifo->fifo_lock);
        /* This conditon shall not happend! */
        return false;
    }

    event_fifo_copy_node(&p_fifo->elements[p_fifo->rear], &node);
    p_fifo->rear = (p_fifo->rear+1) % p_fifo->size;
    p_fifo->cur_size ++;
    mutex_give(p_fifo->fifo_lock);

    return true;
}

bool event_fifo_out(event_fifo_handle * p_fifo, void *data)
{
    event_node_handle *node;

    mutex_take(p_fifo->fifo_lock, portMAX_DELAY );
    if(event_fifo_is_empty(p_fifo)) {
        mutex_give(p_fifo->fifo_lock);
        return false;
    }

    node = &p_fifo->elements[p_fifo->front];

    if (data)
        memcpy(data, node->head, p_fifo->element_size);

    event_fifo_mark_mem_released(p_fifo, node->mem_index);

    p_fifo->front = (p_fifo->front+1) % p_fifo->size;
    p_fifo->cur_size --;
    mutex_give(p_fifo->fifo_lock);

    return true;
}

bool event_fifo_find_node(event_fifo_handle * p_fifo, uint8_t caller_handle, uint8_t *elements_index)
{
    uint16_t i = p_fifo->front;
    uint8_t cur = 0;
    uint8_t this_caller_handle = 0xff;

    mutex_take(p_fifo->fifo_lock, portMAX_DELAY );
    for (cur = 0; cur < p_fifo->cur_size; cur ++)
    {
        this_caller_handle = *(uint8_t *)(p_fifo->elements[i].head + (uint16_t)p_fifo->element_size * i);

        if (this_caller_handle == caller_handle) {
            *elements_index = i;
            return true;
        }

        i = (i+1) % p_fifo->size;
    }
    mutex_give(p_fifo->fifo_lock);
    return false;
}

bool event_fifo_fetch_node(event_fifo_handle * p_fifo, uint8_t caller_handle, void * data)
{
    uint8_t i = p_fifo->front;
    uint8_t cur = 0;
    uint8_t this_caller_handle = 0xff;
    uint8_t copy_size = 0;

    mutex_take(p_fifo->fifo_lock, portMAX_DELAY );
    for (cur = 0; cur < p_fifo->cur_size; cur ++)
    {
        this_caller_handle = *(uint8_t *)(p_fifo->elements[i].head);
        //debug_printf("i=%u this_caller_handle=%u\n", i, this_caller_handle);
        if (this_caller_handle == caller_handle) {
            //debug_printf("Up is the target handle\n");
            break;
        }

        i = (i+1) % p_fifo->size;
    }

    
    if (p_fifo->rear == 0)
        p_fifo->rear = p_fifo->size - 1;
    else
        p_fifo->rear --;
    //    printf("This is not first node, rear-- ,rear =%u\n", p_fifo->rear);


    if (cur != p_fifo->cur_size) {
        memcpy((uint8_t *)data, p_fifo->elements[i].head, p_fifo->element_size);
        event_fifo_mark_mem_released(p_fifo, p_fifo->elements[i].mem_index);

        /* Shift elements behand target */
        for (; cur < p_fifo->cur_size - 1; cur ++) {
            p_fifo->elements[i] = p_fifo->elements[(i + 1) % p_fifo->size];
            i = (i + 1) % p_fifo->size;
        }

        p_fifo->cur_size --;
        mutex_give(p_fifo->fifo_lock);
        return true;
    }
    else {
        mutex_give(p_fifo->fifo_lock);
        return false;
    }

}

void event_fifo_traverse(event_fifo_handle * p_fifo)
{
    uint8_t i = p_fifo->front;
    uint8_t cur = 0;

    for (cur = 0; cur < p_fifo->cur_size; cur ++)
    {
        printf("[0x%08X %u] <- ", p_fifo->elements[i].head, *(uint8_t *)p_fifo->elements[i].head);
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

#define test_element_type event_indication
int fifo_main_test()
{
    event_fifo_handle nwk_confirm_fifo;
    event_fifo_handle nwk_indication_fifo;

    event_fifo_init(&nwk_confirm_fifo, confirm_ptr_area, confirm_big_mem, CONFIRM_FIFO_SIZE, sizeof(event_confirm));
    event_fifo_init(&nwk_indication_fifo, indication_ptr_area, indication_big_mem, INDICATION_FIFO_SIZE, sizeof(event_indication));

    uint32_t t0,t1;
    for (int i = 0; i < 5; i ++) {
        test_element_type c;
        c.caller_handle = i;
        t0 = micros();
        event_fifo_in(&nwk_confirm_fifo, &c);
        t1 = micros();
        event_fifo_traverse(&nwk_confirm_fifo);
        debug_printf("(Dur:%lu)\n", t1 - t0);
    }

    test_element_type c;
    uint8_t target_caller_handle = 2;
    printf("\nFetch caller_handle %u\n", target_caller_handle);

    t0 = micros();
    if (event_fifo_fetch_node(&nwk_confirm_fifo, target_caller_handle, &c)) {
        t1 = micros();
        printf("Fetch success, confirm_event -> caller_handle=%u dur = %lu\n", c.caller_handle, t1 - t0);
    }
    printf("\n");


    printf("Try to fifo out\n");
    bool result = false;
    do {
        test_element_type c;
        t0 = micros();
        result = event_fifo_out(&nwk_confirm_fifo, &c);
        t1 = micros();
        printf("outresult=%u event_confirm.caller_handle=%u dur=%lu\n", result, c.caller_handle, t1 - t0);
        event_fifo_traverse(&nwk_confirm_fifo);
        printf("\n");
    }
    while(result);


    printf("Done!\n");

    return 0;
}
#endif