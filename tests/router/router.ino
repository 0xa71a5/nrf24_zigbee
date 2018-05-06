#include <lxc_nrf24l01.h>

void setup()
{
  Serial.begin(250000);
  Serial.println("Begin config!");
  nrf_gpio_init(8, 9); //Set ce pin and csn pin
  set_tx_addr((uint8_t *)"mac00");
  set_rx_addr((uint8_t *)"mac01");
  nrf_chip_config(12, 32); // Set channel and payload
  Serial.println("Begining!");
}
uint8_t data = 0;
void loop()
{
   char data[32];
   if (data_ready()) {
      get_data(data);
      Serial.print("Got packet->");
      Serial.println(data);
      nrf_send(data);
  }
}

