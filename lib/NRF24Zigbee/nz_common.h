#ifndef NZ_COMMON_H
#define NZ_COMMON_H

#include "NRF24Zigbee.h"
#include <FreeRTOS_AVR.h>

typedef struct __confirm_event
{
  uint8_t confirm_type;
  uint8_t *confirm_ptr;
} confirm_event;

bool signal_wait(uint8_t * signal, uint16_t delay_time = 100);

/* Definition of status */
#define SUCCESS 				0
#define LIMIT_REACHED 			1
#define NO_BEACON 				2
#define SCAN_IN_PROGRESS		3
#define COUNTER_ERROR			4
#define FRAME_TOO_LONG			5
#define UNAVAILABLE_KEY 		6
#define UNSUPPORTED_SECURITY	7
#define INVALID_PARAMETER 		8
#define NO_SHORT_ADDRESS 		9
#define	TRACKING_OFF			10
#define	CHANNEL_ACCESS_FAILURE	11
#define TRANSACTION_OVERFLOW	12
#define TRANSACTION_EXPIRED		13
#define INVALID_ADDRESS			14
#define INVALID_GTS				15
#define NO_ACK					16

#define DEFAULT_LOGICAL_CHANNEL 3

#define DEFAULT_PANID 0x07

#define DEFAULT_BROADCAST_ADDR 0xff00
#define DEFAULT_COORD_NET_ADDR 0x0100
#define DEFAULT_DEVICE_NET_ADDR 0x0200

enum confirm_types {
	confirm_type_scan = 0,
  	confirm_type_set,
  	confirm_type_start,
  	confirm_type_formation,
  	confirm_type_data_confirm,
};



enum mac_frame_type {
  mac_frame_type_beacon = 0,
  mac_frame_type_data,
  mac_frame_type_ack,
  mac_frame_type_command,
};

enum mac_addr_mode {
  mac_addr_no_present = 0,
  mac_addr_16bits = 2,
  mac_addr_64bits = 3,
};

enum mac_frame_version {
  mac_version_802_15_4_2003 = 0,
  mac_version_802_15_4 = 1,
};

typedef struct __mpdu_frame_control {
  uint8_t frame_type:3;
  uint8_t security_enable:1;
  uint8_t frame_pending:1;
  uint8_t ack_request:1;
  uint8_t pan_id_compression:1;
  uint8_t reserved:3;
  uint8_t dst_addr_mode:2;
  uint8_t frame_version:2;
  uint8_t src_addr_mode:2;
} mpdu_frame_control;

typedef struct __mpdu_addressing_field {
  uint8_t a;
} mpdu_addressing_field;

typedef struct __mpdu_frame_handle {
  uint16_t crc;/* This is not like protocal */
  mpdu_frame_control frame_control;
  uint8_t seq;
  uint16_t dst_pan_id;
  uint16_t dst_addr;
  uint16_t src_pan_id;
  uint16_t src_addr;
  uint8_t payload[0];
} mpdu_frame_handle;


#define MPDU_MAX_SIZE 128
#define MPDU_PAYLOAD_MAX_SIZE (MPDU_MAX_SIZE - sizeof(mpdu_frame_handle))

typedef struct __npdu_frame_control {
  uint8_t frame_type:2;
  uint8_t protocal_version:4;
  uint8_t discovery_route:2;
  uint8_t multicast_flag:1;
  uint8_t security:1;
  uint8_t source_route:1;
  uint8_t dst_use_ieee_addr:1;
  uint8_t src_use_ieee_addr:1;
  uint8_t reserved:3;
} npdu_frame_control;

typedef struct __npdu_frame_route {
  uint16_t dest_addr;
  uint16_t src_addr;
  uint8_t radius;
  uint8_t seq;
  uint8_t multicast_control;
} npdu_frame_route;

typedef struct __npdu_frame_wrapper_handle {
  npdu_frame_control frame_control;
  npdu_frame_route   frame_route;
  uint8_t frame_data[0];
} npdu_frame_wrapper_handle;

typedef struct __npdu_frame_handle {
  npdu_frame_control frame_control;
  uint16_t dst_addr;
  uint16_t src_addr;
  uint8_t radius;
  uint8_t seq;
  uint8_t multicast_control;
  uint8_t payload[0];
} npdu_frame_handle;

#define NPDU_MAX_SIZE  MPDU_PAYLOAD_MAX_SIZE
#define NPDU_FRAME_OVERHEAD_SIZE sizeof(npdu_frame_handle)
#define NPDU_PAYLOAD_MAX_SIZE (NPDU_MAX_SIZE - NPDU_FRAME_OVERHEAD_SIZE)
#define APDU_MAX_SIZE NPDU_PAYLOAD_MAX_SIZE

#endif