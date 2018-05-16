#ifndef NZ_PHY_LAYER_H
#define NZ_PHY_LAYER_H
#include "NRF24Zigbee.h"

#define CE_PIN 8
#define CSN_PIN 9
#define CHANNEL 100 /* a channel takes 1MHZ bandwidth */
#define PAYLOAD_LENGTH 32 /* byte */
#define RETRY_TIMES 5
#define RETRY_DURTION 1000 /*us*/
#define SOFTWARE_RETRY_RATIO 10 /* REAL_RETRY_TIMES = SOFTWARE_RETRY_RATIO * RETRY_TIMES */

#define PUBLIC_MAC_ADDR_0 'm'
#define PUBLIC_MAC_ADDR_1 'a'
#define PUBLIC_MAC_ADDR_2 'c'
/*
 * PUBLIC_MAC_ADDR_3 and PUBLIC_MAC_ADDR_4 are to configured by user.
 */

#define BROADCAST_ADDR_BYTE0 0xff

#define SYS_RAM_TRACE() //free_ram_print()

bool phy_layer_init(uint16_t src_addr_u16);
bool phy_layer_data_ready(void);
void phy_packet_trace(phy_packet_handle * packet, uint8_t mode);
void phy_layer_reset_node(rx_node_handle *node);
bool phy_layer_send_slice_packet(phy_packet_handle * packet, uint32_t max_retry);
void phy_layer_listener(void);
uint16_t phy_layer_fifo_top_node_size(void);
uint16_t phy_layer_fifo_pop_data(uint8_t *data, uint16_t max_length = 128);
void phy_layer_set_src_addr(uint8_t src_addr[2]);
void phy_layer_get_src_addr(uint8_t src_addr[2]);
void phy_layer_set_dst_addr(uint16_t addr);
bool phy_layer_send_raw_data(uint16_t dst_mac_addr_u16, uint8_t *raw_data, uint16_t length);
uint16_t phy_layer_fifo_availables(void);
void phy_layer_event_process(void *params);
#endif