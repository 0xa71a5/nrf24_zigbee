#include "nz_common.h"


bool signal_wait(uint8_t * signal, uint16_t delay_time = 100)
{
  uint32_t record_time;
  bool result;
  TickType_t xLastWakeTime;

  *signal = 0;

  record_time = millis();

  while (!*signal) {
    xLastWakeTime = xTaskGetTickCount();
    vTaskDelayUntil(&xLastWakeTime, 250);
    if (millis() - record_time > delay_time)
      break;
  }

  result = (*signal != 0);
  
  *signal = 0;
  return result;
}