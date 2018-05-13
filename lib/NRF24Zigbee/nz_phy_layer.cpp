#include "nz_phy_layer.h"
#include "NRF24Zigbee.h"
#include <FreeRTOS_AVR.h>

static rx_node_handle rx_fifo_mem[MAX_FIFO_SIZE];/* This consumes 528 bytes */
static rx_fifo_handle fifo_instance;

static const uint8_t mac_addr_length = 2;
static uint8_t last_mac_addr[2] = {0}; /* This may replace by a one-byte crc check */
static uint8_t phy_layer_src_addr[2];

static SemaphoreHandle_t rf_chip_use;
static SemaphoreHandle_t phy_rx_fifo_sem;

bool phy_layer_init(uint8_t *src_addr)
{
  uint8_t mac_addr[5] = {PUBLIC_MAC_ADDR_0, PUBLIC_MAC_ADDR_1, PUBLIC_MAC_ADDR_2, 'z', 'z'};
  nrf_gpio_init(CE_PIN, CSN_PIN); //Set ce pin and csn pin
  SYS_RAM_TRACE();
  //nrf_set_tx_addr((uint8_t *)"mac01");
  SRC_ADDR_COPY(phy_layer_src_addr, src_addr);
  mac_addr[3] = src_addr[0];
  mac_addr[4] = src_addr[1];
  nrf_set_rx_addr((uint8_t *)mac_addr);
  nrf_set_broadcast_addr(BROADCAST_ADDR_BYTE0);
  phy_layer_set_src_addr(phy_layer_src_addr);

  rf_chip_use = xSemaphoreCreateCounting(1, 1);
  phy_rx_fifo_sem = xSemaphoreCreateCounting(1, 1);

  nrf_chip_config(CHANNEL, PAYLOAD_LENGTH); // Set channel and payload
  nrf_set_retry_times(RETRY_TIMES);
  nrf_set_retry_durtion(RETRY_DURTION);
  randomSeed(analogRead(A0)^analogRead(A1));
  SYS_RAM_TRACE();
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

void phy_layer_test_and_copy(phy_packet_handle *packet, rx_node_handle *phy_rx_node)
{
  uint16_t offset = packet->slice_index * MAX_PACKET_DATA_SIZE;
  memcpy(phy_rx_node->data + offset, packet->data, packet->length);
  phy_rx_node->recv_chain |= (1 << (uint8_t)packet->slice_index);
}


void phy_layer_listener(void)
{
  uint8_t raw_data[32];
  phy_packet_handle * packet = (phy_packet_handle *)raw_data;
  rx_node_handle * phy_rx_node = NULL;
  uint8_t last_slice_index = 0;
  uint16_t last_src_addr = 0;
  uint8_t last_packet_index = 0;


  /* Do a fifo node valid test, if the top node is invalid and timeout, pop it */
  rx_node_handle *node = NULL;
  if (fifo_top(&fifo_instance, &node) && node->node_status == NODE_INVALID) {
    uint8_t cur_time = millis();
    uint16_t durtion = 0;
    if (cur_time < node->last_start_time)
      durtion = cur_time + 255 - (uint16_t)node->last_start_time;
    else
      durtion = cur_time - node->last_start_time;
    //debug_printf("### %u\n", durtion);
    if (durtion > 12)
      fifo_out(&fifo_instance, NULL); 
  }

  /* Listener shall check if got any rx data from phy */
  xSemaphoreTake(rf_chip_use, portMAX_DELAY);
  if (phy_layer_data_ready()) {
    uint8_t status = read_register(STATUS);
    uint8_t pipe = (status >> 1) & 0x07; /* Get what pipe channel is
                                         this data from */
    nrf_get_data(raw_data);
    xSemaphoreGive(rf_chip_use);

    DISBALE_LOG_OUTPUT();

    if (crc_calculate((uint8_t *)packet, PHY_PACKET_HEADER_SIZE) != packet->crc) {
        pr_err("Recv packet crc check err, raw=0x%02X calc=0x%02X, drop it!\n", packet->crc, crc_calculate((uint8_t *)packet, 2));
        //phy_packet_trace(packet, 1);
        goto exit;
    }

    if (packet->slice_index == last_slice_index && packet->src_addr == last_src_addr
          && packet->packet_index == last_packet_index) {
      pr_err("Repeat packs\n");
      goto exit; /* We omit this repeat packet */
    }

    if (packet->type == MESSAGE_PACKET) {
      /* Check if this is a repeat pack */
      SYS_RAM_TRACE();
      phy_rx_node = fifo_find_node(&fifo_instance, packet->src_addr, packet->packet_index);
      SYS_RAM_TRACE();
      if (!phy_rx_node) {
        rx_node_handle tmp_fifo_node;
        SRC_ADDR_COPY(tmp_fifo_node.src_addr, packet->src_addr);
        tmp_fifo_node.packet_index = packet->packet_index;
        /* Create a new node which doesnt exsits before */
        //pr_debug("packet->src_addr = 0x%04X\n", *(uint16_t *)packet->src_addr);
        //pr_debug("Push new node into fifo, src_addr=0x%04X, packet_index=0x%02X\n", *(uint16_t *)tmp_fifo_node.src_addr, tmp_fifo_node.packet_index);
        xSemaphoreTake(phy_rx_fifo_sem, portMAX_DELAY);
        fifo_in(&fifo_instance, &tmp_fifo_node);
        xSemaphoreGive(phy_rx_fifo_sem);

        phy_rx_node = fifo_find_node(&fifo_instance, packet->src_addr, packet->packet_index);
        phy_layer_reset_node(phy_rx_node);
      }

      if (packet->slice_index == 0) { /* This is the first slice */
          pr_debug("First %u/%u 0x%04X %u\n", packet->slice_index, 
            packet->slice_size - 1, *(uint16_t *)packet->src_addr, packet->packet_index);
          phy_layer_reset_node(phy_rx_node);
          phy_layer_test_and_copy(packet, phy_rx_node);
          phy_rx_node->last_start_time = (uint8_t)millis();
      }
      /* If this is the last slice or timeout , we shall send 
                                              control msg to sender */
      /* Don't use 'else' ,cause slice_size could be 1 */
      if (packet->slice_index == packet->slice_size-1) { 
          phy_rx_node->recv_chain |= (1 << (uint8_t)packet->slice_index);
          pr_debug("Last  %u/%u 0x%04X %u\n", packet->slice_index, 
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
            rx_node_handle * node = NULL;
            uint8_t missed_slices = phy_rx_node->recv_chain;

            phy_rx_node->node_status = NODE_INVALID;

            /* Pop out this invalid packet if it is on top */
            if (fifo_top(&fifo_instance, &node) && node->packet_index == packet->packet_index
              && *(uint16_t *)node->src_addr == *(uint16_t *)packet->src_addr) {
                xSemaphoreTake(phy_rx_fifo_sem, portMAX_DELAY);
                fifo_out(&fifo_instance, NULL);
                xSemaphoreGive(phy_rx_fifo_sem);
            }

            ENABLE_LOG_OUTPUT();
            /* TODO: Invalid packets are to removed from fifo */
            pr_err("expect_status=0x%02X, missed_slices=0x%02X\n", 
                                              expect_status, missed_slices);
            DISBALE_LOG_OUTPUT();
          }
          else {
            /* Make best efforts to send success ack to sender */
            uint8_t packet_index = packet->packet_index;

            phy_layer_test_and_copy(packet, phy_rx_node);

            phy_rx_node->node_status = NODE_VALID;
            phy_rx_node->length = (packet->slice_size-1) * MAX_PACKET_DATA_SIZE + packet->length;
          }
          fifo_traverse(&fifo_instance);
          SYS_RAM_TRACE();
      }
      /* This is middle slices, 
         check if this node's first slice exists */
      else {
        if (packet->slice_index)
          pr_debug("Mid   %u/%u 0x%04X %u\n", packet->slice_index, 
            packet->slice_size - 1, *(uint16_t *)packet->src_addr, packet->packet_index);
          SYS_RAM_TRACE();
        phy_layer_test_and_copy(packet, phy_rx_node);
      }

    }
    else {
      pr_err("Cant handle this type message : type 0x%02X\n", packet->type);
    }
    exit:

    ENABLE_LOG_OUTPUT();
    last_slice_index = packet->slice_index;
    last_src_addr = packet->src_addr;
    last_packet_index = packet->packet_index;

    return ;
  }
  xSemaphoreGive(rf_chip_use);

}

uint16_t phy_layer_fifo_availables(void)
{
  uint8_t result = 0;
  
  xSemaphoreTake(phy_rx_fifo_sem, portMAX_DELAY);
  for (uint8_t i = 0; i < fifo_instance.size; i ++) {
    result += (fifo_instance.elements[i].node_status == NODE_VALID);
  }
  xSemaphoreGive(phy_rx_fifo_sem);
  return result;
}

uint16_t phy_layer_fifo_top_node_size(void)
{
  rx_node_handle * node;

  xSemaphoreTake(phy_rx_fifo_sem, portMAX_DELAY);
  if (fifo_top(&fifo_instance, &node) && node->node_status == NODE_VALID) {
    xSemaphoreGive(phy_rx_fifo_sem);
    return node->length;
  }
  xSemaphoreGive(phy_rx_fifo_sem);

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

  xSemaphoreTake(phy_rx_fifo_sem, portMAX_DELAY);
  /* Dont pop node at first cause top node maybe invalid */
  if (fifo_top(&fifo_instance, &node) && node->node_status == NODE_VALID) {
    uint16_t copy_length = node->length <= max_length ? node->length : max_length;
    memcpy(data, node->data, copy_length);
    fifo_out(&fifo_instance, &node);
    xSemaphoreGive(phy_rx_fifo_sem);
    return copy_length;
  }
  xSemaphoreGive(phy_rx_fifo_sem);
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

void phy_layer_set_dst_addr(uint8_t *addr)
{
  uint8_t mac_addr[5] = {PUBLIC_MAC_ADDR_0, PUBLIC_MAC_ADDR_1, PUBLIC_MAC_ADDR_2, addr[0], addr[1]};
  nrf_set_tx_addr(mac_addr);
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

  SYS_RAM_TRACE();
  packet->type = MESSAGE_PACKET;
  packet->packet_index = packet_index;
  packet->slice_size = length / MAX_PACKET_DATA_SIZE + 
                      ((length % MAX_PACKET_DATA_SIZE) != 0);
  phy_layer_get_src_addr(packet->src_addr);

  if (*(uint16_t *)dst_mac_addr != *(uint16_t *)last_mac_addr) {
    SRC_ADDR_COPY(last_mac_addr, dst_mac_addr);
    pr_debug("Tx addr not the same as last one,write new addr\n");
    phy_layer_set_dst_addr(dst_mac_addr);
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
    xSemaphoreTake(rf_chip_use, portMAX_DELAY);
    phy_layer_send_slice_packet(packet, SOFTWARE_RETRY_RATIO);
    xSemaphoreGive(rf_chip_use);
    //phy_packet_trace(packet ,0);
  }

  SYS_RAM_TRACE();
  packet_index = (packet_index + 1) % MAX_PACKET_INDEX;
}

void phy_layer_event_process(void *params)
{
  uint8_t data_length;
  uint8_t data[128];
  TickType_t last_wake_time;

  while (1) {
    phy_layer_listener();

    if ((data_length = phy_layer_fifo_top_node_size()) > 0) {
      data_length = phy_layer_fifo_pop_data(data);
      debug_printf("<=== read_size=%u crc_raw=0x%02X crc_calc=0x%02X \n\n", data_length, data[data_length-1], crc_calculate(data, data_length-1));
    /* TODO: Indication */
      
    }

    //last_wake_time = xTaskGetTickCount();
    //vTaskDelayUntil(&last_wake_time, 357);
    vTaskDelay(1);
  }
}