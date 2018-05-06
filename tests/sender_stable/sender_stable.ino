#define PT_USE_TIMER
#define PT_USE_SEM
#include "pt.h"
#include <lxc_nrf24l01.h>

static struct pt thread1,thread2;
static struct pt_sem nrf_in_use;

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
  nrf_set_rx_addr((uint8_t *)"mac00");
  send_id = 0x00;
  nrf_chip_config(12, 32); // Set channel and payload
  nrf_set_retry_times(5);
  nrf_set_retry_durtion(1250);
  nrf_set_channel(100);
  config_register(EN_RXADDR, 0x07);//Enable rx pipe0,1,2
  config_register(0x0c, (uint8_t)'7');//pip2 addr "mac07"  
  uint8_t val =0x0;

  
  read_reg(EN_RXADDR, &val, 1);
  printf("EN_RX_ADDR =0x%02X\n", val);
  
  read_reg(RX_PW_P2, &val, 1);
  printf("Channel 2 payload =%u\n", val);
  read_reg(0x0c, &val, 1);
  printf("Pipe2 last addr byte = 0x%02X\n", val);
  read_reg(EN_AA, &val, 1);
  printf("EN_AA = 0x%02X\n", val);
  delay(2000);
  
  enable_rx();
  randomSeed(analogRead(A0)^analogRead(A1));
  PT_SEM_INIT(&nrf_in_use, 1);
  PT_INIT(&thread1);
  PT_INIT(&thread2);
}

int count = 0;
int start = 0;
uint8_t seq_num = 0;
uint8_t failed_times = 0;
uint32_t loop_time = 0;

static int thread_sender_entry(struct pt *pt)
{
   PT_BEGIN(pt);
   while (1) {
     static uint8_t data[32];
     static uint8_t tx_status = 0x00;
     sprintf((char *)data, "Packet %d", count++);
     data[29] = send_id;
     data[30] = seq_num ++;
     data[31] = crc_calculate(data, 31);
     
     printf("[%u] [%3u] Send\n", send_id, seq_num);

     PT_SEM_WAIT(pt, &nrf_in_use);
     if (!nrf_reliable_send(data))
        printf("Send failed!\n");
     PT_SEM_SIGNAL(pt, &nrf_in_use);
     
     PT_TIMER_DELAY(pt, 100);
     if (tx_status != TX_REACH_DST)
        printf("Send failed with too many retries\n");
  
     if(loop_time ++ % 20 == 0)
        PT_TIMER_DELAY(pt, random(1000));   
   }
   PT_END(pt);
}

static int thread_receiver_entry(struct pt *pt)
{
  PT_BEGIN(pt);
  static uint8_t data[32];
  while (1) {
    PT_SEM_WAIT(pt, &nrf_in_use);
    if (nrf_data_ready()) {
        uint8_t reg_val = nrf_get_status();
        reg_val = (reg_val >> 1) & 0x07;
        nrf_get_data(data);
        printf("[%u] [%3u] Recv pipe %u: %s\n", 0, 0, reg_val, data);
    }
    PT_SEM_SIGNAL(pt, &nrf_in_use);
    PT_YIELD(pt);
  }
  PT_END(pt);
}


static int thread_test_entry()
{
  static uint8_t data[32];
  enable_rx();
  while (1) {
    if (nrf_data_ready()) {
        nrf_get_data(data);
        printf("[%u] [%3u] Recv : %s\n", 0, 0, data);
    }
  }
}

void loop()
{
  thread_sender_entry(&thread1);  
  thread_receiver_entry(&thread2);
  //thread_test_entry();
}

