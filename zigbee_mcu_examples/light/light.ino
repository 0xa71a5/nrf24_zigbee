#include <lxc_nrf24l01.h>
#include <Servo.h>
Servo lightServo;
#define LED 4
uint8_t power_status;
uint8_t ON_ANGLE = 60;
uint8_t MIDDLE_ANGLE = 90;
uint8_t OFF_ANGLE = 120;

uint8_t my_mac_addr = '4';

enum {
  POWER_OFF = 0,
  POWER_ON = 1
}POWER_STATUS;

uint8_t getSwitchState()
{
  return power_status;
}


void turnOnLight()
{
  digitalWrite(LED, 1);
  power_status = POWER_ON;
  /*
  lightServo.write(ON_ANGLE);
  delay(500);
  lightServo.write(MIDDLE_ANGLE);
  */
}

void turnOffLight()
{
  digitalWrite(LED, 0);
  power_status = POWER_OFF;
  /*
  lightServo.write(OFF_ANGLE);
  delay(500);
  lightServo.write(MIDDLE_ANGLE);
  */
}


void light_functional_handler(String type,String content,uint8_t senderId=0)
{
  String packet;
  if (type == "get")//getVal1
  {
    if (content == "status")
    {
      construct_format(packet, "status", "online");
      apl_send(COORD_NET_ADDR, packet.c_str());
    }
    else if (content == "switchState")//开关状态
    {
      String state = "";
      if (getSwitchState() == POWER_ON)
      {
        state = "on";//开关被打开了
      }
      else
      {
        state = "off";//开关被关闭了
      }
      construct_format(packet, "status", state);
      apl_send(COORD_NET_ADDR, packet.c_str());
    }
  }
  else if (type == "power")
  {
    construct_format(packet, "result", "suc");
    apl_send(COORD_NET_ADDR, packet.c_str());
     if(content == "on")
    {
     turnOnLight(); 
    }
    else
    {
      turnOffLight(); 
    }
  }
  else if (type == "angle" )
  {
    construct_format(packet, "result", "suc");
    apl_send(COORD_NET_ADDR, packet.c_str());
    int index0 = content.indexOf('|');
    String angleNum = content.substring(0, index0);
    Serial.print(angleNum);Serial.print(",");
    int index1 = content.indexOf('|', index0 + 1);
    angleNum = content.substring(index0+1, index1);
    Serial.print(angleNum);Serial.print(",");
    angleNum = content.substring(index1+1);
    Serial.println(angleNum);
  }
}

void device_init()
{
  //lightServo.attach(5);
  pinMode(LED, OUTPUT);
  turnOffLight();
}

void setup()
{
  Serial.begin(2000000);
  Serial.println("Begin config!");
  handle_func_register(light_functional_handler);
  nrf_gpio_init(8, 9);
  set_tx_addr((uint8_t *)"mac00");
  my_mac_addr = '4';
  set_mac_addr(&my_mac_addr);
  nrf_chip_config(12, 32);
  device_init();
  zigbee_network_init(ZIGBEE_END_DEVICE);
  Serial.println("Device light is running!");
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
