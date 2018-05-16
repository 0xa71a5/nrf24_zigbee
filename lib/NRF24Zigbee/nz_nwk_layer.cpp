#include "nz_nwk_layer.h"
#include "nz_common.h"
#include "event_fifo.h"
#include "nz_apl_layer.h"

QueueHandle_t nwk_confirm_fifo;
QueueHandle_t nwk_indication_fifo;

volatile uint8_t scan_confirm_event_flag = 0;
volatile uint8_t start_confirm_event_flag = 0;
volatile uint8_t data_confirm_event_flag = 0;
volatile uint8_t assocation_confirm_event_flag = 0;

#define BEACON_INDICATION_FIFO_SIZE 4
event_node_handle event_pan_des_ptr_area[BEACON_INDICATION_FIFO_SIZE];
pan_descriptor_64_handle nwk_pan_descriptors_mem[BEACON_INDICATION_FIFO_SIZE];
event_fifo_handle nwk_pan_descriptors_fifo;

#define NWK_DESCRIPTOR_FIFO_SIZE 4
event_node_handle event_nwk_desc_ptr_area[NWK_DESCRIPTOR_FIFO_SIZE];
network_descriptor_handle nwk_descriptors_mem[NWK_DESCRIPTOR_FIFO_SIZE];
event_fifo_handle nwk_descriptors_fifo;

#define NWK_ASSOC_FIFO_SIZE 4
event_node_handle event_nwk_assoc_ptr_area[NWK_ASSOC_FIFO_SIZE];
network_associate_handle nwk_assoc_mem[NWK_ASSOC_FIFO_SIZE];
event_fifo_handle nwk_assoc_fifo;

struct NWK_PIB_attributes_handle NWK_PIB_attributes;

