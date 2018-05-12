#include <FreeRTOS_AVR.h>
#include <NRF24Zigbee.h>
#include "nz_phy_layer.h"

SemaphoreHandle_t phy_rx_fifo_sem;

TaskHandle_t task_rx_server_handle;
TaskHandle_t task_rx_get_data_handle;

static void get_phy_layer_data_service(void * params)
{
  uint8_t data_length;
  uint8_t data[128];
  while (1) {
    xSemaphoreTake(phy_rx_fifo_sem, portMAX_DELAY);
    if ((data_length = phy_layer_fifo_top_node_size()) > 0) {
      data_length = phy_layer_fifo_pop_data(data);
      debug_printf("read_size=%u crc_raw=0x%02X crc_calc=0x%02X \n\n", data_length, data[data_length-1], crc_calculate(data, data_length-1));
    }
    xSemaphoreGive(phy_rx_fifo_sem);

    vTaskDelay(5);
  }
}

static void phy_rx_service(void *params)
{
  while (1) {

    xSemaphoreTake(phy_rx_fifo_sem, portMAX_DELAY);
    phy_layer_listener();
    xSemaphoreGive(phy_rx_fifo_sem);
    
    vTaskDelay(1);
  }
}

static void vPrintTask(void *pvParameters) {
  while (1) {
    // Sleep for one second.
    vTaskDelay(177);
    //xSemaphoreTake(phy_rx_fifo_sem, portMAX_DELAY);
    Serial.print(F("Unused: "));
    Serial.print(uxTaskGetStackHighWaterMark(task_rx_server_handle));//Here is a 255 max value ,so this is bug
    Serial.print(";");
    Serial.print(uxTaskGetStackHighWaterMark(task_rx_get_data_handle));
    Serial.print(";");
    Serial.println(freeHeap());
    //xSemaphoreTake(phy_rx_fifo_sem, portMAX_DELAY);
  }
}


void setup()
{
  Serial.begin(1000000);
  printf_begin();
  printf("Begin config!\n");
  phy_layer_init("02");

  phy_rx_fifo_sem = xSemaphoreCreateCounting(1, 1);

  xTaskCreate(phy_rx_service, "rx_sv", configMINIMAL_STACK_SIZE +500,/*85+500 bytes stack*/
    NULL, tskIDLE_PRIORITY + 2, &task_rx_server_handle); //Used: 580 bytes stack

  xTaskCreate(get_phy_layer_data_service, "get", configMINIMAL_STACK_SIZE + 300, 
    NULL, tskIDLE_PRIORITY + 2, &task_rx_get_data_handle);// Used 190 bytes stack

  // create print task
  xTaskCreate(vPrintTask, "prtStack", configMINIMAL_STACK_SIZE + 100,
    NULL, tskIDLE_PRIORITY + 1, NULL);

  printf("OS running...\n");
  
  vTaskStartScheduler();
  
  Serial.println(F("Die"));
  while(1);
}

void loop()
{

}
