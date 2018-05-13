#include <NRF24Zigbee.h>
#include <nz_phy_layer.h>
#include <FreeRTOS_AVR.h>

SemaphoreHandle_t phy_rx_fifo_sem;

TaskHandle_t task_rx_server_handle;
TaskHandle_t task_rx_get_data_handle;
TaskHandle_t task_tx_server_handle;


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

static void phy_tx_service(void *params)
{
  uint8_t seq_num = 0;
  debug_printf("Enter phy_tx_service\n");
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
    
    xSemaphoreTake(phy_rx_fifo_sem, portMAX_DELAY);
    phy_layer_send_raw_data(dst, data, test_size);
    xSemaphoreGive(phy_rx_fifo_sem);

    record_time = micros() - record_time;
    debug_printf("Take %uSt to send\n\n", record_time);
    
    //debug_printf("===> Send to 0 0xff,crc = 0x%02X\n", 0xfa);
    vTaskDelay(100);
  }
}


void setup()
{
  Serial.begin(1000000);
  printf_begin();
  debug_printf("Begin config!\n");
  phy_layer_init("00");
  phy_rx_fifo_sem = xSemaphoreCreateCounting(1, 1);


  xTaskCreate(phy_rx_service, "rx_sv", 150,/*150 bytes stack*/
    NULL, tskIDLE_PRIORITY + 2, &task_rx_server_handle); //Used: 580 bytes stack

  xTaskCreate(get_phy_layer_data_service, "getdata", 250, 
    NULL, tskIDLE_PRIORITY + 2, &task_rx_get_data_handle);// Used 250 bytes stack

  xTaskCreate(phy_tx_service, "tx_sv", 350, 
    NULL, tskIDLE_PRIORITY + 2, &task_tx_server_handle);//Used 327 byte

  debug_printf("Zigbee network starts!\n");
  vTaskStartScheduler();
}

void loop()
{
  
}
