#include <NRF24Zigbee.h>
#define pr_err(...) printf("[-] ");printf(__VA_ARGS__)
#define pr_info(...) printf("[+] ");printf(__VA_ARGS__)
#define pr_debug(...) printf("[?] ");printf(__VA_ARGS__)


int serial_putc( char c, struct __file * )
{
  Serial.write( c );
  return c;
}
void printf_begin(void)
{
  fdevopen( &serial_putc, 0 );
}

uint8_t send_id = 0x00;


void print_info()
{
  printf("\n############### REG TRACE START ###############\n");
  printf("EN_AA     = 0x%02X\n", read_register(EN_AA));
  printf("EN_RXADDR = 0x%02X\n", read_register(EN_RXADDR));
  printf("RX_ADDR_P2= '%c' RX_PW_P2=%u\n", read_register(RX_ADDR_P2), read_register(RX_PW_P2));
  printf("RX_ADDR_P3= '%c' RX_PW_P3=%u\n", read_register(RX_ADDR_P3), read_register(RX_PW_P3));
  printf("RX_ADDR_P4= '%c' RX_PW_P4=%u\n", read_register(RX_ADDR_P4), read_register(RX_PW_P4));
  printf("###############  REG TRACE END  ###############\n\n");
}


static const uint8_t mac_addr_length = 5;
static uint8_t last_mac_addr[5] = {0};

void phy_packet_trace(phy_packet_handle * packet, uint8_t mode = 0)
{
  if (mode == 0) {
    pr_debug("######## Phy packet trace ########\n");
    pr_debug("type        :%4u\n", packet->type);
    pr_debug("length      :%4u\n", packet->length);
    pr_debug("packet_index:%4u\n", packet->packet_index);
    pr_debug("slice_size  :%4u\n", packet->slice_size);
    pr_debug("slice_index :%4u\n", packet->slice_index);
    pr_debug("crc         :0x%02X\n", packet->crc);
    pr_debug("data        :");
    for (int i = 0; i < packet->length; i ++) {
      if (i % 10 == 0)
        printf("\n");
      printf("0x%02X ", packet->data[i]);
    }  
  }
  else {
    uint8_t *ptr = (uint8_t *)packet;
    pr_info("######## Phy packet trace ########\n");
    for (int i = 0; i < 32; i ++) {
      if (i % 10 == 0)
        printf("\n");
      printf("0x%02X ", ptr[i]);
    }
  }
  printf("\n\n");
}

static uint8_t phy_layer_addr[2];

inline void phy_layer_set_src_addr(uint8_t src_addr[2])
{
  SRC_ADDR_COPY(phy_layer_addr, src_addr);
}

inline void phy_layer_get_src_addr(uint8_t src_addr[2])
{
  SRC_ADDR_COPY(src_addr, phy_layer_addr);
}

bool phy_layer_data_ready(void)
{
  return nrf_data_ready();
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
  nrf_set_rx_addr((uint8_t *)"mac02");
  phy_layer_set_src_addr("02");
  send_id = 0x02;
  nrf_chip_config(12, 32); // Set channel and payload
  nrf_set_retry_times(5);
  nrf_set_retry_durtion(1250);
  nrf_set_channel(100);
  randomSeed(analogRead(A0)^analogRead(A1));
  
}

int count = 0;
int start = 0;
uint8_t seq_num = 0;
uint8_t failed_times = 0;
uint32_t loop_time = 0;
#define MAX_RETRY_TIME 10


void loop()
{
  #define test_size 15
  uint8_t data[128];
  for (int i = 0; i < test_size; i++)
    data[i] = i;
  pr_info("Call phy_layer_send_raw_data\n\n");
  phy_layer_send_raw_data("mac01", data, test_size);
  delay(1000);
}

void loop_old()
{
   uint8_t data[32];
   uint8_t tx_status = 0x00;
   sprintf((char *)data, "Packet %d", count++);
   data[29] = send_id;
   data[30] = seq_num ++;
   data[31] = crc_calculate(data, 31);

   if (loop_time ++ % 2) {
      printf("Reliable send\n");
      nrf_set_tx_addr((uint8_t *)"mac01");
      if (!nrf_reliable_send(data))
        printf("Send failed!\n");
   }
   else {
      printf("Broad    send\n");
      nrf_set_tx_addr((uint8_t *)"mac0a");
      nrf_broad(data);
   }
   
  delay(300);
}

