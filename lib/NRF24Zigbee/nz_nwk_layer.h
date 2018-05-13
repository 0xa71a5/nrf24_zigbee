#ifndef NZ_NWK_LAYER_H
#define NZ_NWK_LAYER_H

#include "NRF24Zigbee.h"
#include <FreeRTOS_AVR.h>
#include "nz_apl_layer.h"
#include "nz_mac_layer.h"
#include "nz_common.h"

#define STARTUP_FAILURE 20
#define NWK_CONFIRM_FIFO_SIZE 3
#define FORMATION_CONFIRM_TIMEOUT 100

typedef struct __nlme_formation_confirm_handle
{
	uint8_t status;
} nlme_formation_confirm_handle;

enum nwk_confirm_type_enum {
  confirm_type_formation = 0
};

extern QueueHandle_t nwk_confirm_fifo;

void nwk_layer_init();
void nlme_send_confirm_event(uint8_t confirm_type, void *ptr);
void nlme_network_formation_request();
void nlme_network_formation_confirm(uint8_t status);
void nwk_layer_event_process(void * params);



#endif