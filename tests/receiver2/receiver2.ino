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

void print_info()
{
  printf("EN_AA     = 0x%02X\n", read_register(EN_AA));
  printf("EN_RXADDR = 0x%02X\n", read_register(EN_RXADDR));
  printf("RX_ADDR_P2= '%c' RX_PW_P2=%u\n", read_register(RX_ADDR_P2), read_register(RX_PW_P2));
  printf("RX_ADDR_P3= '%c' RX_PW_P3=%u\n", read_register(RX_ADDR_P3), read_register(RX_PW_P3));
  printf("RX_ADDR_P4= '%c' RX_PW_P4=%u\n", read_register(RX_ADDR_P4), read_register(RX_PW_P4));
  delay(1000);  
}

void setup()
{
  Serial.begin(115200);
  printf_begin();
  Serial.println("Begin config!");
  nrf_gpio_init(8, 9); //Set ce pin and csn pin
  nrf_set_tx_addr((uint8_t *)"mac00");
  nrf_set_rx_addr((uint8_t *)"mac03");
  config_register(RX_ADDR_P2, (uint8_t)'a');
  config_register(RX_ADDR_P3, (uint8_t)'b');
  config_register(RX_ADDR_P4, (uint8_t)'c');
  
  nrf_chip_config(12, 32); // Set channel and payload
  nrf_set_retry_times(5);
  nrf_set_retry_durtion(750);
  nrf_set_channel(100);
  Serial.println("Zigbee network starts!");
  print_info();
  enable_rx();
}

uint32_t comm_rate = 0;
uint32_t comm_sum = 0;
uint32_t last_check_sum = 0;
uint32_t last_check_time = 0;

uint8_t last_seq_num[10] = {0}; /* Care that only 10 node addr allowed! */
uint8_t data[32];
uint32_t print_count = 0;

#define RATE_SAMPLE_TIME 128
#define RATE_SAMPLE_TIME_SHIFT 7

#define MAX_RETRY_TIME 10

uint8_t seq_num = 0;
uint8_t send_id = 0;
uint8_t crc = 0;
uint8_t crc_check = 0;
void loop()
{
  if (nrf_data_ready()) {
    uint8_t status = read_register(STATUS);
    uint8_t pipe = (status >> 1) & 0x07;
    nrf_get_data(data);
   
    send_id = data[29] < 10 ? data[29] : 0x00;
    seq_num = data[30];
    crc = data[31];
    crc_check = crc_calculate(data, 31);

    
    if (crc == crc_check) {
        printf("Recv pipe:%u msg:[%s]\n", pipe, data);
    }
    else {
      printf("[%u] [%3u]Wrong crc 0x%02X != 0x%02X\n", send_id, seq_num, data, crc, crc_check);
    }
  
  }
  
}

