#include <NRF24Zigbee.h>
#include <nz_phy_layer.h>

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
  debug_printf("Begin config!");
  phy_layer_init("01");
  Serial.println("Zigbee network starts!");
}

uint32_t last_check_time = 0;

void loop()
{
  if (millis() - last_check_time > 100) {
    uint8_t data[40] = {0};
    debug_printf("===> Send to 06\n");
    phy_layer_send_raw_data("06", data, 11);
    last_check_time = millis();
  }

  phy_layer_listener();

  uint8_t data_length;
  if ((data_length = phy_layer_fifo_top_node_size()) > 0) {
    uint8_t *data = new uint8_t(data_length);
    data_length = phy_layer_fifo_pop_data(data);
    debug_printf("read_size=%u crc_raw=0x%02X crc_calc=0x%02X \n\n", data_length, data[data_length-1], crc_calculate(data, data_length-1));
    free(data);
  }
}
