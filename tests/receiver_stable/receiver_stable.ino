#include <lxc_nrf24l01.h>

int serial_putc( char c, struct __file * )
{
  Serial.write( c );
  return c;
}
void printf_begin(void)
{
  fdevopen( &serial_putc, 0 );
}

void setup()
{
  Serial.begin(115200);
  printf_begin();
  Serial.println("Begin config!");
  nrf_gpio_init(8, 9); //Set ce pin and csn pin
  nrf_set_tx_addr((uint8_t *)"mac00");
  nrf_set_rx_addr((uint8_t *)"mac01");
  nrf_chip_config(12, 32); // Set channel and payload
  nrf_set_retry_times(5);
  nrf_set_retry_durtion(750);
  nrf_set_channel(100);
  Serial.println("Begining!");
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
    nrf_get_data(data);
    if (millis() - last_check_time > RATE_SAMPLE_TIME) {
      comm_rate = (1000 * (comm_sum - last_check_sum)) >> RATE_SAMPLE_TIME_SHIFT;
      last_check_sum = comm_sum;
      last_check_time = millis();
    }

    send_id = data[29] < 10 ? data[29] : 0x00;
    seq_num = data[30];
    crc = data[31];
    crc_check = crc_calculate(data, 31);

    if (crc == crc_check) {
      //if(print_count ++ % 50 == 0)
        printf("Recv[%u] [%3u] [%4u B/s]\n", send_id, (uint8_t)seq_num, comm_rate);
      if ((uint8_t)seq_num == last_seq_num[send_id])
        printf("[%u]Repeat seq num %u!\n", send_id, seq_num);
      else if ((uint8_t)seq_num != ((last_seq_num[send_id] + 1) % 256))
        printf("[%u]Lost frame! cur=%u last=%u############\n", send_id, (uint8_t)seq_num, last_seq_num[send_id]);
      last_seq_num[send_id] = seq_num;
      
      comm_sum += 32;
    }
    else {
      printf("[%u] [%3u]Wrong crc 0x%02X != 0x%02X\n", send_id, seq_num, data, crc, crc_check);
    }
  
  }
  
}

