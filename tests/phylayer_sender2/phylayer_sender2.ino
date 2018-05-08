#include <NRF24Zigbee.h>
#include "nz_phy_layer.h"


void setup()
{
  Serial.begin(500000);
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
}
