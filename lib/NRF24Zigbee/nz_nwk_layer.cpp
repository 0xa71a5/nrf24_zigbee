#include "nz_nwk_layer.h"
#include "nz_common.h"

QueueHandle_t nwk_confirm_fifo;

volatile uint8_t scan_confirm_event_flag = 0;
volatile uint8_t start_confirm_event_flag = 0;

void nwk_layer_init()
{
  nwk_confirm_fifo = xQueueCreate(NWK_CONFIRM_FIFO_SIZE, sizeof(confirm_event));
}

void nlme_send_confirm_event(uint8_t confirm_type, void *ptr)
{
  confirm_event event;

  event.confirm_type = confirm_type;
  event.confirm_ptr = (uint8_t *)ptr;
  xQueueSendToBack(apl_confirm_fifo, &event, pdMS_TO_TICKS(1000));
}



/* Format a new network request */
void nlme_network_formation_request_old()
{
  uint32_t notify_value = 0;
  uint32_t record_time = 0;
  confirm_event event;

  /* scan_type, scan_channels, scan_duration, scan_channel_i_page */
  // do a ed scan and wait for result
  debug_printf("Send ed_scan request to mac\n");
  
  scan_confirm_event_flag = 0;
  mlme_scan_request(ed_scan, 0, 0, 0);

  record_time = millis();
  while (!scan_confirm_event_flag) {
    vTaskDelay(1);
    if (millis() - record_time > FORMATION_CONFIRM_TIMEOUT)
      break;
  }

  if (scan_confirm_event_flag == 0)
    goto fail_exit;
  else {
    scan_confirm_event_flag = 0;
    debug_printf("Got ed_scan result\n");
  }


  scan_confirm_event_flag = 0;
  mlme_scan_request(active_scan, 0, 0, 0);

  record_time = millis();
  while (!scan_confirm_event_flag) {
    vTaskDelay(1);
    if (millis() - record_time > FORMATION_CONFIRM_TIMEOUT)
      break;
  }

  if (scan_confirm_event_flag == 0)
    goto fail_exit;
  else {
    scan_confirm_event_flag = 0;
    debug_printf("Got active_scan result\n");
  }


  mlme_set_request(macShortAddress, 0x0034);

  debug_printf("Send start request to mac\n");
  start_confirm_event_flag = 0;
  /* macPANId, logicalChannel, PANCoordinator ,macBattLifeExt*/
  mlme_start_request(1, 12, 1, 0);

  while (!start_confirm_event_flag) {
    vTaskDelay(1);
    if (millis() - record_time > FORMATION_CONFIRM_TIMEOUT)
      break;
  }

  if (start_confirm_event_flag == 0)
    goto fail_exit;
  else {
    start_confirm_event_flag = 0;
    debug_printf("Got active_scan result\n");
  }

  nlme_network_formation_confirm(SUCCESS);
  return;

  fail_exit:
  nlme_network_formation_confirm(STARTUP_FAILURE);
}

/* Format a new network request */
void nlme_network_formation_request()
{
  uint32_t notify_value = 0;
  uint32_t record_time = 0;
  confirm_event event;

  /* scan_type, scan_channels, scan_duration, scan_channel_i_page */
  // do a ed scan and wait for result
  debug_printf("Send ed_scan request to mac\n");

  mlme_scan_request(ed_scan, 0, 0, 0);

  if (signal_wait(&scan_confirm_event_flag, 100))
    debug_printf("Got ed_scan result\n");
  else {
    goto fail_exit;
  }

  mlme_scan_request(active_scan, 0, 0, 0);

  if (signal_wait(&scan_confirm_event_flag, 100))
    debug_printf("Got active_scan result\n");
  else {
    goto fail_exit;
  }

  mlme_set_request(macShortAddress, 0x0034);

  debug_printf("Send start request to mac\n");

  /* macPANId, logicalChannel, PANCoordinator ,macBattLifeExt*/
  mlme_start_request(1, 12, 1, 0);

  if (signal_wait(&start_confirm_event_flag, 100))
    debug_printf("Got start result\n");
  else {
    goto fail_exit;
  }


  nlme_network_formation_confirm(SUCCESS);
  return;

  fail_exit:
  debug_printf("fail_exit\n");
  nlme_network_formation_confirm(STARTUP_FAILURE);
}

/* Format a new network confirm */
void nlme_network_formation_confirm(uint8_t status)
{
  nlme_formation_confirm_handle confirm;

  confirm.status = status;
  nlme_send_confirm_event(confirm_type_formation, &confirm);
  debug_printf("nlme_network_formation_confirm %u\n", status);
}


void nwk_layer_event_process(void * params)
{
  confirm_event event;

  while (1) {
    if (xQueueReceive(nwk_confirm_fifo, &event, pdMS_TO_TICKS(500))) {
      debug_printf("nwk_sv:recv from fifo :type=%u addr=0x%04X\n", 
        event.confirm_type, event.confirm_ptr);

      /* We got confirm signal from mac layer */
      switch (event.confirm_type) {
        case confirm_type_scan:
          scan_confirm_event_flag = 1;
        break;

        case confirm_type_start:
          start_confirm_event_flag = 1;
        break;

        case confirm_type_set:

        break;
      }
    }

    vTaskDelay(1); 
  }
}
