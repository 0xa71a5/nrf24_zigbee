#include <NRF24Zigbee.h>
#include "nz_phy_layer.h"

void print_buffer(uint8_t *base, uint16_t length)
{
  for (uint16_t i = 0; i < length; i ++) {
    if (i % 10 == 0 && i != 0)
      debug_printf("\n");
    debug_printf("0x%02X ", base[i]);
  }
  debug_printf("\n");
}

void setup()
{
  Serial.begin(1000000);
  printf_begin();
  printf("Begin config!");
  phy_layer_init("02");
}

int count = 0;
int start = 0;
uint8_t seq_num = 0;
uint8_t failed_times = 0;
uint32_t loop_time = 0;
#define MAX_RETRY_TIME 10

uint32_t last_check_time = 0;

void loop()
{
  if (millis() - last_check_time > 100) {
    #define test_size 128
    uint8_t data[128];
    for (int i = 0; i < test_size; i++)
      data[i] = i;
    pr_info("Call phy_layer_send_raw_data\n\n");
    
    phy_layer_send_raw_data("mac01", data, test_size);
    last_check_time = millis();
  }

  phy_layer_listener();

  uint8_t data_length;
  if ((data_length = phy_layer_fifo_top_node_size()) > 0) {
    if (data_length > 128)
      data_length = 128;
    uint8_t *data = new uint8_t(data_length);
    uint8_t read_size = phy_layer_fifo_pop_data(data);
    debug_printf("################\n");
    debug_printf("top_size=%u read_size=%u raw_data=\n", data_length, read_size);
    print_buffer(data, 10);
    debug_printf("################\n\n");
    free(data);
  }
}
