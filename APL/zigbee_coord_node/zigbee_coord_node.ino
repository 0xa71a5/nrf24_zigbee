#include <FreeRTOS_AVR.h>
#include <NRF24Zigbee.h>
#include <nz_phy_layer.h>
#include <nz_mac_layer.h>
#include <nz_nwk_layer.h>
#include <nz_apl_layer.h>
#include <event_fifo.h>
#include <nz_common.h>




TaskHandle_t task_rx_server_handle;
TaskHandle_t task_rx_get_data_handle;
TaskHandle_t task_tx_server_handle;



void apl_layer_test(void *params)
{
  uint32_t record_time;
  static apl_indication indication;

  debug_printf("Enter apl_layer_test\n");

  debug_printf("apl layer call nlme_network_formation_request\n");
    /* scan channels, scan duration, batter life ext */
  nlme_network_formation_request(0, 100, 0);

  if (signal_wait(&formation_confirm_event_flag, 500)) {
    debug_printf("###formation success###\n");
    debug_printf("NWK: PANid=0x%04X ADDR=0x%04X\n", nlme_get_request(nwkPANID), nlme_get_request(nwkNetworkAddress));
    debug_printf("MAC: PANid=0x%04X ADDR=0x%04X\n", mlme_get_request(macPANId), mlme_get_request(macShortAddress));
    debug_printf("#######################\n\n");
  }
  else {
    debug_printf("###formation failed!###\n\n");
  }

  while (1) {
    if (xQueueReceive(apl_indication_fifo, &indication, 1000)) {
      debug_printf("app:recv msg,data_size=%u data=[%s] \n\n", indication.length, indication.data);
    }
    // TODO : APL layer send and receive data 
    vTaskDelay(7);
    /*
    if (Serial.available()) {
      Serial.read();
      debug_printf("\nCall nlme_network_discovery_request\n");
      nlme_network_discovery_request(0xffffffff, 100);
    }
    */
  }

}


void setup()
{
  Serial.begin(2000000);
  printf_begin();
  debug_printf("Begin config!\n");
  node_identify = ZIGBEE_ROUTE;

  phy_layer_init(0x0100);
  mac_layer_init();
  nwk_layer_init();
  apl_layer_init();


  xTaskCreate(phy_layer_event_process, "rx_sv", 300,/*150 bytes stack*/
    NULL, tskIDLE_PRIORITY + 2, &task_rx_server_handle); //Used: 580 bytes stack

  xTaskCreate(mac_layer_event_process, "mac_sv", 250+120,
    NULL, tskIDLE_PRIORITY + 2, NULL);

  xTaskCreate(nwk_layer_event_process, "nwk_sv", 400+120,
    NULL, tskIDLE_PRIORITY + 2, NULL);

  xTaskCreate(apl_layer_event_process, "apl_sv", 400,
    NULL, tskIDLE_PRIORITY + 2, NULL);

  xTaskCreate(apl_layer_test, "apl_test", 400,
    NULL, tskIDLE_PRIORITY + 2, NULL);

  //xTaskCreate(send_packet_test, "tx_sv", 350, 
    //NULL, tskIDLE_PRIORITY + 2, &task_tx_server_handle);//Used 327 byte

  debug_printf("Zigbee network starts!\n");
  vTaskStartScheduler();
}

void loop()
{
  
}
