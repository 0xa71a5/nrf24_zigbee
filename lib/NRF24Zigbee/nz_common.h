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

#endif