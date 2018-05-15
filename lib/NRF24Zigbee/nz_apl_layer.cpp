#include "nz_apl_layer.h"


QueueHandle_t apl_confirm_fifo;
QueueHandle_t apl_indication_fifo;

volatile uint8_t formation_confirm_event_flag = 0;
volatile uint8_t apl_data_confirm_event_flag = 0;


void apl_layer_init()
{
  apl_confirm_fifo = xQueueCreate(APL_CONFIRM_FIFO_SIZE, sizeof(confirm_event));
  apl_indication_fifo = xQueueCreate(APL_INDICATION_FIFO_SIZE, sizeof(apl_indication));
  /* Do a default setting of PIB attributes */
}

nlme_formation_confirm_handle *apl_data_confirm_ptr;

void apl_layer_event_process(void *params)
{
  confirm_event event;
  static apl_indication indication;

  while (1) {
    if (xQueueReceive(apl_confirm_fifo, &event, 230)) {
      debug_printf("apl_sv:recv from fifo :type=%u addr=0x%04X\n", 
        event.confirm_type, event.confirm_ptr);

      /* We got confirm signal from nwk layer */
      switch (event.confirm_type) {
        case confirm_type_formation:
          formation_confirm_event_flag = 1;
        break;

        case confirm_type_data_confirm:
          apl_data_confirm_event_flag = 1;
          apl_data_confirm_ptr = (nlme_formation_confirm_handle *)event.confirm_ptr;
        break;

      }
    }

    

    vTaskDelay(1); 
  }
}