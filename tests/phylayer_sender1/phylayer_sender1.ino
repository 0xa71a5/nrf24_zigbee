#include <NRF24Zigbee.h>


static const uint8_t mac_addr_length = 5;
static uint8_t last_mac_addr[5] = {0};

static uint8_t phy_layer_addr[2];

inline void phy_layer_set_src_addr(uint8_t src_addr[2])
{
  SRC_ADDR_COPY(phy_layer_addr, src_addr);
}

inline void phy_layer_get_src_addr(uint8_t src_addr[2])
{
  SRC_ADDR_COPY(src_addr, phy_layer_addr);
}

void phy_layer_set_dst_addr(uint8_t *addr, uint8_t length)
{
  if (length == 5)
    nrf_set_tx_addr(addr);
  else {
    pr_err("Temp not support non-5byte addr\n");
  }
}

bool phy_layer_send_slice_packet(phy_packet_handle * packet)
{
  nrf_reliable_send((uint8_t *)packet);
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
    phy_layer_send_slice_packet(packet);
    //phy_packet_trace(packet ,0);
  }

  packet_index = (packet_index + 1) % MAX_PACKET_INDEX;
}

void setup()
{
  Serial.begin(500000);
  printf_begin();
  printf("Begin config!");
  nrf_gpio_init(8, 9); //Set ce pin and csn pin
  nrf_set_tx_addr((uint8_t *)"mac01");
  nrf_set_rx_addr((uint8_t *)"mac07");
  phy_layer_set_src_addr("07");
  nrf_chip_config(12, 32); // Set channel and payload
  nrf_set_retry_times(5);
  nrf_set_retry_durtion(1250);
  nrf_set_channel(100);
  randomSeed(analogRead(A0)^analogRead(A1));
  while(1);
}

int count = 0;
int start = 0;
uint8_t seq_num = 0;
uint8_t failed_times = 0;
uint32_t loop_time = 0;
#define MAX_RETRY_TIME 10


void loop()
{
  #define test_size 100
  uint8_t data[128];
  for (int i = 0; i < test_size; i++)
    data[i] = i;
  pr_info("Call phy_layer_send_raw_data\n\n");
  phy_layer_send_raw_data("mac01", data, test_size);
  delay(200);
}
