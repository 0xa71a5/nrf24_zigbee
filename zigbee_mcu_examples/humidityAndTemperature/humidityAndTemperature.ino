#include <dht.h>
#include <lxc_nrf24l01.h>

#define DHT11_PIN 5
dht DHT;

uint8_t my_mac_addr = '1';

void dhtsensor_functional_handler(String type,String content,uint8_t senderId=0)
{
  String packet;
  if (type == "get")//getVal1
  {
    if (content == "status")
    {
      construct_format(packet, "status", "online");
      apl_send(COORD_NET_ADDR, packet.c_str());
    }
    else if (content == "humidity")//time and val1
    {
      DHT.read11(DHT11_PIN);
      int humidityVal = DHT.humidity;
      construct_format(packet, "humidity", String(humidityVal));
      apl_send(COORD_NET_ADDR, packet.c_str());
    }
    else if (content == "temperature")
    {
      DHT.read11(DHT11_PIN);
      int temperatureVal = DHT.temperature;
      construct_format(packet, "temperature", String(temperatureVal));
      apl_send(COORD_NET_ADDR, packet.c_str());
    }
  }
}

void device_init()
{

}

void setup()
{
  Serial.begin(2000000);
  Serial.println("Begin config!");
  handle_func_register(dhtsensor_functional_handler);
  nrf_gpio_init(8, 9);
  set_tx_addr((uint8_t *)"mac00");
  my_mac_addr = '1';
  set_mac_addr(&my_mac_addr);
  nrf_set_broadcast_addr(0xff);
  nrf_set_retry_times(3);
  nrf_set_retry_durtion(1250);
  nrf_chip_config(12, 32);
  device_init();
  zigbee_network_init(ZIGBEE_END_DEVICE);
  Serial.println("Device dht is running!");
}


void loop()
{
   char data[32];
   if (apl_data_ready()){
      apl_get_data(data);
      Serial.print(" Got packet->");
      Serial.println(data);
      String str_data = data;
      handle_packet(str_data);
  }
}
