#include <FreeRTOS_AVR.h>
#include <NRF24Zigbee.h>
#include <nz_phy_layer.h>
#include <nz_mac_layer.h>
#include <nz_nwk_layer.h>
#include <nz_apl_layer.h>

TaskHandle_t task_rx_server_handle;
TaskHandle_t task_rx_get_data_handle;
TaskHandle_t task_tx_server_handle;



static void send_packet_test(void *params)
{
  uint8_t seq_num = 0;
  debug_printf("Enter send_packet_test\n");
  while (1) {
    #define test_size 127
    uint8_t data[test_size];
    data[0] = seq_num ++;
    for (uint8_t i = 1; i < test_size-1; i++)
      data[i] = random(256);
    data[test_size - 1] = crc_calculate(data, test_size-1);
    debug_printf("===> Send to 0 0xff,crc = 0x%02X\n", data[test_size-1]);
    uint8_t dst[2] = {'0', 0xff};
    uint32_t record_time = micros();
    
    phy_layer_send_raw_data(dst, data, test_size);

    record_time = micros() - record_time;
    debug_printf("Take %u s to send\n\n", record_time);
    
    vTaskDelay(500);
  }
}



void apl_layer_test(void *params)
{
  uint32_t record_time;
  debug_printf("Enter apl_layer_test\n");


  while (1) {
    vTaskDelay(933);

    debug_printf("apl layer call nlme_network_formation_request\n");
    nlme_network_formation_request();


    if (signal_wait(&formation_confirm_event_flag, 100)) {
      debug_printf("formation success\n");
    }
    else {
      debug_printf("formation failed!\n");
    }

  }

}


void setup()
{
  Serial.begin(2000000);
  printf_begin();
  debug_printf("Begin config!\n");
  phy_layer_init("00");
  mac_layer_init();
  nwk_layer_init();
  apl_layer_init();


  xTaskCreate(phy_layer_event_process, "rx_sv", 400,/*150 bytes stack*/
    NULL, tskIDLE_PRIORITY + 2, &task_rx_server_handle); //Used: 580 bytes stack

  xTaskCreate(mac_layer_event_process, "mac_sv", 250,
    NULL, tskIDLE_PRIORITY + 2, NULL);

  xTaskCreate(nwk_layer_event_process, "nwk_sv", 400,
    NULL, tskIDLE_PRIORITY + 2, NULL);

  xTaskCreate(apl_layer_event_process, "apl_sv", 400,
    NULL, tskIDLE_PRIORITY + 2, NULL);

  xTaskCreate(apl_layer_test, "apl_test", 400,
    NULL, tskIDLE_PRIORITY + 2, NULL);

  xTaskCreate(send_packet_test, "tx_sv", 350, 
    NULL, tskIDLE_PRIORITY + 2, &task_tx_server_handle);//Used 327 byte

  debug_printf("Zigbee network starts!\n");
  vTaskStartScheduler();
}

void loop()
{
  
}
