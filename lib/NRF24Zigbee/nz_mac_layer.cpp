#include "nz_mac_layer.h"
#include "nz_phy_layer.h"
#include "nz_nwk_layer.h"

#define MAC_CONFIRM_FIFO_SIZE 3

QueueHandle_t mac_confirm_fifo;
struct MAC_PIB_attributes_handle MAC_PIB_attributes;


void mac_layer_init()
{
  mac_confirm_fifo = xQueueCreate(MAC_CONFIRM_FIFO_SIZE, sizeof(confirm_event)); /* Create 3 fifo node for confirm event fifo*/
}

void mlme_send_confirm_event(uint8_t confirm_type, void *ptr)
{
	confirm_event event;

	event.confirm_type = confirm_type;
  event.confirm_ptr = (uint8_t *)ptr;
  xQueueSendToBack(nwk_confirm_fifo, &event, 500 /portTICK_PERIOD_MS);
}

uint16_t restore_pan_id = 0xffff;

void mlme_scan_request(uint8_t scan_type, uint32_t scan_channels, uint8_t scan_duration, uint8_t channel_i_page)
{  
  mpdu_frame_handle cmd_frame;
  bool send_result;
  uint8_t to_send_size;
  //TODO: edscan and active scan implemention

  if (scan_type == active_scan) {
    debug_printf("mlme_set_request active_scan\n");
    restore_pan_id = mlme_get_request(macPANId);
    // Set local panid to 0xffff
    MAC_PIB_attributes.macPANId = 1;
//    mlme_set_request(macPANId, 0x1);
/*

    // Send out active scan request beacon
    cmd_frame.frame_control.frame_type = mac_frame_type_command;
    cmd_frame.frame_control.dst_addr_mode = mac_addr_16bits;
    // TODO: src addr shall be comprised
    //cmd_frame.frame_control.src_addr_mode = mac_addr_no_present;
    cmd_frame.dst_pan_id = 0xffff;
    cmd_frame.dst_addr = 0xffff;
    cmd_frame.payload[0] = beacon_request;
    to_send_size = sizeof(cmd_frame) + 1;

    debug_printf("call phy_layer_send_raw_data, dst_addr=0x%04X\n", DEFAULT_BROADCAST_ADDR);
    phy_layer_send_raw_data(DEFAULT_BROADCAST_ADDR, (uint8_t *)&cmd_frame, to_send_size);
  */
  }

  //mlme_scan_confirm(scan_type, scan_channels, channel_i_page);
}

void mlme_scan_confirm(uint8_t status=0, uint8_t scan_type=0, uint8_t channel_page=0, uint32_t unscaned_channels=0,
  uint16_t result_list_size=0, uint8_t *energy_detect_list=0, uint8_t *pan_descript_list=0)
{
  static mlme_scan_confirm_handle scan_confirm;/* Set this to static ,cause we dont use malloc */

  scan_confirm.status = status;
  scan_confirm.scan_type = scan_type;
  scan_confirm.channel_page = channel_page;
  scan_confirm.result_list_size = result_list_size;
  scan_confirm.unscaned_channels = unscaned_channels;
  //scan_confirm.energy_detect_list = energy_detect_list;
  //scan_confirm.pan_descript_list = pan_descript_list;
  debug_printf("mlme_scan_confirm\n");
  mlme_send_confirm_event(confirm_type_scan, &scan_confirm);
}

//MLME-START
void mlme_start_request(uint16_t macPANId = 0, uint8_t logicalChannel = 0, uint8_t PANCoordinator = 0,
	uint8_t macBattLifeExt = 0)
{
  if (mlme_get_request(macShortAddress) == 0xffff) {
    debug_printf("mlme_start_request: return of NO_SHORT_ADDRESS\n");
  	mlme_start_confirm(NO_SHORT_ADDRESS);
  	return;
  }

  mlme_set_request(macPANId, macPANId);
  mlme_set_request(macLogicalChannel, logicalChannel);
  mlme_set_request(macPANCoordinator, PANCoordinator);
  mlme_set_request(macBattLifeExt, macBattLifeExt);

  mlme_start_confirm(SUCCESS);
}

void mlme_start_confirm(uint8_t status)
{
	static mlme_start_confirm_handle start_confirm;
	
	start_confirm.status = status;
	mlme_send_confirm_event(confirm_type_start, &start_confirm);
}

