#include <NRF24Zigbee.h>
#include <nz_phy_layer.h>
#include <nz_mac_layer.h>
#include <nz_nwk_layer.h>
#include <nz_apl_layer.h>
#include <FreeRTOS_AVR.h>
#include <nz_common.h>

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
    //uint8_t dst[2] = {'0', '0'};
    uint16_t dst = 0xff00;
    uint32_t record_time = micros();
    
    phy_layer_send_raw_data(dst, data, test_size);

    record_time = micros() - record_time;
    debug_printf("Take %u s to send\n\n", record_time);
    
    vTaskDelay(1002);
  }
}

extern volatile nlme_nwk_discovery_confirm_handle *apl_nwk_discovery_ptr;



uint8_t test_coord_ieee_addr[8];
void apl_layer_test(void *params)
{
  uint32_t record_time;
  static apl_indication indication;
  #define apl_data_length 45
  static uint8_t apl_data[apl_data_length];
  static network_descriptor_handle nwk_descriptor;

  uint16_t dst_addr = 0x0100;
  uint8_t handle = random(255);

  debug_printf("Enter apl_layer_test\n");

  while(1){
    if (xQueueReceive(apl_indication_fifo, &indication, 1000)) {
      debug_printf("app:recv msg,data_size=%u data=[%s] \n\n", indication.length, indication.data);
    }
    // TODO : APL layer send and receive data 
    vTaskDelay(7);

    if (Serial.available()) {
      char c = Serial.read();
      if (c == '2') {
        debug_printf("apl:call join\n");
        nlme_join_request(test_coord_ieee_addr, 0, 0xffff, 100, 0);
        if(signal_wait(&apl_join_confirm_event_flag, 2500)) {
          debug_printf("apl:recv join confirm\n");
          debug_printf("My new nwk_addr=0x%04X\n", apl_join_confirm_ptr->nwk_addr);
        }
        else {
          debug_printf("apl:join confirm timeout\n");
        }
      }
    }
  }

#if 0
  /* only coord needs to formation */
  while (1) {
    if (xQueueReceive(apl_indication_fifo, &indication, 1000)) {
      debug_printf("app:recv msg,data_size=%u data=[%s] \n\n", indication.length, indication.data);
    }
    // TODO : APL layer send and receive data 
    vTaskDelay(7);

    if (Serial.available()) {
      char c = Serial.read();

      if (c == '0') {
        debug_printf("\nCall nlme_network_discovery_request####\n");
        nlme_network_discovery_request(0, 100);
        if (signal_wait(&apl_nwk_discovery_event_flag, 2000)) {
        //if (wait_event((uint8_t *)apl_nwk_discovery_ptr, 2000)) {
          debug_printf("apl:Got nwk discovery confirm %u\n", apl_nwk_discovery_ptr->status);
        }
        else {
          debug_printf("apl: wait nwk discovery timeout\n");
        }

        debug_printf("Got nwk_descriptor size %u\n", nwk_descriptors_fifo.cur_size);
        while(event_fifo_out(&nwk_descriptors_fifo, &nwk_descriptor)) {
            debug_printf("NWK descriptor: perimit_join=%u router_capacity=%u ext_panid=", nwk_descriptor.permit_joining, nwk_descriptor.router_capacity);
            extended_panid_print(nwk_descriptor.extended_panid);
        }
      }

      else if (c == '1') {
        debug_printf("\napl:Try to do network discovery\n");
        
        nlme_network_discovery_request(0, 100);
        //vTaskDelay(500);
        if (signal_wait(&apl_nwk_discovery_event_flag, 2000)) {
        //if (wait_event((uint8_t *)apl_nwk_discovery_ptr, 2000)) {
          debug_printf("apl:Got nwk discovery confirm %u\n", apl_nwk_discovery_ptr->status);
        }
        else {
          debug_printf("apl: wait nwk discovery timeout\n");
        }

        debug_printf("apl:Got nwk_descriptor size %u\n", nwk_descriptors_fifo.cur_size);
        
        if (nwk_descriptors_fifo.cur_size != 0) {
          while(event_fifo_out(&nwk_descriptors_fifo, &nwk_descriptor)) {
            debug_printf("apl:NWK descriptor: perimit_join=%u router_capacity=%u ext_panid=", nwk_descriptor.permit_joining, nwk_descriptor.router_capacity);
            extended_panid_print(nwk_descriptor.extended_panid);
          }
          /* We just find the last one to join */

          debug_printf("apl:Try to join network:");
          extended_panid_print(nwk_descriptor.extended_panid);
          vTaskDelay(1000);
          debug_printf("apl:Call nlme_join_request\n");
          // TODO: Try to get shortPanId from panIdDesciptor fifo....,now i set it to 0xffff
          nlme_join_request(nwk_descriptor.extended_panid, 0, 0xffff, 100, 0);
        }
        else {
          debug_printf("Use default coord addr\n");
          vTaskDelay(1000);
          nlme_join_request(test_coord_ieee_addr, 0, 0xffff, 100, 0);
        }
      }

      else if (c == '2') {
        debug_printf("apl:call join\n");
        nlme_join_request(test_coord_ieee_addr, 0, 0xffff, 100, 0);
        if(signal_wait(&apl_join_confirm_event_flag, 2500)) {
          debug_printf("apl:recv join confirm\n");
          debug_printf("My new nwk_addr=0x%04X\n", apl_join_confirm_ptr->nwk_addr);
        }
        else {
          debug_printf("apl:join confirm timeout\n");
        }
      }

    }

  }
#endif
}


void setup()
{
  Serial.begin(2000000);
  printf_begin();
  debug_printf("Begin config!\n");
  node_identify = ZIGBEE_DEVICE;

  phy_layer_init(0x0200);
  mac_layer_init();
  nwk_layer_init();
  apl_layer_init();

  for (uint8_t i = 0; i < 7; i ++) {
    nlme_set_request(nwkExtendedPANID[i], i);
    mlme_set_request(aExtendedAddress[i], i);
    test_coord_ieee_addr[i] = i;
  }
  nlme_set_request(nwkExtendedPANID[7], 0x02);
  mlme_set_request(aExtendedAddress[7], 0x02);
  test_coord_ieee_addr[7] = 0x0;

  xTaskCreate(phy_layer_event_process, "rx_sv", 400,/*150 bytes stack*/
    NULL, tskIDLE_PRIORITY + 2, &task_rx_server_handle); //Used: 580 bytes stack

  xTaskCreate(mac_layer_event_process, "mac_sv", 250,
    NULL, tskIDLE_PRIORITY + 2, NULL);

  xTaskCreate(nwk_layer_event_process, "nwk_sv", 400,
    NULL, tskIDLE_PRIORITY + 2, NULL);

  xTaskCreate(apl_layer_event_process, "apl_sv", 400+50,
    NULL, tskIDLE_PRIORITY + 2, NULL);

  xTaskCreate(apl_layer_test, "apl_test", 400,
    NULL, tskIDLE_PRIORITY + 2, NULL);


  debug_printf("Zigbee network starts!\n");
  vTaskStartScheduler();
}

void loop()
{
  
}
