#include <NRF24Zigbee.h>
#include <nz_phy_layer.h>

void setup()
{
  Serial.begin(500000);
  printf_begin();
 
  debug_printf("Begin config!");

  phy_layer_init("01");
  Serial.println("Zigbee network starts!");
}

uint32_t last_check_time = 0;

void loop()
{
  if (millis() - last_check_time > 1000) {
    uint8_t data[40] = {0};
    debug_printf("===> Send to mac02\n");
    phy_layer_send_raw_data("mac02", data, 40);
    last_check_time = millis();
  }

  phy_layer_listener();
}