/* Recv confirm event from phy layer */
void mac_layer_event_process(void * params)
{
  confirm_event event;
  mlme_scan_confirm_handle * scan_confirm_ptr;
  static uint8_t data[128];
  mpdu_frame_handle * mpdu_frame = (mpdu_frame_handle *)data;
  mpdu_beacon_frame_handle * beacon = (mpdu_beacon_frame_handle *)data;
  uint8_t data_length;
  uint8_t payload_size = 0;

  debug_printf("Enter mac_layer_server\n");
  while (1) {
    if (xQueueReceive(mac_confirm_fifo, &event, 100)) {
    	debug_printf("mac_sv:recv from fifo :type=%u addr=0x%04X\n", 
    		event.confirm_type, event.confirm_ptr);    	
    }

    if ((data_length = phy_layer_fifo_top_node_size()) > 0) {
      data_length = phy_layer_fifo_pop_data(data);

      // TODO : DSN
      if (mpdu_frame->frame_control.frame_type == mac_frame_type_data) {
        payload_size = data_length - sizeof(mpdu_frame_handle);
        debug_printf("<=== mac_sv data_size=%u msdu_size=%u \n", data_length, payload_size);
        
        mcps_data_indication(mpdu_frame->frame_control.src_addr_mode, mpdu_frame->src_pan_id, mpdu_frame->src_addr,
          mpdu_frame->frame_control.dst_addr_mode, mpdu_frame->dst_pan_id, mpdu_frame->dst_addr,
          payload_size, mpdu_frame->payload, 0, millis());
      
      }

      else if (mpdu_frame->frame_control.frame_type == mac_frame_type_beacon) {
        payload_size = data_length - sizeof(mpdu_beacon_frame_handle);
        debug_printf("<=== mac_sv beacon_size=%u payload_size=%u \n", data_length, payload_size);

        /* Here we store panDescriptor and pendingAddrList in beacon frame payload */
        mcps_beacon_notify_indication(beacon->seq, payload_size, beacon->payload);
      }

      else if (mpdu_frame->frame_control.frame_type == mac_frame_type_command) {
        payload_size = data_length - sizeof(mpdu_frame_handle);
        debug_printf("<=== mac_sv cmd_size=%u msdu_size=%u \n", data_length, payload_size);
        mcps_command_response(mpdu_frame, payload_size);
        //TODO: response this data request
      }

      else {
        debug_printf("<=== mac_sv Unknown frame type!\n");
      }

    }


    vTaskDelay(1);
  }
}

void mcps_command_response(mpdu_frame_handle * mpdu_frame, uint8_t payload_size)
{
  if (mpdu_frame->frame_control.frame_type != mac_frame_type_command) {
    debug_printf("This is not a command frame,ignore\n");
    return;
  }

  if (payload_size == 0) {
    debug_printf("command frame payload size == 0,ignore\n");
  }

  switch (mpdu_frame->payload[0]) {
    case association_request:
      debug_printf("handle association_request\n");
    break;

    case association_response:
      debug_printf("handle association_response\n");
    break;

    case disassociation_notification:
      debug_printf("handle disassociation_notification\n");
    break;

    case data_request:
      debug_printf("handle data_request\n");
    break;

    case pan_id_conflic_notification:
      debug_printf("handle pan_id_conflic_notification\n");
    break;

    case orphan_notification:
      debug_printf("handle orphan_notification\n");
    break;

    case beacon_request:
      // TODO: Judge if this device is a coord ,otherwise it shouldnt handle beacon request
      debug_printf("handle beacon_request\n");
      if (mlme_get_request(macPANCoordinator))
        mcps_handle_beacon_request();
      else
        debug_printf("This device is not coord ,ignore beacon request\n");

    break;

    case coordinator_realignment:
      debug_printf("handle coordinator_realignment\n");
    break;

    case gts_request:
      debug_printf("handle gts_request\n");
    break;

    default:
      debug_printf("Unknow command type!\n");
  }

}

