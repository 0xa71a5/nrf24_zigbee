#include "nz_phy_layer.h"
#include "NRF24Zigbee.h"

static rx_node_handle rx_fifo_mem[MAX_FIFO_SIZE];/* This consumes 528 bytes */
static rx_fifo_handle fifo_instance;

static const uint8_t mac_addr_length = 5;
static uint8_t last_mac_addr[5] = {0}; /* This may replace by a one-byte crc check */
static uint8_t phy_layer_src_addr[2];

bool phy_layer_init(uint8_t *src_addr)
{
  uint8_t mac_addr[5] = {'m', 'a', 'c', 'z', 'z'};
  nrf_gpio_init(CE_PIN, CSN_PIN); //Set ce pin and csn pin

  //nrf_set_tx_addr((uint8_t *)"mac01");
  SRC_ADDR_COPY(phy_layer_src_addr, src_addr);
  mac_addr[3] = src_addr[0];
  mac_addr[4] = src_addr[1];
  nrf_set_rx_addr((uint8_t *)mac_addr);
  phy_layer_set_src_addr(phy_layer_src_addr);
  nrf_chip_config(CHANNEL, PAYLOAD_LENGTH); // Set channel and payload
  nrf_set_retry_times(RETRY_TIMES);
  nrf_set_retry_durtion(RETRY_DURTION);
  randomSeed(analogRead(A0)^analogRead(A1));

  fifo_init(&fifo_instance, rx_fifo_mem, MAX_FIFO_SIZE);
  enable_rx();
  return true;
}

bool phy_layer_data_ready(void)
{
  return nrf_data_ready();
}

void phy_packet_trace(phy_packet_handle * packet, uint8_t mode = 0)
{
  if (mode == 0) {
    pr_debug("######## Phy packet trace ########\n");
    pr_debug("type        :%4u\n", packet->type);
    pr_debug("length      :%4u\n", packet->length);
    pr_debug("packet_index:%4u\n", packet->packet_index);
    pr_debug("slice_size  :%4u\n", packet->slice_size);
    pr_debug("slice_index : %4u\n", packet->slice_index);
    pr_debug("src_addr    :0x%04X\n", *(uint16_t *)packet->src_addr);
    pr_debug("crc         :0x%02X\n", packet->crc);
    pr_debug("data        :");
    for (int i = 0; i < packet->length; i ++) {
      if (i % 10 == 0)
        debug_printf("\n");
      debug_printf("0x%02X ", packet->data[i]);
    }  
  }
  else {
    uint8_t *ptr = (uint8_t *)packet;
    pr_info("######## Phy packet trace ########\n");
    for (int i = 0; i < 32; i ++) {
      if (i % 10 == 0)
        debug_printf("\n");
      debug_printf("0x%02X ", ptr[i]);
    }
  }
  debug_printf("\n\n");
}

void phy_layer_reset_node(rx_node_handle *node)
{
  node->node_status = NODE_INVALID;
  node->recv_chain = 0x00;
}

bool phy_layer_send_slice_packet(phy_packet_handle * packet, uint32_t max_retry = SOFTWARE_RETRY_RATIO)
{
  nrf_reliable_send((uint8_t *)packet, 32, max_retry);
}

