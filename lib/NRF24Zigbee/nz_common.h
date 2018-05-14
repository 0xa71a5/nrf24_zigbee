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

#endif