void mcps_handle_beacon_request()
{
  static uint8_t beacon_mem[MPDU_BEACON_MAX_SIZE];
  mpdu_beacon_frame_handle *beacon = (mpdu_beacon_frame_handle *)beacon_mem;
  pan_descriptor_16_handle *pan_descriptor = NULL;
  pending_addr_list *addr_list = NULL;
  mac_beacon_payload_handle *mac_beacon_payload = NULL;
  uint8_t to_send_size = 0;

  /* Send out pending data and pan descriptor */
  
  //TODO: pending data check

  beacon->frame_control.frame_type = mac_frame_type_beacon;
  beacon->frame_control.frame_pending = 0;
  beacon->frame_control.src_addr_mode = mac_addr_16bits;

  beacon->seq ++;
  beacon->src_pan_id = mlme_get_request(macPANId);
  beacon->src_addr = mlme_get_request(macShortAddress);

  pan_descriptor = (pan_descriptor_16_handle *)beacon->payload;
  addr_list = (pending_addr_list *)((uint8_t *)pan_descriptor + sizeof(pan_descriptor_16_handle));

  pan_descriptor->coord_addr_mode = addr_16_bit;
  pan_descriptor->gts_perimit = 0;
  pan_descriptor->link_quality = 0;
  pan_descriptor->coord_pan_id = mlme_get_request(macPANId);
  pan_descriptor->coord_addr = mlme_get_request(macShortAddress);
  pan_descriptor->logical_channel = mlme_get_request(macLogicalChannel);
  pan_descriptor->channel_page = 0;
  pan_descriptor->time_stamp = millis();

  addr_list->size = 0;

  mac_beacon_payload = (mac_beacon_payload_handle *)((uint8_t *)addr_list + sizeof(pending_addr_list) + 
    addr_list->size*2);

  *mac_beacon_payload = mlme_get_request(macBeaconPayload);

  to_send_size = sizeof(mpdu_beacon_frame_handle) + sizeof(pan_descriptor_16_handle)
    + sizeof(pending_addr_list) + sizeof(uint16_t) * addr_list->size + sizeof(mac_beacon_payload_handle);

  debug_printf(">>>> Send out beacon!\n");
  phy_layer_send_raw_data(DEFAULT_BROADCAST_ADDR, (uint8_t *)beacon, to_send_size);
}

/* Not use pending addr param */
void mcps_beacon_notify_indication(uint8_t bsn, uint8_t sdu_length, uint8_t *sdu)
{
  /* Judge beacon type to */
  pan_descriptor_64_handle  pan_descriptor_64 = *(pan_descriptor_64_handle *)sdu;
  pending_addr_list *addr_list = NULL;
  mac_beacon_payload_handle *mac_beacon_payload;
  network_descriptor_handle nwk_descriptor;

  if (pan_descriptor_64.coord_addr_mode == addr_16_bit) {
    debug_printf("mcps_beacon_notify_indication, pan_descriptor is 16bit\n");
    addr_list = (pending_addr_list *)(sdu + sizeof(pan_descriptor_16_handle));
    // TODO: serach add_list to find if this device is in the list
  }
  else if (pan_descriptor_64.coord_addr_mode == addr_64_bit) {
    debug_printf("mcps_beacon_notify_indication, pan_descriptor is 64bit\n");
    addr_list = (pending_addr_list *)(sdu + sizeof(pan_descriptor_64_handle));
  }
  else {
    debug_printf("Unknown type of pan_descriptor\n");
    return;
  }

  mac_beacon_payload = (mac_beacon_payload_handle *)((uint8_t *)addr_list + sizeof(pending_addr_list) + addr_list->size*2);
  
  memcpy(nwk_descriptor.extended_panid, mac_beacon_payload->nwk_extended_panid, 8);
  nwk_descriptor.logical_channel = 0;
  nwk_descriptor.stack_profile = mac_beacon_payload->stack_profile;
  nwk_descriptor.zigbee_version = mac_beacon_payload->nwk_protocal_version;
  nwk_descriptor.beacon_order = 0;
  nwk_descriptor.superframe_order = 0;
  /* permit_joining ? */
  nwk_descriptor.permit_joining = mac_beacon_payload->end_device_capacity;
  nwk_descriptor.router_capacity = mac_beacon_payload->router_capacity;
  nwk_descriptor.end_device_capacity = mac_beacon_payload->end_device_capacity;

  /* Here we consume that pan_descriptor to 64bit addr
     cause in this way we can have space to store both 16bit and 64bit types data
  */
  event_fifo_in(&nwk_pan_descriptors_fifo, &pan_descriptor_64);
  event_fifo_in(&nwk_descriptors_fifo, &nwk_descriptor);
}

