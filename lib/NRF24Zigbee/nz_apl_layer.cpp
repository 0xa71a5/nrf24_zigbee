#include "nz_apl_layer.h"


QueueHandle_t apl_confirm_fifo;
QueueHandle_t apl_indication_fifo;

#define NWK_JOIN_INDICATION_FIFO_SIZE 3
event_node_handle event_join_ind_ptr_area[NWK_JOIN_INDICATION_FIFO_SIZE];
nwk_join_indication_handle nwk_join_ind_mem[NWK_JOIN_INDICATION_FIFO_SIZE];
event_fifo_handle nwk_join_ind_fifo;

volatile uint8_t formation_confirm_event_flag = 0;
volatile uint8_t apl_data_confirm_event_flag = 0;
volatile uint8_t apl_nwk_discovery_event_flag = 0;
volatile uint8_t apl_join_confirm_event_flag = 0;

volatile uint8_t *apl_data_ptr;

void apl_layer_init()
{
  apl_confirm_fifo = xQueueCreate(APL_CONFIRM_FIFO_SIZE, sizeof(confirm_event));
  apl_indication_fifo = xQueueCreate(APL_INDICATION_FIFO_SIZE, sizeof(apl_indication));

  event_fifo_init(&nwk_join_ind_fifo, event_join_ind_ptr_area,
  (uint8_t *)nwk_join_ind_mem, NWK_JOIN_INDICATION_FIFO_SIZE, sizeof(nwk_join_indication_handle));

}

volatile nlme_formation_confirm_handle *apl_data_confirm_ptr = NULL;
volatile nlme_nwk_discovery_confirm_handle *apl_nwk_discovery_ptr = NULL;
volatile nlme_join_confirm_handle *apl_join_confirm_ptr = NULL;

void apl_layer_event_process(void *params)
{
  confirm_event event;
  static apl_indication indication;
  static nwk_join_indication_handle join_indication;

  debug_printf("Enter apl layer event process\n");
  while (1) {
    if (xQueueReceive(apl_confirm_fifo, &event, 230)) {
      debug_printf("apl_sv:recv from fifo :type=%u addr=0x%04X\n", 
        event.confirm_type, event.confirm_ptr);

      /* We got confirm signal from nwk layer */
      switch (event.confirm_type) {
        case confirm_type_formation:
          formation_confirm_event_flag = 1;
        break;

        case confirm_type_data_confirm:
          apl_data_confirm_ptr = (nlme_formation_confirm_handle *)event.confirm_ptr;
          apl_data_confirm_event_flag = 1;
        break;

        case confirm_type_nwk_discovery:
          apl_nwk_discovery_ptr = (nlme_nwk_discovery_confirm_handle *)event.confirm_ptr;
          apl_nwk_discovery_event_flag = 1;
        break;

        case confirm_type_join:
          apl_join_confirm_ptr = (nlme_join_confirm_handle *)event.confirm_ptr;
          apl_join_confirm_event_flag = 1;
        break;

        default:
          debug_printf("Unknown type of event confirm\n");
        break;
      }
    }

    if (xQueueReceive(apl_indication_fifo, &indication, 1000)) {
      debug_printf("app:recv msg,data_size=%u data=[%s] \n\n", indication.length, indication.data);
      apl_data_ptr = indication.data;
    }

    if (nwk_join_ind_fifo.cur_size != 0) {
      debug_printf("apl_sv: new device join this network\n");
      event_fifo_out(&nwk_join_ind_fifo, &join_indication);
    }

    vTaskDelay(1); 
  }
}



void zigbee_network_init(uint8_t device_role)
{
  /* Init all layer */
  phy_layer_init(0x0f00);
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

  if (device_role == ZIGBEE_END_DEVICE) {
    nlme_network_discovery_request(0, 100);
    if (signal_wait(&apl_nwk_discovery_event_flag, 2000)) {
    //if (wait_event((uint8_t *)apl_nwk_discovery_ptr, 2000))
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
    nlme_join_request(nwk_descriptor.extended_panid, 0, 0xffff, 100, 0);
  }
  else if (device_role == ZIGBEE_ROUTER) {
    nlme_network_discovery_request(0, 100);
    if (signal_wait(&apl_nwk_discovery_event_flag, 2000)) {
    //if (wait_event((uint8_t *)apl_nwk_discovery_ptr, 2000))
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
    nlme_join_request(nwk_descriptor.extended_panid, 0, 0xfffe, 200, 0);
  }
  else if (device_role == ZIGBEE_COORD) {
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
  }

  debug_printf("Zigbee network starts!\n");
}

uint8_t apl_data_ready()
{
  return nwk_join_ind_fifo.cur_size;
}

uint8_t apl_get_data(uint8_t *dst_data_ptr)
{
  memcpy(dst_data_ptr, apl_data_ptr, 32);  
  return 32;
}

uint8_t apl_send(uint16_t dst_nwk_addr, uint8_t *data_32)
{
  static uint8_t handle = 0;

  nlde_data_request(dst_nwk_addr, 32, data_32, handle++, 0, 0);
  if (signal_wait(&apl_data_confirm_event_flag, 500)) {
      if (!apl_data_confirm_ptr->status) {
        debug_printf("Got data request confirm\n");
        return 1;
      }
      else {
        debug_printf("Got data confirm ,but it says send failed\n");
        return 0;
      }
    }
    else {
      debug_printf("Dont get date request confirm in limit time\n");
      return 0;
    }
}

