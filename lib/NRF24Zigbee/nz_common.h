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



#endif