void mcps_data_confirm(uint8_t msdu_handle, uint8_t status, uint32_t time_stamp)
{
   static mcps_data_confirm_handle confirm;

   confirm.msdu_handle = msdu_handle;
   confirm.status = status;
   confirm.time_stamp = time_stamp;

   mlme_send_confirm_event(confirm_type_data_confirm, &confirm);
   debug_printf("mcps_data_confirm %u\n", status);
}


void mcps_data_request(uint8_t src_addr_mode, uint8_t dst_addr_mode, uint16_t dst_pan_id, uint16_t dst_addr,
  uint8_t msdu_length, uint8_t *msdu, uint8_t msdu_handle, uint8_t tx_options)
{
  // TODO : addr compressing
  static uint8_t mpdu_mem[MPDU_MAX_SIZE] = {0};
  mpdu_frame_handle *mpdu_frame = (mpdu_frame_handle *)mpdu_mem;
  uint8_t to_send_size = 0;
  uint8_t send_result = 1;

  if (msdu_length > MPDU_PAYLOAD_MAX_SIZE) {
    debug_printf("msdu length overflow\n");
    mcps_data_confirm(msdu_handle, FRAME_TOO_LONG, millis());
    return;
  }

  mpdu_frame->frame_control.frame_type = mac_frame_type_data;
  mpdu_frame->frame_control.security_enable = 0;
  mpdu_frame->frame_control.frame_pending = 0;
  mpdu_frame->frame_control.ack_request = 0;
  mpdu_frame->frame_control.pan_id_compression = 0;
  mpdu_frame->frame_control.dst_addr_mode = mac_addr_16bits;
  mpdu_frame->frame_control.src_addr_mode = mac_addr_16bits;

  mpdu_frame->seq ++;
  mpdu_frame->dst_pan_id = dst_pan_id;
  mpdu_frame->dst_addr = dst_addr;
  mpdu_frame->src_pan_id = mlme_get_request(macPANId);
  mpdu_frame->src_addr = mlme_get_request(macShortAddress);

  memcpy(mpdu_frame->payload, msdu, msdu_length);
  to_send_size = sizeof(mpdu_frame_handle) + msdu_length;
 
  debug_printf("call phy_layer_send_raw_data, dst_addr=0x%04X\n", dst_addr);
  /* here dst addr we can use 0xff00 ,cause this is a broadcast addr ,and anyone can receive it */
  send_result = phy_layer_send_raw_data(dst_addr, (uint8_t *)mpdu_frame, to_send_size);
  if (send_result)
    mcps_data_confirm(msdu_handle, SUCCESS, millis());
  else
    mcps_data_confirm(msdu_handle, TRANSACTION_EXPIRED, millis());

  return;
}

/* Don't use mpduLinkQuality and security options */
/* msdu is the payload */
void mcps_data_indication(uint8_t src_addr_mode, uint16_t src_pan_id, uint16_t src_addr, uint8_t dst_addr_mode,
  uint16_t dst_pan_id, uint16_t dst_addr, uint8_t msdu_length, uint8_t *msdu, uint8_t dsn, uint32_t time_stamp)
{
  //uint8_t payload[MPDU_PAYLOAD_MAX_SIZE] = {0};
  nwk_indication indication;

  //Q: whether should i send params that other than msdu data to the nwk layer?
  //TODO : Judge addr mode

  if ((dst_addr == mlme_get_request(macShortAddress) && dst_pan_id == mlme_get_request(macPANId))
    || (dst_addr == 0xffff) 
    || (mlme_get_request(macPANId) == 0xffff )) {
    indication.length = msdu_length;
    memcpy(indication.data, msdu, msdu_length);
    xQueueSendToBack(nwk_indication_fifo, &indication, 500);
  }
  else {
    debug_printf("mac data ind: recv data, not corspond to my addr\n");
    debug_printf("dst_addr=0x%04X ; my macShortAddress=0x%04X\n", dst_addr, mlme_get_request(macShortAddress));
    debug_printf("dst_pan_id=0x%04X ; my macPANId=0x%04X\n", dst_pan_id, mlme_get_request(macPANId));
  }

}
