#include "rx_fifo.h"
#include "NRF24Zigbee.h"


inline void copy_node(rx_node_handle *dst, rx_node_handle *src)
{
    memcpy(dst, src, sizeof(rx_node_handle));
}
 
void fifo_init(rx_fifo_handle * p_fifo, rx_node_handle *elements, uint8_t size)
{
    p_fifo->elements = elements;
    p_fifo->front = 0;
    p_fifo->rear = 0;
    p_fifo->size = size;
    p_fifo->cur_size = 0;
}


inline bool fifo_is_full(rx_fifo_handle * p_fifo)
{
    return p_fifo->cur_size >= p_fifo->size;
}

inline bool fifo_is_empty(rx_fifo_handle * p_fifo)
{
    return p_fifo->cur_size == 0;
}

bool fifo_in(rx_fifo_handle * p_fifo, rx_node_handle * value)
{
    if (fifo_is_full(p_fifo)) {
        rx_node_handle *useless;
        fifo_out(p_fifo, &useless);
    }

    //printf("Insert node\n");
    copy_node(&p_fifo->elements[p_fifo->rear], value);
    p_fifo->rear = (p_fifo->rear+1) % p_fifo->size;
    p_fifo->cur_size ++;
    return true;
}

bool fifo_out(rx_fifo_handle * p_fifo, rx_node_handle ** p_element)
{
    if(fifo_is_empty(p_fifo))
    {
        //printf("Fifo is empty, failed to fifo_out\n");
        return false;
    }
    else
    {
        /* printf("Out node\n"); */
        if (p_element != NULL)
            *p_element = &p_fifo->elements[p_fifo->front];
        p_fifo->front = (p_fifo->front+1) % p_fifo->size;
        p_fifo->cur_size --;
        return true;
    }  
}

bool fifo_top(rx_fifo_handle *p_fifo, rx_node_handle ** p_element)
{
    if(fifo_is_empty(p_fifo))
    {
        return false;
    }
    else
    {
        if (p_element != NULL)
            *p_element = &p_fifo->elements[p_fifo->front];
        return true;
    }  
}

void fifo_traverse(rx_fifo_handle * p_fifo)
{
    uint8_t i = p_fifo->front;
    uint8_t cur = 0;

    printf("Traverse fifo:\n");
    for (cur = 0; cur < p_fifo->cur_size; cur ++)
    {
        printf("[0x%04X:0x%02X:%u] <=", *(uint16_t *)p_fifo->elements[i].src_addr, 
            p_fifo->elements[i].packet_index, p_fifo->elements[i].node_status);
        i = (i+1) % p_fifo->size;
    }
    printf("\n");
}

rx_node_handle * fifo_find_node(rx_fifo_handle * p_fifo,
                                        uint8_t * src_addr, uint8_t packet_index)
{
    uint8_t i = p_fifo->front;
    uint8_t cur = 0;

    //printf("fifo_find_node() param: src_addr=0x%04X, packet_index=0x%02X\n", *(uint16_t *)src_addr, packet_index);
    for (cur = 0; cur < p_fifo->cur_size; cur ++)
    {
        //printf("cur=%u src_addr=0x%04X packet_index=0x%02X\n", cur, *(uint16_t *)p_fifo->elements[i].src_addr, p_fifo->elements[i].packet_index);
        if (*(uint16_t *)p_fifo->elements[i].src_addr == *(uint16_t *)src_addr && 
                            p_fifo->elements[i].packet_index == packet_index) {
            //printf("fifo_find_node() : find node\n");
            return &p_fifo->elements[i];
        }
        i = (i+1) % p_fifo->size;
    }
    //printf("fifo_find_node() : no node found!\n");
    return NULL;
}