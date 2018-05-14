#include "nz_nwk_layer.h"
#include "nz_common.h"

QueueHandle_t nwk_confirm_fifo;
QueueHandle_t nwk_indication_fifo;

volatile uint8_t scan_confirm_event_flag = 0;
volatile uint8_t start_confirm_event_flag = 0;
volatile uint8_t data_confirm_event_flag = 0;

struct NWK_PIB_attributes_handle NWK_PIB_attributes;

void nwk_layer_init()
{
  nwk_confirm_fifo = xQueueCreate(NWK_CONFIRM_FIFO_SIZE, sizeof(confirm_event));
  nwk_indication_fifo = xQueueCreate(NWK_INDICATION_FIFO_SIZE, sizeof(nwk_indication));
}

void nlme_send_confirm_event(uint8_t confirm_type, void *ptr)
{
  confirm_event event;

  event.confirm_type = confirm_type;
  event.confirm_ptr = (uint8_t *)ptr;
  xQueueSendToBack(apl_confirm_fifo, &event, pdMS_TO_TICKS(1000));
}

/* Format a new network request */
void nlme_network_formation_request(uint8_t scan_channels, uint8_t scan_duration, uint8_t battery_life_ext)
{
  uint32_t notify_value = 0;
  uint32_t record_time = 0;
  confirm_event event;

  /* scan_type, scan_channels, scan_duration, scan_channel_i_page */
  // do a ed scan and wait for result
  debug_printf("Send ed_scan request to mac\n");

  mlme_scan_request(ed_scan, scan_channels, scan_duration, 0);

  if (signal_wait(&scan_confirm_event_flag, 100))
    debug_printf("Got ed_scan result\n");
  else {
    goto fail_exit;
  }

  mlme_scan_request(active_scan, scan_channels, scan_duration, 0);

  if (signal_wait(&scan_confirm_event_flag, 100))
    debug_printf("Got active_scan result\n");
  else {
    goto fail_exit;
  }

  /* Set network address and mac address */
  nlme_set_request(nwkNetworkAddress, DEFAULT_COORD_NET_ADDR);
  mlme_set_request(macShortAddress, DEFAULT_COORD_NET_ADDR);

  /* Set network PAN id and mac PAN id */
  nlme_set_request(nwkPANID, DEFAULT_PANID);

  debug_printf("Send start request to mac\n");

  /* macPANId, logicalChannel, PANCoordinator ,macBattLifeExt*/
  mlme_start_request(DEFAULT_PANID, DEFAULT_LOGICAL_CHANNEL, 1, battery_life_ext);

  if (signal_wait(&start_confirm_event_flag, 100))
    debug_printf("Got start result\n");
  else {
    goto fail_exit;
  }


  nlme_network_formation_confirm(SUCCESS);
  return;

  fail_exit:
  debug_printf("formation fail_exit\n");
  nlme_network_formation_confirm(STARTUP_FAILURE);
}

/* Format a new network confirm */
void nlme_network_formation_confirm(uint8_t status)
{
  static nlme_formation_confirm_handle confirm;

  confirm.status = status;
  nlme_send_confirm_event(confirm_type_formation, &confirm);
  debug_printf("nlme_network_formation_confirm %u\n", status);
}

void nwk_layer_event_process(void * params)
{
  confirm_event event;
  nwk_indication indication;

  while (1) {
    /* Handle confirm from mac layer */
    if (xQueueReceive(nwk_confirm_fifo, &event, 100)) {
      debug_printf("nwk_sv:recv confirm :type=%u addr=0x%04X\n", 
        event.confirm_type, event.confirm_ptr);

      /* We got confirm signal from mac layer */
      switch (event.confirm_type) {
        case confirm_type_scan:
          scan_confirm_event_flag = 1;
        break;

        case confirm_type_start:
          start_confirm_event_flag = 1;
        break;

        case confirm_type_set:

        break;

        case confirm_type_data_confirm:
          data_confirm_event_flag = 1;
        break;
      }
    }

    /* Handle indication from mac layer */
    if (xQueueReceive(nwk_indication_fifo, &indication, 100)) {
      debug_printf("nwk_sv:recv indication,datasize=%u\n", indication.length);
      //TODO : send indication to apl layer
    }

    vTaskDelay(1); 
  }
}


/*
 * @param dst_addr 			:dstination device network 16bit addr
 * @param nsdu_length		:size of nsdu
 * @param nsdu 				:network service data unit
 * @param nsdu_handle		:handle combined to nsdu
 * @param broadcast_radius  :jump cnt limits
 * @param discovery_route	:true->allow route discovery , false->not allow route discovery
 * @param security_mode 	:true->take security actions
 */
void nlde_data_request(uint16_t dst_addr, uint8_t nsdu_length, uint8_t *nsdu, uint8_t nsdu_handle, uint8_t broadcast_radius,
	uint8_t discovery_route)
{
  static uint8_t npdu_mem[NPDU_MAX_SIZE] = {0};
  npdu_frame_handle * npdu_frame = (npdu_frame_handle *)npdu_mem;
  uint8_t to_send_size = 0;

  /* Check if data length under law */
  if (nsdu_length > NPDU_PAYLOAD_MAX_SIZE) {
    nlde_data_confirm(FRAME_TOO_LONG, nsdu_handle, millis());
    return;
  }

  /* Construct npdu data */
  //npdu_frame->frame_control = 0x00;
  npdu_frame->dst_addr = dst_addr;
  npdu_frame->src_addr = nlme_get_request(nwkNetworkAddress);
  npdu_frame->radius = 0xff;
  npdu_frame->seq++;
  npdu_frame->multicast_control = 0x00;
  memcpy(npdu_frame->payload, nsdu, nsdu_length);

  to_send_size = sizeof(npdu_frame_handle) + nsdu_length;
  mcps_data_request(0, 0, DEFAULT_PANID, dst_addr, to_send_size, (uint8_t *)npdu_frame, nsdu_handle, 0);

  if (signal_wait(&data_confirm_event_flag, 500)) {
    /* TODO: here we shall read confirm message from mac and retransfer to apl */
    nlde_data_confirm(SUCCESS, nsdu_handle, millis());
  }
  else {
    nlde_data_confirm(TRANSACTION_EXPIRED, nsdu_handle, millis());
  }
}


void nlde_data_confirm(uint8_t status, uint8_t npdu_handle, uint32_t tx_time)
{
  static nlde_data_confirm_handle confirm;

  confirm.status = status;
  confirm.nsdu_handle = npdu_handle;
  confirm.tx_time = tx_time;

  nlme_send_confirm_event(confirm_type_data_confirm, &confirm);
  debug_printf("nlde_data_confirm %u\n", status);
}

/* Dont use link quality and security */
void nlde_data_indication(uint8_t dst_addr_mode, uint16_t dst_addr, uint16_t src_addr, 
  uint8_t nsdu_length, uint8_t *nsdu, uint32_t rx_time)
{

}