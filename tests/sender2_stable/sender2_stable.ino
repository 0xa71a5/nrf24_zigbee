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

uint8_t send_id = 0x00;

void setup()
{
  Serial.begin(115200);
  printf_begin();
  Serial.println("Begin config!");
  nrf_gpio_init(8, 9); //Set ce pin and csn pin
  nrf_set_tx_addr((uint8_t *)"mac01");
  nrf_set_rx_addr((uint8_t *)"mac02");
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
   uint8_t data[32];
   uint8_t tx_status = 0x00;
   sprintf((char *)data, "Packet %d", count++);
   data[29] = send_id;
   data[30] = seq_num ++;
   data[31] = crc_calculate(data, 31);
   
   printf("[%u] [%3u] Send\n", send_id, seq_num);
   if (!nrf_reliable_send(data))
      printf("Send failed!\n");
   delay(10);
   if (tx_status != TX_REACH_DST)
      printf("Send failed with too many retries\n");

   if (nrf_data_ready()) {
      nrf_get_data(data);
      //printf("Recv->%s\n\n", data);
  }

  if(loop_time ++ % 10 == 0)
      delay(random(1000));
}

