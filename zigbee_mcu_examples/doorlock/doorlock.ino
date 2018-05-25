#include <lxc_nrf24l01.h>
#include <Servo.h>
char my_mac_addr = '3' ; 

uint32_t last_action_time = 0;
#define signalPinA 3
#define signalPinB 4
#define LOCK_MOVE_TIME   180
#define MIN_OPERATION_INTERVAL 1000

//#define LOCK_TYPE_MOTOR
#define LOCK_TYPE_RELAY


Servo lockServo;

enum {
  POWER_OFF = 0,
  POWER_ON = 1
} POWER_STATUS;
uint8_t power_status = POWER_OFF;

uint8_t getSwitchState()
{
  return power_status;
}

void turnOnDoorlock()
{
  power_status = POWER_ON;
  if (millis() - last_action_time < MIN_OPERATION_INTERVAL) {
    last_action_time = millis();
    return;
  }
  #ifdef LOCK_TYPE_MOTOR
    digitalWrite(signalPinA,1);
    digitalWrite(signalPinB,0);
    delay(LOCK_MOVE_TIME);
    digitalWrite(signalPinA,0);
    digitalWrite(signalPinB,0);
  #else
    lockServo.write(60);
  #endif
  last_action_time = millis();
}

void turnOffDoorlock()
{
  power_status = POWER_OFF;
  if (millis() - last_action_time < MIN_OPERATION_INTERVAL) {
    last_action_time = millis();
    return;
  }

  #ifdef LOCK_TYPE_MOTOR
    digitalWrite(signalPinA,0);
    digitalWrite(signalPinB,1);
    delay(LOCK_MOVE_TIME);
    digitalWrite(signalPinA,0);
    digitalWrite(signalPinB,0); 
  #else
    lockServo.write(120);
  #endif
  last_action_time = millis();
}

void dl_functional_handler(String type,String content,uint8_t senderId=0)
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
      String state="";
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
    if (content == "on")
    {
     turnOnDoorlock(); 
    }
    else
    {
      turnOffDoorlock(); 
    }
  }
}

void device_init()
{
  #ifdef LOCK_TYPE_MOTOR
  pinMode(signalPinA,OUTPUT);
  pinMode(signalPinB,OUTPUT);
  digitalWrite(signalPinA,0);
  digitalWrite(signalPinB,0);
  #else
  lockServo.attach(4);
  delay(1250);
  turnOffDoorlock();
  #endif
}

void setup()
{
  Serial.begin(2000000);
  Serial.println("Begin config!");
  handle_func_register(dl_functional_handler);
  nrf_gpio_init(8, 9);
  set_tx_addr((uint8_t *)"mac00");
  my_mac_addr = '3' ; 
  set_mac_addr(&my_mac_addr);
  nrf_chip_config(12, 32);
  device_init();
  zigbee_network_init(ZIGBEE_END_DEVICE);
  Serial.println("Device doorlock is running!");
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

