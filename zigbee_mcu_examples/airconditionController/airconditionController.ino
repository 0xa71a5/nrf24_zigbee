#include <lxc_nrf24l01.h>

char my_mac_addr = '2' ; 
#define PWM_PIN 5

void turnOnAC()
{
  analogWrite(PWM_PIN, 127);
}

void turnOffAC()
{
  analogWrite(PWM_PIN, 0);
}

void setTemperature(uint32_t t)
{
  if (t > 30)
    t = 30;
  t = t * 255 / 30;
  analogWrite(PWM_PIN, t);
}

void ac_functional_handler(String type,String content,uint8_t senderId=0)
{
  String packet;
  if (type == "get")//getVal1
  {
    if(content == "status")
    {
      construct_format(packet, "status", "online");
      apl_send(COORD_NET_ADDR, packet.c_str());
    }
  }
  else if (type == "changeAcTemp")
  {
    String newTemp = content;
    int newTempInt = atoi(newTemp.c_str());

    construct_format(packet, "changeAcTempResult", "suc");
    apl_send(COORD_NET_ADDR, packet.c_str());
    setTemperature(newTempInt);
  }
  else if (type == "turnOnAc")
  {
    construct_format(packet, "turnOnAcResult", "suc");
    apl_send(COORD_NET_ADDR, packet.c_str());
    turnOnAC();
  }
  else if (type == "turnOffAc")
  {
    construct_format(packet, "turnOffAcResult", "suc");
    apl_send(COORD_NET_ADDR, packet.c_str());
    turnOffAC();
  }
}

void device_init()
{
  turnOffAC();
}

void setup()
{
  Serial.begin(2000000);
  Serial.println("Begin config!");
  handle_func_register(ac_functional_handler);
  nrf_gpio_init(8, 9);
  set_tx_addr((uint8_t *)"mac00");
  my_mac_addr = '2' ; 
  set_mac_addr(&my_mac_addr);
  nrf_chip_config(12, 32);
  Serial.println("Device ac is running!");
  device_init();
  zigbee_network_init(ZIGBEE_END_DEVICE);
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