void nwk_layer_init()
{
  nwk_confirm_fifo = xQueueCreate(NWK_CONFIRM_FIFO_SIZE, sizeof(confirm_event));
  nwk_indication_fifo = xQueueCreate(NWK_INDICATION_FIFO_SIZE, sizeof(nwk_indication));

  event_fifo_init(&nwk_pan_descriptors_fifo, event_pan_des_ptr_area,
  (uint8_t *)nwk_pan_descriptors_mem, BEACON_INDICATION_FIFO_SIZE, sizeof(pan_descriptor_64_handle));

  event_fifo_init(&nwk_descriptors_fifo, event_nwk_desc_ptr_area,
  (uint8_t *)nwk_descriptors_mem, NWK_DESCRIPTOR_FIFO_SIZE, sizeof(network_descriptor_handle));

  event_fifo_init(&nwk_assoc_fifo, event_nwk_assoc_ptr_area,
  (uint8_t *)nwk_assoc_mem, NWK_ASSOC_FIFO_SIZE, sizeof(network_associate_handle));
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
  uint8_t *nwk_extended_panid = nlme_get_request(nwkExtendedPANID);

  /* scan_type, scan_channels, scan_duration, scan_channel_i_page */
  // do a ed scan and wait for result
  debug_printf("Send ed_scan request to mac\n");

  mlme_scan_request(ed_scan, scan_channels, scan_duration, 0);

  if (signal_wait(&scan_confirm_event_flag, 1000))
    debug_printf("Got ed_scan result\n");
  else {
    debug_printf("Dont get ed_scan result\n");
    //goto fail_exit;
  }

  mlme_scan_request(active_scan, scan_channels, scan_duration, 0);

  if (signal_wait(&scan_confirm_event_flag, 1000))
    debug_printf("Got active_scan result\n");
  else {
    debug_printf("Dont get active_scan result!\n");
    //goto fail_exit;
  }

  /* Set network address and mac address */
  nlme_set_request(nwkNetworkAddress, DEFAULT_COORD_NET_ADDR);
  mlme_set_request(macShortAddress, DEFAULT_COORD_NET_ADDR);

  /* Set network PAN id and mac PAN id */
  nlme_set_request(nwkPANID, DEFAULT_PANID);

  debug_printf("Send start request to mac\n");

  debug_printf("my nwk_extended_panid =");
  for (uint8_t i = 0; i < 8; i ++) {
    debug_printf("%u.", nwk_extended_panid[i]);
    mlme_set_request(macBeaconPayload.nwk_extended_panid[i], nwk_extended_panid[i]);
  }
  debug_printf("\n");

  mlme_set_request(macBeaconPayload.protocal_id, 0x00);
  mlme_set_request(macBeaconPayload.stack_profile, 0x00);
  mlme_set_request(macBeaconPayload.nwk_protocal_version, 0x00);
  mlme_set_request(macBeaconPayload.router_capacity, 0x01);
  mlme_set_request(macBeaconPayload.device_depth, 0x00);
  mlme_set_request(macBeaconPayload.end_device_capacity, 1);
  mlme_set_request(macBeaconPayload.protocal_id, 0x00);
  mlme_set_request(macBeaconPayload.nwk_update_id, nlme_get_request(nwkUpdateId));
  //By default,nwk_extended_panid is the 64bit ieee addr of coord

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

static volatile mlme_scan_confirm_handle  *scan_confirm_ptr = NULL;
static volatile mlme_start_confirm_handle *start_confirm_ptr = NULL;
static volatile mcps_data_confirm_handle  *data_confirm_ptr = NULL;
static volatile mlme_associate_confirm_handle *associate_confirm_ptr = NULL;

void nwk_layer_event_process(void * params)
{
  confirm_event event;
  static nwk_indication indication;
  static network_associate_handle assoc_device;
  npdu_frame_handle * npdu_frame = (npdu_frame_handle *)indication.data;
  uint8_t payload_size = 0;

  debug_printf("Enter nwk sv\n");
  while (1) {
    /* Handle confirm from mac layer */
    if (xQueueReceive(nwk_confirm_fifo, &event, 100)) {
      debug_printf("nwk_sv:recv confirm :type=%u addr=0x%04X\n", 
        event.confirm_type, event.confirm_ptr);

      /* We got confirm signal from mac layer */
      switch (event.confirm_type) {
        case confirm_type_scan:
          scan_confirm_event_flag = 1;
          scan_confirm_ptr = (mlme_scan_confirm_handle *)event.confirm_ptr;
        break;

        case confirm_type_start:
          start_confirm_event_flag = 1;
          start_confirm_ptr = (mlme_start_confirm_handle *)event.confirm_ptr;
        break;

        case confirm_type_set:

        break;

        case confirm_type_data_confirm:
          data_confirm_event_flag = 1;
          data_confirm_ptr = (mcps_data_confirm_handle *)event.confirm_ptr;
        break;

        case confirm_type_association:
          assocation_confirm_event_flag = 1;
          associate_confirm_ptr = (mlme_associate_confirm_handle *)event.confirm_ptr;
        break;
      }
    }

    /* Handle indication from mac layer */
    if (xQueueReceive(nwk_indication_fifo, &indication, 100)) {
      payload_size = indication.length - sizeof(npdu_frame_handle);
      debug_printf("nwk_sv:recv indication,datasize=%u\n", indication.length);

      nlde_data_indication(npdu_frame->frame_control.dst_use_ieee_addr, npdu_frame->dst_addr, npdu_frame->src_addr, 
        payload_size, npdu_frame->payload, millis());
      //TODO : send indication to apl layer
    }

    if (nwk_assoc_fifo.cur_size != 0) {
      if (event_fifo_out(&nwk_assoc_fifo, &assoc_device)) {
        debug_printf("nwk_sv:recv association indication ,last byte = 0x%02X\n", assoc_device.device_addr[7]);
        //Dtermine whether or not accept this device ,if accept,send assoc response
        nlme_association_handle(assoc_device.device_addr);
      }
 
    }

    vTaskDelay(1); 
  }
}

assocation_table_handle assocation_table;



void nlme_association_handle(uint8_t *dev_addr)
{
  uint8_t a_invalid_index = 0xff;
  
  debug_printf("Search my assocation table ,judge if this device is in it.\n");

  for (uint8_t i = 0; i < MAX_ASSOCIATION_ENTRY_SIZE; i ++) {
    uint8_t *addr = assocation_table.entries[i].device_ieee_addr;
    if (assocation_table.entries[i].valid) {

      if ( addr[0]==dev_addr[0] && addr[1]==dev_addr[1] && addr[2]==dev_addr[2]
            && addr[3]==dev_addr[3] && addr[4]==dev_addr[4] && addr[5]==dev_addr[5]
              && addr[6]==dev_addr[6] && addr[7]==dev_addr[7]) {
          debug_printf("Find device already in assocation table, response\n");
          
          mlme_associate_response(dev_addr, assocation_table.entries[i].alloc_addr, assocation_success, nlme_get_request(nwkExtendedPANID));

          /* rejoin indication */
          nlme_join_indication(assocation_table.entries[a_invalid_index].alloc_addr, dev_addr, 0, 1);
          return;
      }

    }
    else {
      a_invalid_index = i;
    }
  }

  if (a_invalid_index != 0xff) {
    //Means we have empty entry to accept this one
    assocation_table.entries[a_invalid_index].valid = 1;
    memcpy(assocation_table.entries[a_invalid_index].device_ieee_addr, dev_addr, 8);
    //TODO: Here we shall calculate a nwk addr to allocate to device 
    assocation_table.entries[a_invalid_index].alloc_addr = (uint16_t)dev_addr[7];
    mlme_associate_response(dev_addr, assocation_table.entries[a_invalid_index].alloc_addr, assocation_success,  nlme_get_request(nwkExtendedPANID));

    nlme_join_indication(assocation_table.entries[a_invalid_index].alloc_addr, dev_addr, 0, 0);
  }
  else {
    //In this case, we dont have enough space for this device,shall we return a response?
    mlme_associate_response(dev_addr, 0xffff, pan_at_capacity, nlme_get_request(nwkExtendedPANID));
  }

}

/* Dont use link quality and security */
void nlde_data_indication(uint8_t dst_addr_mode, uint16_t dst_addr, uint16_t src_addr, 
  uint8_t nsdu_length, uint8_t *nsdu, uint32_t rx_time)
{
  apl_indication indication;

  debug_printf("nwk data ind: dst=0x%04X src=0x%04X len=%u \n", dst_addr, src_addr, nsdu_length);
  if (dst_addr == nlme_get_request(nwkNetworkAddress)) {
    indication.length = nsdu_length;
    memcpy(indication.data, nsdu, nsdu_length);
    xQueueSendToBack(apl_indication_fifo, &indication, 500);
  }
  else {
    debug_printf("nwk data ind: dst addr not equal to mine(0x%04X), drop it\n", nlme_get_request(nwkNetworkAddress));
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
  npdu_frame->frame_control.frame_type = nwk_frame_type_data;
  npdu_frame->frame_control.security = 0;

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
    nlde_data_confirm(data_confirm_ptr->status, data_confirm_ptr->msdu_handle, data_confirm_ptr->time_stamp);
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
  debug_printf("nlde_data_confirm %u 0x%04X\n", status, &confirm);
}

extern volatile uint16_t restore_pan_id;

void nlme_network_discovery_request(uint32_t scan_channels, uint8_t scan_duration)
{
  uint32_t start_time;
  pan_descriptor_64_handle pan_descriptor_64;
  pan_descriptor_16_handle *pan_descriptors_16_ptr;
  uint8_t nwk_count = 0;
 

  //We have to use the 64bit one,
  //cause we dont know true size of it before we read it

  // Do a active scan for sniffering any pan coord
  mlme_scan_request(active_scan, scan_channels, scan_duration, 0);

  // Wait for scan request beacon all sent
  if (signal_wait(&scan_confirm_event_flag, 1000)) {
    debug_printf("Active scan confirm status=%u,all beacon requests sent out\n", scan_confirm_ptr->status);
  }
  else {
    debug_printf("Active scan timeout\n");
    goto fail_exit;
  }

  debug_printf("Waiting for pan_descriptor beacons...\n");
  // Sleep 1000ms to receive beacons
  vTaskDelay(1000);
  mlme_set_request(macPANId, restore_pan_id);

  
  nlme_network_discovery_confirm(scan_confirm_ptr->status);

  /*
  //!!!This will trigger an error,dont know why
  if (nwk_pan_descriptors_fifo.cur_size != 0) {
    debug_printf("Got %u pan_descriptors!\n", nwk_pan_descriptors_fifo.cur_size);
    debug_printf("###### PAN DESCRIPTOR PRINT ######\n");
    while (event_fifo_out(&nwk_pan_descriptors_fifo, &pan_descriptor_64)) {
      if (pan_descriptor_64.coord_addr_mode == addr_16_bit) {
        pan_descriptors_16_ptr = (pan_descriptor_16_handle *)&pan_descriptor_64;
        debug_printf("PANid=0x%04X CoordAddr=0x%04X\n", 
          pan_descriptors_16_ptr->coord_pan_id, pan_descriptors_16_ptr->coord_addr);
      }
      else {
        debug_printf("PANid=0x%04X CoordAddr=0x%04X\n", 
          pan_descriptor_64.coord_pan_id, pan_descriptor_64.coord_addr);
      }

    }
    debug_printf("##################################\n\n");
  }
  else {
    debug_printf("Dont got any beacons!\n");
  }
  */

  return;

  fail_exit:
  nlme_network_discovery_confirm(STARTUP_FAILURE);
  return;

}


void nlme_network_discovery_confirm(uint8_t status)
{
  static nlme_nwk_discovery_confirm_handle confirm;

  confirm.status = status;
  nlme_send_confirm_event(confirm_type_nwk_discovery, &confirm);
  debug_printf("nlme_nwk_discovery_confirm %u\n", status);
}

// We ignore security_enable param
void nlme_join_request(uint8_t *extended_panid, uint8_t rejoin_network, uint16_t short_panid,
  uint8_t scan_duration, uint8_t capability)
{
  //TODO: Judge current join status and rejoin param
  //TODO: get panId
  mlme_associate_request(0, 0, short_panid, extended_panid, capability);
  //wait for join result.

  if (signal_wait(&assocation_confirm_event_flag, 1200)) {
    debug_printf("nlme join got confirm\n");
    nlme_join_confirm(associate_confirm_ptr->status, associate_confirm_ptr->assoc_short_addr, associate_confirm_ptr->extended_panid, 0);
  }
  else {
    debug_printf("nlme join timeout\n");
    nlme_join_confirm(pan_access_denied, 0xffff, nlme_get_request(nwkExtendedPANID), 0);
  }

}

void nlme_join_confirm(uint8_t status, uint16_t nwk_addr, uint8_t *extended_panid, uint8_t active_channel)
{
  static nlme_join_confirm_handle confirm;

  confirm.status = status;
  confirm.nwk_addr = nwk_addr;
  memcpy(confirm.extended_panid, extended_panid, 8);
  confirm.active_channel = active_channel;
  nlme_send_confirm_event(confirm_type_join, &confirm);
}

void nlme_join_indication(uint16_t nwk_addr, uint8_t *extended_addr, uint8_t capability, uint8_t rejoin_network)
{
  static nwk_join_indication_handle indication;
  
  indication.nwk_addr = nwk_addr;
  memcpy(indication.extended_addr, extended_addr, 8);
  indication.capability = capability;
  indication.rejoin_network = rejoin_network;
  debug_printf("nlme_join_indication nwk_addr=0x%04X\n", nwk_addr);
  //event_fifo_in(&nwk_join_ind_fifo, (uint8_t *)&indication);
}