void phy_layer_listener(void)
{
  static uint8_t raw_data[32];
  static phy_packet_handle * packet = (phy_packet_handle *)raw_data;
  static rx_node_handle * phy_rx_node = NULL;
  static uint8_t in_receive_state = 0;
  static uint16_t packet_receive_duration = 0;
  static uint16_t packet_max_duration = 0;
  
  static uint8_t last_slice_index = 0;
  static uint16_t last_src_addr = 0;
  static uint8_t last_packet_index = 0;

  /* Listener shall check if got any rx data from phy */

  /* TODO: Add timeout check */
  if (in_receive_state && packet_receive_duration > packet_max_duration) {
    pr_err("Time out for this message chain reception.\n");
    /* Do something for timeout handle */
  }

  if (phy_layer_data_ready()) {
    uint8_t status = read_register(STATUS);
    uint8_t pipe = (status >> 1) & 0x07; /* Get what pipe channel is
                                         this data from */
    nrf_get_data(raw_data);
    //phy_packet_trace(packet, 0);

    if (crc_calculate((uint8_t *)packet, PHY_PACKET_HEADER_SIZE) != packet->crc) {
        pr_err("Recv packet crc check err, raw=0x%02X calc=0x%02X, drop it!\n", packet->crc, crc_calculate((uint8_t *)packet, 2));
        //phy_packet_trace(packet, 1);
        return ;
    }

    if (packet->slice_index == last_slice_index && packet->src_addr == last_src_addr
          && packet->packet_index == last_packet_index) {
      pr_err("Repeat packs\n");
      goto exit; /* We omit this repeat packet */
    }

    if (packet->type == MESSAGE_PACKET) {
      /* Check if this is a repeat pack */

      phy_rx_node = fifo_find_node(&fifo_instance, packet->src_addr, packet->packet_index);

      if (!phy_rx_node) {
        rx_node_handle tmp_fifo_node;
        SRC_ADDR_COPY(tmp_fifo_node.src_addr, packet->src_addr);
        tmp_fifo_node.packet_index = packet->packet_index;
        /* Create a new node which doesnt exsits before */
        //pr_debug("packet->src_addr = 0x%04X\n", *(uint16_t *)packet->src_addr);
        //pr_debug("Push new node into fifo, src_addr=0x%04X, packet_index=0x%02X\n", *(uint16_t *)tmp_fifo_node.src_addr, tmp_fifo_node.packet_index);
        fifo_in(&fifo_instance, &tmp_fifo_node);        
        phy_rx_node = fifo_find_node(&fifo_instance, packet->src_addr, packet->packet_index);
        phy_layer_reset_node(phy_rx_node);
      }

      if (packet->slice_index == 0) { /* This is the first slice */
          pr_debug("First %u/%u Mac:0x%04X Index:%u\n", packet->slice_index, 
            packet->slice_size - 1, *(uint16_t *)packet->src_addr, packet->packet_index);
          phy_layer_reset_node(phy_rx_node);
      }
      /* If this is the last slice or timeout , we shall send 
                                              control msg to sender */
      /* Don't use 'else' ,cause slice_size could be 1 */
      if (packet->slice_index == packet->slice_size-1) { 
          phy_rx_node->recv_chain |= (1 << (uint8_t)packet->slice_index);
          pr_debug("Last  %u/%u Mac:0x%04X Index:%u\n", packet->slice_index, 
            packet->slice_size - 1, *(uint16_t *)packet->src_addr, packet->packet_index);
            /* Check if all slices received and send ack to sender .
             If not all slices received ,we have two ways to handle this 
             situation.One is just drop all slices and send a fail signal to 
             sender , the other one is that we send missed slice indexs to 
             sender and hope it will then sends those slices.          
          */
          uint8_t expect_status = (uint8_t)0xff >> (8 - packet->slice_size);
          //phy_layer_set_tx_addr()
          if (phy_rx_node->recv_chain != expect_status) {
            /* Some slices missed, here we dont want a ack or any compensate */
            uint8_t missed_slices = phy_rx_node->recv_chain;
            phy_rx_node->node_status = NODE_INVALID;
            /* TODO: Invalid packets are to removed from fifo */
            pr_err("expect_status=0x%02X, missed_slices=0x%02X\n", 
                                              expect_status, missed_slices);
          }
          else {
            /* Make best efforts to send success ack to sender */
            uint8_t packet_index = packet->packet_index;
            phy_rx_node->node_status = NODE_VALID;
            phy_rx_node->length = (packet->slice_size-1) * MAX_PACKET_DATA_SIZE +
                                    packet->slice_size;
            //pr_info("Rece all succ\n");
          }
          fifo_traverse(&fifo_instance);
          debug_printf("\n");
      }
      /* This is middle slices, 
         check if this node's first slice exists */
      else {
        if (packet->slice_index)
          pr_debug("Mid   %u/%u Mac:0x%04X Index:%u\n", packet->slice_index, 
            packet->slice_size - 1, *(uint16_t *)packet->src_addr, packet->packet_index);
      }

      /* Parse recv packet and fetch data into fifo node */
      if (!(phy_rx_node->recv_chain & (1 << (uint8_t)packet->slice_index))) {
        /* This slice doesnt exsits */
        uint16_t offset = packet->slice_index * MAX_PACKET_DATA_SIZE;
        memcpy(phy_rx_node->data + offset, packet->data, packet->length);
        phy_rx_node->recv_chain |= (1 << (uint8_t)packet->slice_index);
      }
    }
    else {
      pr_err("Cant handle this type message : type 0x%02X\n", packet->type);
    }
    
    exit:

    last_slice_index = packet->slice_index;
    last_src_addr = packet->src_addr;
    last_packet_index = packet->packet_index;
    return ;
  }
}

