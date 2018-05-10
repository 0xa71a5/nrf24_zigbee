#include <NRF24Zigbee.h>
#include "nz_phy_layer.h"
#include <TimerTwo.h>

void print_buffer(uint8_t *base, uint16_t length)
{
  for (uint16_t i = 0; i < length; i ++) {
    if (i % 10 == 0 && i != 0)
      debug_printf("\n");
    debug_printf("0x%02X ", base[i]);
  }
  debug_printf("\n");
}

void interrupt_handler()
{
  Timer2.StopTimer();
  phy_layer_listener();
  Timer2.ResumeTimer();
}


void setup()
{
  Serial.begin(1000000);
  printf_begin();
  printf("Begin config!");
  phy_layer_init("02");
  //Timer2.EnableTimerInterrupt(interrupt_handler, 1000);
}

uint8_t seq_num = 0;
uint32_t last_check_time = 0;
uint32_t wait_time = 1000;

void loop()
{
  if (millis() - last_check_time > wait_time) {
    #define test_size 128
    uint8_t data[test_size];
    data[0] = seq_num ++;
    for (uint8_t i = 1; i < test_size-1; i++)
      data[i] = random(256);
    data[test_size - 1] = crc_calculate(data, test_size-1);
    debug_printf("===> Send to 00\n");
    phy_layer_send_raw_data("00", data, test_size);
    last_check_time = millis();
    wait_time = random(1500);
  }

  phy_layer_listener();

  uint8_t data_length;
  if ((data_length = phy_layer_fifo_top_node_size()) > 0) {
    uint8_t *data = new uint8_t(data_length);
    data_length = phy_layer_fifo_pop_data(data);
    print_buffer(data, 10);
    uint8_t crc = 0xf1;
    crc = crc_calculate(data, data_length-1);
    printf("crc=%u data_length=%u\n", crc, data_length);
    debug_printf("pop size=%u crc=%u\n\n", data_length, crc);
    free(data);
  }

}
