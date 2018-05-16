#include "nz_apl_layer.h"


QueueHandle_t apl_confirm_fifo;
QueueHandle_t apl_indication_fifo;

#define NWK_JOIN_INDICATION_FIFO_SIZE 3
event_node_handle event_join_ind_ptr_area[NWK_JOIN_INDICATION_FIFO_SIZE];
nwk_join_indication_handle nwk_join_ind_mem[NWK_JOIN_INDICATION_FIFO_SIZE];
event_fifo_handle nwk_join_ind_fifo;

volatile uint8_t formation_confirm_event_flag = 0;
volatile uint8_t apl_data_confirm_event_flag = 0;
volatile uint8_t apl_nwk_discovery_event_flag = 0;
volatile uint8_t apl_join_confirm_event_flag = 0;

void apl_layer_init()
{
  apl_confirm_fifo = xQueueCreate(APL_CONFIRM_FIFO_SIZE, sizeof(confirm_event));
  apl_indication_fifo = xQueueCreate(APL_INDICATION_FIFO_SIZE, sizeof(apl_indication));

  event_fifo_init(&nwk_join_ind_fifo, event_join_ind_ptr_area,
  (uint8_t *)nwk_join_ind_mem, NWK_JOIN_INDICATION_FIFO_SIZE, sizeof(nwk_join_indication_handle));

}

volatile nlme_formation_confirm_handle *apl_data_confirm_ptr = NULL;
volatile nlme_nwk_discovery_confirm_handle *apl_nwk_discovery_ptr = NULL;
volatile nlme_join_confirm_handle *apl_join_confirm_ptr = NULL;

void apl_layer_event_process(void *params)
{
  confirm_event event;
  static apl_indication indication;
  static nwk_join_indication_handle join_indication;

  debug_printf("Enter apl layer event process\n");
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
          apl_data_confirm_ptr = (nlme_formation_confirm_handle *)event.confirm_ptr;
          apl_data_confirm_event_flag = 1;
        break;

        case confirm_type_nwk_discovery:
          apl_nwk_discovery_ptr = (nlme_nwk_discovery_confirm_handle *)event.confirm_ptr;
          apl_nwk_discovery_event_flag = 1;
        break;

        case confirm_type_join:
          apl_join_confirm_ptr = (nlme_join_confirm_handle *)event.confirm_ptr;
          apl_join_confirm_event_flag = 1;
        break;

        default:
          debug_printf("Unknown type of event confirm\n");
        break;
      }
    }

    if (nwk_join_ind_fifo.cur_size != 0) {
      debug_printf("apl_sv: new device join this network\n");
      event_fifo_out(&nwk_join_ind_fifo, &join_indication);
    }

    vTaskDelay(1); 
  }
}