#ifndef NZ_APL_LAYER_H
#define NZ_APL_LAYER_H

#include "NRF24Zigbee.h"
#include <FreeRTOS_AVR.h>
#include "nz_mac_layer.h"
#include "nz_nwk_layer.h"
#include "nz_common.h"

#define APL_CONFIRM_FIFO_SIZE 2
#define APL_INDICATION_FIFO_SIZE 2



extern QueueHandle_t apl_confirm_fifo;
extern QueueHandle_t apl_indication_fifo;
extern volatile uint8_t formation_confirm_event_flag;
extern volatile uint8_t apl_data_confirm_event_flag;
extern volatile uint8_t apl_nwk_discovery_event_flag;
extern event_fifo_handle nwk_join_ind_fifo;
extern volatile uint8_t apl_join_confirm_event_flag;

extern volatile nlme_join_confirm_handle *apl_join_confirm_ptr;

typedef struct __apl_indication_handle
{
	uint8_t length;
	uint8_t data[APDU_MAX_SIZE];
} apl_indication;


void apl_layer_init();
void apl_layer_event_process(void *params);

#endif