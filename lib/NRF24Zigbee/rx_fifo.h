#ifndef RX_FIFO_H
#define RX_FIFO_H
#include <stdint.h>

enum fifo_status {
  NODE_INVALID = 0,
  NODE_VALID
};

typedef struct __rx_node_handle {
  uint8_t node_status:5;
  uint8_t packet_index:3;
  uint8_t recv_chain;
  uint8_t length;
  uint8_t src_addr[2];
  uint8_t crc;
  uint8_t last_start_time; /* This variable used to check packet timeout, 254 ms is the max timeout */
  uint8_t data[128];
} rx_node_handle;

typedef struct __rx_fifo_handle
{  
    rx_node_handle * elements;
    uint8_t front;
    uint8_t rear;
    uint8_t size;
    uint8_t cur_size;
} rx_fifo_handle;

#define MAX_FIFO_SIZE 4
#define SRC_ADDR_COPY(dst,src) (*(uint16_t *)dst) = (*(uint16_t *)src)

inline void copy_node(rx_node_handle *dst, rx_node_handle *src);
void fifo_init(rx_fifo_handle * p_fifo, rx_node_handle *elements, uint8_t size);
inline bool fifo_is_full(rx_fifo_handle * p_fifo);
inline bool fifo_is_empty(rx_fifo_handle * p_fifo);
bool fifo_in(rx_fifo_handle * p_fifo, rx_node_handle * value);
bool fifo_out(rx_fifo_handle * p_fifo, rx_node_handle ** p_element);
bool fifo_top(rx_fifo_handle *p_fifo, rx_node_handle ** p_element);
void fifo_traverse(rx_fifo_handle * p_fifo);
rx_node_handle * fifo_find_node(rx_fifo_handle * p_fifo, uint8_t * src_addr, uint8_t packet_index);

#endif