#ifndef NZ_MAC_LAYER_H
#define NZ_MAC_LAYER_H

#include "NRF24Zigbee.h"
#include <FreeRTOS_AVR.h>
#include "nz_common.h"

//MLME-SCAN
enum mlme_scan_type {
  ed_scan = 0, /* optional for rfd */
  active_scan, /* optional for rfd */
  passive_scan,
  orphan_scan,
};




#define MAX_PAN_FIND 3
#define MAX_ENERGY_LIST_DETECT 0

typedef struct __mlme_scan_confirm_handle { /* Sizeof = 11 */
  uint8_t status:4;
  uint8_t scan_type:4;
  uint8_t channel_page:4;
  uint8_t result_list_size:4;
  uint32_t unscaned_channels;
  uint8_t energy_detect_list[MAX_ENERGY_LIST_DETECT + 1];/* First byte is size*/
  uint8_t pan_descript_list[MAX_PAN_FIND + 1];/* First byte is size */
} mlme_scan_confirm_handle;

typedef struct __mlme_start_confirm_handle
{
	uint8_t status;
} mlme_start_confirm_handle;

typedef struct __mcps_data_confirm_handle {
  uint8_t status;
  uint8_t msdu_handle;
  uint32_t time_stamp;
} mcps_data_confirm_handle;

enum coord_addr_mode_enum {
  addr_16_bit = 2,
  addr_64_bit = 3,
};

typedef struct __pan_descriptor_16 {
  uint8_t coord_addr_mode:2;
  uint8_t gts_perimit:1;
  uint8_t link_quality:5;
  uint16_t coord_pan_id;
  uint16_t coord_addr;
  uint8_t logical_channel;
  uint8_t channel_page;
  uint8_t superframe_spec;
  uint32_t time_stamp;
} pan_descriptor_16_handle;

typedef struct __pan_descriptor_64 {
  uint8_t coord_addr_mode:2;
  uint8_t gts_perimit:1;
  uint8_t link_quality:5;
  uint16_t coord_pan_id;
  uint8_t coord_addr[8];
  uint8_t logical_channel;
  uint8_t channel_page;
  uint8_t superframe_spec;
  uint32_t time_stamp;
} pan_descriptor_64_handle;

typedef struct __pending_addr_list {
  uint8_t size;
  uint16_t addr[0];
} pending_addr_list;


/* Currently, size of MAC_PIB_attributes = 24 */
struct MAC_PIB_attributes_handle {
  uint8_t macAckWaitDuration;//0x40 int, the max number of symbols wait for an ack frame folling a transimited data frame
  
  uint8_t macAssociatedPANCoord:1;//0x56 bool, indicate if device is associated to the pan through pan coord
  uint8_t macAssociationPermit:1;//0x41 bool, whether allowing association
  uint8_t macBattLifeExt:1;//0x43 bool, for power save
  uint8_t macBattLifeExtPeriods:5;//0x44 int, not used
  
  //uint8_t macAutoRequest:1;//0x42 bool, whether device auto sends a data request command if its addr is in beacon, not used
  //uint8_t *macBeaconPayload;//0x45 array
  //uint8_t macBeaconPayloadLength;//0x46 int
  //uint8_t macBeaconOrder;//0x47 int
  //uint16_t macBeaconTxTime;//0x48 int
  //uint8_t macBSN;//0x49 seq num added to the transmitted beacon frame
  //uint8_t macMaxFrameTotalWaitTime; not used in non-beacon net
  //uint8_t macResponseWaitTime:6;//not used
  
  uint8_t macDSN;//0x4c seq num added to transmitted data or mac command frame

  uint8_t macGTSPermit:1;//0x4d bool, true to accept GTS request
  uint8_t macMaxBE:4;//0x57 integer ,the max value of the back off exponent in CSMA-CA,range 3-8
  uint8_t macMaxCSMABackoffs:3;//0x4e ,the max value of the back off times in CSMA-CA,range 0-5

  uint8_t macMaxFrameRetries:4;//max retry allowed after tx failure
  uint8_t macMinBE:4;//minimum number of backoff exponent 
  
  uint8_t macMinLIFSPeriod:4;// the min number of symbols forming a LIFS period
  uint8_t macMinSIFSPeriod:4;// the min number of symbols forming a SIFS period

  
  uint8_t macPromiscuousMode:1;//indication whether mac is in a promiscuous mode(receive all)
  uint8_t macRxOnWhenIdle:1;//
  uint8_t macSecurityEnabled:1;//whether mac has security enabled
  uint8_t macTimestampSupported:1;
  uint8_t macPANCoordinator:1;// whether the device self is a PAN Coordinator

  uint8_t  macCoordExtendedAddress[8];//0x4a 64bit IEEE addr
  uint8_t  macCoordShortAddress[2];//0x4b 16bit short address to coord which device associated
  uint16_t macPANId;//the 16 bit identifier of PAN, if the value is 0xffff then device is not associated
  uint16_t macShortAddress;// the 16 bit addr that device used to communicate in PAN,
  //if dev is coord this value shall chosen before a PAN started 
  //otherwise the addr is allocated by a coord durion assocation
  uint8_t macLogicalChannel;

  //uint8_t macSuperframeOrder;//not used
  uint8_t macSyncSymbolOffset;
  uint16_t macTransactionPersistenceTime;//the max time a transaction is stored by a coord
};

extern struct MAC_PIB_attributes_handle MAC_PIB_attributes;
extern QueueHandle_t mac_confirm_fifo;

#define mlme_set_request(perp_name, value) MAC_PIB_attributes.perp_name = value
/* We dont use set_confirm , cause this is some kind a waste */
#define mlme_get_request(perp_name) MAC_PIB_attributes.perp_name

void mac_layer_init();

void mlme_scan_request(uint8_t scan_type=0, uint32_t scan_channels=0, uint8_t scan_duration=0, 
    uint8_t channel_i_page=0);

void mlme_scan_confirm(uint8_t status=0, uint8_t scan_type=0, uint8_t channel_page=0, uint32_t unscaned_channels=0,
  uint16_t result_list_size=0, uint8_t *energy_detect_list=0, uint8_t *pan_descript_list=0);

void mlme_start_request(uint16_t macPANId = 0, uint8_t logicalChannel = 0, uint8_t PANCoordinator = 0,
	uint8_t macBattLifeExt = 0);

void mlme_start_confirm(uint8_t status);

void mac_layer_event_process(void * params);

void mcps_data_request(uint8_t src_addr_mode, uint8_t dst_addr_mode, uint16_t dst_pan_id, uint16_t dst_addr,
  uint8_t msdu_length, uint8_t *msdu, uint8_t msdu_handle, uint8_t tx_options);

void mcps_data_confirm(uint8_t msdu_handle, uint8_t status, uint32_t time_stamp);

void mcps_data_indication(uint8_t src_addr_mode, uint16_t src_pan_id, uint16_t src_addr, uint8_t dst_addr_mode,
  uint16_t dst_pan_id, uint16_t dst_addr, uint8_t msdu_length, uint8_t *msdu, uint8_t dsn, uint32_t time_stamp);

void mcps_beacon_notify_indication(uint8_t bsn, uint8_t sdu_length, uint8_t *sdu);

void mcps_command_response(mpdu_frame_handle * mpdu_frame, uint8_t payload_size);

void mcps_handle_beacon_request();

#endif