#include <FreeRTOS_AVR.h>
#include <NRF24Zigbee.h>
#include <nz_phy_layer.h>
#include <nz_mac_layer.h>
#include <nz_nwk_layer.h>
#include <nz_apl_layer.h>
#include <event_fifo.h>
#include <nz_common.h>



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

TaskHandle_t phy_task;
TaskHandle_t mac_task;
TaskHandle_t nwk_task;
TaskHandle_t apl_task;
TaskHandle_t app_task;

extern SemaphoreHandle_t rf_chip_use;
extern SemaphoreHandle_t phy_rx_fifo_sem;

static void vPrintTask(void *pvParameters) {
  debug_printf("Enter vPrintTask\n");
  while (1) {
    // Sleep for one second.
    vTaskDelay(70);
    debug_printf("Unused:%u %u %u %u %u [%u][%u]\n", uxTaskGetStackHighWaterMark(phy_task),
      uxTaskGetStackHighWaterMark(mac_task),
      uxTaskGetStackHighWaterMark(nwk_task),
      uxTaskGetStackHighWaterMark(apl_task),
      uxTaskGetStackHighWaterMark(app_task),
      (uint16_t)xSemaphoreGetMutexHolder(rf_chip_use),
      (uint16_t)xSemaphoreGetMutexHolder(phy_rx_fifo_sem));
  }
}

#define mac_task_stack_size (370+100)
uint8_t mac_task_stack[mac_task_stack_size];

#define nwk_task_stack_size (450+100)
uint8_t nwk_task_stack[nwk_task_stack_size];

#define apl_task_stack_size (400-100)
uint8_t apl_task_stack[apl_task_stack_size];

#define app_task_stack_size (400-100)
uint8_t app_task_stack[app_task_stack_size];

#define print_task_stack_size 150
uint8_t print_task_stack[print_task_stack_size];

void setup()
{
  Serial.begin(2000000);
  printf_begin();
  debug_printf("Begin config!\n");
  node_identify = ZIGBEE_ROUTE;

  phy_layer_init(0x0000);
  mac_layer_init();
  nwk_layer_init();
  apl_layer_init();

  for (uint8_t i = 0; i < 7; i ++) {
    nlme_set_request(nwkExtendedPANID[i], i);
    mlme_set_request(aExtendedAddress[i], i);
  }
  nlme_set_request(nwkExtendedPANID[7], 0x00);
  mlme_set_request(aExtendedAddress[7], 0x00);


  xTaskCreate(phy_layer_event_process, "rx_sv", 300+50,/*150 bytes stack*/
    //NULL, tskIDLE_PRIORITY + 2, &phy_task); //Used: 580 bytes stack

  xTaskCreate2(mac_layer_event_process, "mac_sv", mac_task_stack_size,
    NULL, tskIDLE_PRIORITY + 2, &mac_task, mac_task_stack);

  xTaskCreate2(nwk_layer_event_process, "nwk_sv", nwk_task_stack_size,
    NULL, tskIDLE_PRIORITY + 2, &nwk_task, nwk_task_stack);

  xTaskCreate2(apl_layer_event_process, "apl_sv", apl_task_stack_size,
    NULL, tskIDLE_PRIORITY + 2, &apl_task, apl_task_stack);

  xTaskCreate2(apl_layer_test, "apl_test", app_task_stack_size,
    NULL, tskIDLE_PRIORITY + 2, &app_task, app_task_stack);

  xTaskCreate2(vPrintTask, "printstack", print_task_stack_size, 
    NULL, tskIDLE_PRIORITY + 2, NULL, print_task_stack);

  debug_printf("Zigbee network starts!\n");
  vTaskStartScheduler();
}

void loop()
{
  
}
