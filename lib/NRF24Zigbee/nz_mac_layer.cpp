#include "nz_mac_layer.h"
#include "nz_phy_layer.h"

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

void mlme_scan_request(uint8_t scan_type=0, uint8_t scan_channels=0, uint8_t scan_duration=0, 
    uint8_t channel_i_page=0)
{
  vTaskDelay(10);

  //TODO: real scan implemention

  mlme_scan_confirm(scan_type, scan_channels, channel_i_page);
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

  mlme_send_confirm_event(confirm_type_scan, &scan_confirm);
}

//MLME-START
void mlme_start_request(uint16_t macPANId = 0, uint8_t logicalChannel = 0, uint8_t PANCoordinator = 0,
	uint8_t macBattLifeExt = 0)
{
  if (mlme_get_request(macShortAddress) == 0xffff) {
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
  debug_printf("Enter mac_layer_server\n");
  while (1) {
    if (xQueueReceive(mac_confirm_fifo, &event, 1000 /portTICK_PERIOD_MS)) {
    	debug_printf("mac_sv:recv from fifo :type=%u addr=0x%04X\n", 
    		event.confirm_type, event.confirm_ptr);    	
    }

    vTaskDelay(1);
  }
}


void mcps_data_request(uint8_t src_addr_mode, uint8_t dst_addr_mode, uint16_t dst_pan_id, uint16_t dst_addr,
  uint8_t msdu_length, uint8_t *msdu, uint8_t msdu_handle, uint8_t tx_options)
{
  // TODO : addr compressing
  static uint8_t mpdu_mem[MPDU_MAX_SIZE] = {0};
  mpdu_frame_handle *mpdu_frame = (mpdu_frame_handle *)mpdu_mem;
  uint8_t to_send_size = 0;
  uint8_t phy_dst[2] = {'0', 0xff};
  uint8_t send_result = 1;

  if (msdu_length > MPDU_PAYLOAD_MAX_SIZE) {
    debug_printf("msdu length overflow\n");
    mcps_data_confirm(msdu_handle, FRAME_TOO_LONG, millis());
    return;
  }

  mpdu_frame->seq ++;
  mpdu_frame->dst_pan_id = dst_pan_id;
  mpdu_frame->dst_addr = dst_addr;
  mpdu_frame->src_pan_id = mlme_get_request(macPANId);
  mpdu_frame->src_addr = mlme_get_request(macShortAddress);

  memcpy(mpdu_frame->payload, msdu, msdu_length);
  to_send_size = sizeof(mpdu_frame_handle) + msdu_length;
 
  send_result = phy_layer_send_raw_data(phy_dst, (uint8_t *)mpdu_frame, to_send_size);
  if (send_result)
    mcps_data_confirm(msdu_handle, SUCCESS, millis());
  else
    mcps_data_confirm(msdu_handle, TRANSACTION_EXPIRED, millis());

  return;
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