uint16_t phy_layer_fifo_endpoint_size(void)
{
  rx_node_handle * node;

  if (fifo_top(&fifo_instance, &node) && node->node_status == NODE_VALID) {
    return node->length;
  }
  return 0;
}

/* This function get receive data from local rx fifo.
 * @Param data: to store raw_data
 * @Param max_length: data max length to get,default to 128
 * @Return : data amount get at last 
 */
uint16_t phy_layer_fifo_pop_data(uint8_t *data, uint16_t max_length = 128)
{
  rx_node_handle * node;

  if (fifo_out(&fifo_instance, &node) && node->node_status == NODE_VALID) {
    uint16_t copy_length = node->length <= max_length ? node->length : max_length;
    memcpy(data, node->data, copy_length);
    return copy_length;
  }
  pr_debug("fifo empty ,pop no data\n");
  return 0;
}

void phy_layer_set_src_addr(uint8_t src_addr[2])
{
  SRC_ADDR_COPY(phy_layer_src_addr, src_addr);
}

void phy_layer_get_src_addr(uint8_t src_addr[2])
{
  SRC_ADDR_COPY(src_addr, phy_layer_src_addr);
}

void phy_layer_set_dst_addr(uint8_t *addr, uint8_t length)
{
  if (length == 5)
    nrf_set_tx_addr(addr);
  else {
    pr_err("Temp not support non-5byte addr\n");
  }
}


bool phy_layer_send_raw_data(uint8_t *dst_mac_addr, uint8_t *raw_data, uint32_t length)
{
  uint8_t compare_flag = 0;
  uint8_t i;
  static uint8_t packet_index = 0;
  uint8_t packet_mem[32] = {0};
  phy_packet_handle * packet = (phy_packet_handle *)packet_mem;
  uint8_t packet_send_status = 0x00;
  uint8_t *data_offset = raw_data; /* Offset ptr for raw_data*/

  packet->type = MESSAGE_PACKET;
  packet->packet_index = packet_index;
  packet->slice_size = length / MAX_PACKET_DATA_SIZE + 
                      ((length % MAX_PACKET_DATA_SIZE) != 0);
  phy_layer_get_src_addr(packet->src_addr);

  for (i = 0; i < mac_addr_length; i ++)
    if (dst_mac_addr[i] != last_mac_addr[i]) {
      compare_flag = 1;
      last_mac_addr[i] = dst_mac_addr[i];
    }

  if (compare_flag) {
    pr_debug("Tx addr not the same as last one,write new addr\n");
    phy_layer_set_dst_addr(dst_mac_addr, mac_addr_length);
  }

  /* Slice 128 byte data to multiple parts, each one's max length is 29 byte */
  for (i = 0; i < packet->slice_size; i ++) {
    if (i != packet->slice_size - 1) /* If not the last pack, than 
                                                choose the max data size */
    {
      packet->length = MAX_PACKET_DATA_SIZE;
    }
    else /*If is the last pack, choose remain not sending data size as length */
    {
      packet->length = length % MAX_PACKET_DATA_SIZE; 
    }  
   
    packet->slice_index = i;
    /* We just calculate header crc */
    packet->crc = crc_calculate((uint8_t *)packet, PHY_PACKET_HEADER_SIZE);
    memcpy(packet->data, data_offset, packet->length);
    data_offset += packet->length;
    //phy_packet_trace(packet);
    phy_layer_send_slice_packet(packet, SOFTWARE_RETRY_RATIO);
    //phy_packet_trace(packet ,0);
  }

  packet_index = (packet_index + 1) % MAX_PACKET_INDEX;
}
