#ifndef NZ_NWK_LAYER_H
#define NZ_NWK_LAYER_H

#include "NRF24Zigbee.h"
#include <FreeRTOS_AVR.h>
#include "nz_common.h"
#include "nz_apl_layer.h"
#include "nz_mac_layer.h"

#define STARTUP_FAILURE 20
#define NWK_CONFIRM_FIFO_SIZE 3
#define NWK_INDICATION_FIFO_SIZE 3
#define FORMATION_CONFIRM_TIMEOUT 100


extern QueueHandle_t nwk_confirm_fifo;
extern QueueHandle_t nwk_indication_fifo;
extern event_fifo_handle nwk_pan_descriptors_fifo;

#define nlme_set_request(perp_name, value) NWK_PIB_attributes.perp_name = value
/* We dont use set_confirm , cause this is some kind a waste */
#define nlme_get_request(perp_name) NWK_PIB_attributes.perp_name





typedef struct __nlme_formation_confirm_handle
{
	uint8_t status;
} nlme_formation_confirm_handle;

extern nlme_formation_confirm_handle *apl_data_confirm_ptr;

extern struct NWK_PIB_attributes_handle NWK_PIB_attributes;

extern event_fifo_handle nwk_descriptors_fifo;

struct NWK_PIB_attributes_handle {
	uint8_t nwkMaxBroadcastRetries:3;//range 0 - 5
	uint8_t nwkReportConstantCost:1;
	uint8_t nwkSymLink:1;
	uint8_t nwkAddrAlloc:2;
	uint8_t nwkUseTreeRouting:1;
	uint8_t nwkStackProfile:4;
	uint8_t nwkUseMulticast:1;
	uint8_t nwkIsConcentrator:1;
	uint8_t nwkUniqueAddr:1;
	uint8_t nwkTimeStamp:1;

	uint8_t nwkSequenceNumber;// seq num add to outgoing frames
	uint16_t nwkPassiveAckTimeout;
	uint8_t nwkMaxChildren;//range 0x00 - 0xff
	uint8_t nwkMaxDepth;//range 0x00 - 0xff
	uint8_t nwkMaxRoutes;//range 0x01 - 0xff
	uint8_t *nwkNeighborTable;//current set of neighbor table entries in the device
	uint32_t nwkNetworkBroadcaseDeliveryTime;//time duration that a broadcast message needs to encompass the entir network


	uint8_t *nwkRouteTable;
	uint8_t nwkCapabilityInfomation;
	uint16_t nwkManagerAddr;
	uint8_t nwkMaxSourceRoute;
	uint8_t nwkUpdateId;
	//uint8_t nwkTransactionPersistenceTime;//not used

	uint16_t nwkNetworkAddress;//this reflects MAC PIB macShortAddress
	uint8_t *nwkBroadcastTransactionTable;
	uint8_t *nwkGroupIDTable;
	uint8_t nwkExtendedPANID[8];
	uint8_t *nwkRouteRecordTable;
	uint8_t nwkConcentratorRadius;
	uint8_t nwkConcentratorDiscoveryTime;
	uint8_t nwkLinkStatusPeriod;
	uint8_t nwkRouteAgeLimit;
	uint8_t *nwkAddressMap;
	uint16_t nwkPANID;
	uint16_t nwkTxTotal;
	uint8_t nwkLeaveRequestAllowed:1;
};


enum nwk_frame_type {
  nwk_frame_type_data = 0,
  nwk_frame_type_command,
  nwk_frame_type_reserved,
  nwk_frame_type_inter_pan,
};

enum nwk_discovery_route_type {
  suppress_route_discovery = 0,
  enable_route_discovery,
};

enum nwk_multicast_type {
  unicast_or_broadcast_frame = 0,
  multicast_frame,
};

enum nwk_soure_route_type {
  source_route_not_present = 0,
  source_route_present,
};



typedef struct __nlde_data_confirm_handle {
	uint8_t status;
	uint8_t nsdu_handle;
	uint32_t tx_time;
} nlde_data_confirm_handle;

typedef struct __nwk_indicaiton_handle
{
	uint8_t length;
	uint8_t data[NPDU_MAX_SIZE];
} nwk_indication;



void nwk_layer_init();
void nlme_send_confirm_event(uint8_t confirm_type, void *ptr);
void nlme_network_formation_request(uint8_t scan_channels, uint8_t scan_duration, uint8_t battery_life_ext);
void nlme_network_formation_confirm(uint8_t status);
void nwk_layer_event_process(void * params);
void nlde_data_confirm(uint8_t status, uint8_t nsdu_handle, uint32_t tx_time);
void nlde_data_request(uint16_t dst_addr, uint8_t nsdu_length, uint8_t *nsdu, uint8_t nsdu_handle, uint8_t broadcast_radius,
	uint8_t discovery_route);
void nlde_data_confirm(uint8_t status, uint8_t npdu_handle, uint32_t tx_time);
void nlde_data_indication(uint8_t dst_addr_mode, uint16_t dst_addr, uint16_t src_addr, 
  uint8_t nsdu_length, uint8_t *nsdu, uint32_t rx_time);
void nlme_network_discovery_request(uint32_t scan_channels, uint8_t scan_duration);
void nlme_network_discovery_confirm();

#endif