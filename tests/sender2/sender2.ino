#include <NRF24Zigbee.h>

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


struct phy_packet_handle {
  uint8_t type:2;
  uint8_t length:5;
  uint8_t packet_index:3;
  uint8_t slice_size:3;
  uint8_t slice_index:3;
  uint8_t crc:8;
  uint8_t data[0];
};


void setup()
{
  Serial.begin(500000);
  printf_begin();
  printf("Begin config!");
  nrf_gpio_init(8, 9); //Set ce pin and csn pin
  nrf_set_tx_addr((uint8_t *)"mac01");
  nrf_set_rx_addr((uint8_t *)"mac02");
  send_id = 0x02;
  nrf_chip_config(12, 32); // Set channel and payload
  nrf_set_retry_times(5);
  nrf_set_retry_durtion(1250);
  nrf_set_channel(100);
  randomSeed(analogRead(A0)^analogRead(A1));
  print_info();
}

int count = 0;
int start = 0;
uint8_t seq_num = 0;
uint8_t failed_times = 0;
uint32_t loop_time = 0;
#define MAX_RETRY_TIME 10

void loop()
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

