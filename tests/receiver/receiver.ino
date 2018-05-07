#include <NRF24Zigbee.h>

#define pr_err(...) printf("[-] ");printf(__VA_ARGS__)
#define pr_info(...) printf("[+] ");printf(__VA_ARGS__)
#define pr_debug(...) printf("[?] ");printf(__VA_ARGS__)

int serial_putc( char c, struct __file * )
{
  Serial.write( c );
  return c;
}
void printf_begin(void)
{
  fdevopen( &serial_putc, 0 );
}


void print_info()
{
  printf("\n############### REG TRACE START ###############\n");
  printf("EN_AA     = 0x%02X\n", read_register(EN_AA));
  printf("EN_RXADDR = 0x%02X\n", read_register(EN_RXADDR));
  printf("RX_ADDR_P2= '%c' RX_PW_P2=%u\n", read_register(RX_ADDR_P2), read_register(RX_PW_P2));
  printf("RX_ADDR_P3= '%c' RX_PW_P3=%u\n", read_register(RX_ADDR_P3), read_register(RX_PW_P3));
  printf("RX_ADDR_P4= '%c' RX_PW_P4=%u\n", read_register(RX_ADDR_P4), read_register(RX_PW_P4));
  printf("###############  REG TRACE END  ###############\n\n");
}

struct phy_packet_handle {
  uint8_t type:2;
  uint8_t length:5;
  uint8_t packet_index:3;
  uint8_t slice_size:3;
  uint8_t slice_index:3;
  uint8_t crc:8;
  uint8_t data[0];
};

void test()
{
  uint8_t raw_packet[32] = {0};
  struct phy_packet_handle *input = (struct phy_packet_handle *)raw_packet;
  input->type = 0x00;
  input->length = 20;
  input->packet_index = 5;
  input->slice_size = 4;
  input->slice_index = 3;
  input->crc = 0xf3;
  for (int i = 0; i < input->length; i ++)
    input->data[i] = i;

  printf("Array = 0x%02X 0x%02X 0x%02X 0x%02X\n", 
    raw_packet[0], raw_packet[1], raw_packet[2], raw_packet[3]);
  
  struct phy_packet_handle *parser= (struct phy_packet_handle *)raw_packet;
  printf("type = %u\n", parser->type);
  printf("length = %u\n", parser->length);
  printf("index = %u\n", parser->packet_index);
  printf("slice_size = %u\n", parser->slice_size);
  printf("slice_index = %u\n", parser->slice_index);
  printf("crc = 0x%02X\n", parser->crc);
  printf("Print data ===> \n");
  for (int i = 0; i < input->length; i ++) {
    if (i % 7 == 0)
      printf("\n");
    printf("0x%02X ", input->data[i]);
  }

  //while(1);
}

#define MAX_PACKET_INDEX 8

#define MAX_PACKET_DATA_SIZE 29

void nrf_set_broadcast_addr(uint8_t addr)
{
  config_register(RX_ADDR_P2, addr);
}

bool phy_layer_data_ready(void)
{
  return nrf_data_ready();
}

struct phy_rx_node_handle {
  uint8_t data[128];
  uint8_t node_status;
  uint8_t recv_chain;
  uint16_t length;
};

enum fifo_status {
  NODE_INVALID = 0,
  NODE_VALID
};

enum ack_type {
  ACK_SUCCESS = 0,
  ACK_SUCCESS_WITH_INDEX, /* Index of success handled packet */
  ACK_FAILED_NOT_COMPLETED, /* When not receiving completed
                                               message slices */
  ACK_FAILED_COMMON_REASON, /* Common fail reasons */
};

enum phy_packet_type {
  MESSAGE_PACKET = 0,
  ACK_PACKET,
  CONTROL_PACKET,
};

struct phy_rx_node_handle phy_rx_node_instance;

void phy_packet_trace(struct phy_packet_handle * packet, uint8_t mode = 0)
{
  if (mode == 0) {
    pr_debug("######## Phy packet trace ########\n");
    pr_debug("type        :%4u\n", packet->type);
    pr_debug("length      :%4u\n", packet->length);
    pr_debug("packet_index:%4u\n", packet->packet_index);
    pr_debug("slice_size  :%4u\n", packet->slice_size);
    pr_debug("slice_index :%4u\n", packet->slice_index);
    pr_debug("crc         :0x%02X\n", packet->crc);
    pr_debug("data        :");
    for (int i = 0; i < packet->length; i ++) {
      if (i % 10 == 0)
        printf("\n");
      printf("0x%02X ", packet->data[i]);
    }  
  }
  else {
    uint8_t *ptr = (uint8_t *)packet;
    pr_info("######## Phy packet trace ########\n");
    for (int i = 0; i < 32; i ++) {
      if (i % 10 == 0)
        printf("\n");
      printf("0x%02X ", ptr[i]);
    }
  }
  printf("\n\n");
}

inline void phy_layer_reset_node(struct phy_rx_node_handle *node)
{
  node->node_status = NODE_INVALID;
  node->recv_chain = 0x00;
}

bool phy_layer_send_slice_packet(struct phy_packet_handle * packet, uint32_t max_retry = 100)
{
  nrf_reliable_send((uint8_t *)packet, 32, max_retry);
}

bool phy_layer_send_ack(ack_type ack, uint8_t * optional_data = NULL,
                              uint8_t length = 0)
{
  uint8_t packet_mem[32];
  struct phy_packet_handle * packet = (struct phy_packet_handle *) packet_mem;
  packet->type = ACK_PACKET;
  packet->length = 1 + length;/* 1byte for ack_type and 
                                  length bytes for optional data */
  packet->data[0] = ack;
  if(length > MAX_PACKET_DATA_SIZE)
    length = MAX_PACKET_DATA_SIZE; /* Limit ack data area max size*/
  memcpy(packet->data + 1, optional_data, length);
  phy_layer_send_slice_packet(packet, 3);
}

void phy_layer_listener(void)
{
  static uint8_t raw_data[32];
  static struct phy_packet_handle * packet = (phy_packet_handle *)raw_data;
  static struct phy_rx_node_handle * phy_rx_node = &phy_rx_node_instance;
  static uint8_t in_receive_state = 0;
  static uint16_t packet_receive_duration = 0;
  static uint16_t packet_max_duration = 0;

  /* Listener shall check if got any rx data from phy */
  if (in_receive_state && packet_receive_duration > packet_max_duration) {
    pr_err("Time out for this message chain reception.\n");
    /* Do something for timeout handle */
  }

  if (phy_layer_data_ready()) {
    uint8_t status = read_register(STATUS);
    uint8_t pipe = (status >> 1) & 0x07; /* Get what pipe channel is
                                         this data from */
    nrf_get_data(raw_data);
    if (crc_calculate((uint8_t *)packet, 2) != packet->crc) {
        pr_err("Recv packet crc check err, raw=0x%02X calc=0x%02X, drop it!\n", packet->crc, crc_calculate((uint8_t *)packet, 2));
        //phy_packet_trace(packet, 1);
        return ;
    }

    if (packet->type == MESSAGE_PACKET) {

      if (packet->slice_index == 0) { /* This is the first slice */
          pr_info("First slice,%u/%u ;Pipe %u, Index %u\n", packet->slice_index, 
            packet->slice_size - 1, pipe, packet->packet_index);
          phy_layer_reset_node(phy_rx_node);
      }
      /* If this is the last slice or timeout , we shall send 
                                              control msg to sender */
      else if (packet->slice_index == packet->slice_size-1) { 
          phy_rx_node->recv_chain |= (1 << (uint8_t)packet->slice_index);
          pr_info("Last slice, %u/%u ;Pipe %u, Index %u\n", packet->slice_index, 
            packet->slice_size - 1, pipe, packet->packet_index);
            /* Check if all slices received and send ack to sender .
             If not all slices received ,we have two ways to handle this 
             situation.One is just drop all slices and send a fail signal to 
             sender , the other one is that we send missed slice indexs to 
             sender and hope it will then sends those slices.          
          */
          uint8_t expect_status = (uint8_t)0xff >> (8 - packet->slice_size);
          //phy_layer_set_tx_addr()
          if (phy_rx_node->recv_chain != expect_status) {
            /* Some slices missed */
            uint8_t missed_slices = phy_rx_node->recv_chain;
            phy_layer_send_ack(ACK_FAILED_NOT_COMPLETED, &missed_slices, 1);
            pr_err("expect_status=0x%02X, missed_slices=0x%02X\n", 
                                              expect_status, missed_slices);
          }
          else {
            /* Make best efforts to send success ack to sender */
            uint8_t packet_index = packet->packet_index;
            phy_layer_send_ack(ACK_SUCCESS_WITH_INDEX, &packet_index, 1);
            phy_rx_node->node_status = NODE_VALID;
            phy_rx_node->length = (packet->slice_size-1) * MAX_PACKET_DATA_SIZE +
                                    packet->slice_size;
            pr_info("Receive all slice packets successfully\n");
          }
          printf("\n");
      }
      /* This is middle slices, 
         check if this node's first slice exists */
      else {
          pr_info("Middle slice , %u/%u ;Pipe %u, Index %u\n", packet->slice_index, 
              packet->slice_size - 1, pipe, packet->packet_index);
      }

      /* Parse recv packet and fetch data into fifo node */
      if (!(phy_rx_node->recv_chain & (1 << (uint8_t)packet->slice_index))) {
        /* This slice doesnt exsits */
        uint16_t offset = packet->slice_index * MAX_PACKET_DATA_SIZE;
        memcpy(phy_rx_node->data + offset, packet->data, packet->length);
        phy_rx_node->recv_chain |= (1 << (uint8_t)packet->slice_index);
      }
    }
    else {
      pr_err("Cant handle this type message : type 0x%02X\n", packet->type);
    }
    
  }
}

/* This function get receive data from local rx fifo.
 * @Param data: to store raw_data
 * @Param max_length: data max length to get,default to 128
 * @Return : data amount get at last 
*/
uint16_t phy_layer_get_data(uint8_t *data, uint16_t max_length = 128)
{
  return 0;
}

void setup()
{
  Serial.begin(500000);
  printf_begin();
 
  printf("Begin config!");
  nrf_gpio_init(8, 9); //Set ce pin and csn pin
  nrf_set_tx_addr((uint8_t *)"mac00");
  nrf_set_rx_addr((uint8_t *)"mac01");
  nrf_set_broadcast_addr('a');
  
  nrf_chip_config(12, 32); // Set channel and payload
  nrf_set_retry_times(5);
  nrf_set_retry_durtion(750);
  nrf_set_channel(100);
  Serial.println("Zigbee network starts!");
  print_info();
  enable_rx();
}

uint32_t comm_rate = 0;
uint32_t comm_sum = 0;
uint32_t last_check_sum = 0;
uint32_t last_check_time = 0;

uint8_t last_seq_num[10] = {0}; /* Care that only 10 node addr allowed! */
uint8_t data[32];
uint32_t print_count = 0;

#define RATE_SAMPLE_TIME 128
#define RATE_SAMPLE_TIME_SHIFT 7

#define MAX_RETRY_TIME 10

volatile uint32_t seq_num = 0;
uint8_t send_id = 0;
uint8_t crc = 0;
uint8_t crc_check = 0;


void loop()
{
  phy_layer_listener();

  /*
  if (phy_rx_node_instance.node_status == NODE_VALID) {
    pr_info("Detect phy_rx_node_instance valid,print data:");
    for (int i = 0; i < phy_rx_node_instance.length; i ++) {
      if (i % 15 == 0)
        printf("\n");
      printf("0x%02X ", phy_rx_node_instance.data[i]);
    }
    printf("\n\n");
    phy_rx_node_instance.node_status = NODE_INVALID;
  }
  */
  //printf("\n");
}


void loop_old()
{
  if (nrf_data_ready()) {
    uint8_t status = read_register(STATUS);
    uint8_t pipe = (status >> 1) & 0x07;
    nrf_get_data(data);
   
    send_id = data[29] < 10 ? data[29] : 0x00;
    seq_num = data[30];
    crc = data[31];
    crc_check = crc_calculate(data, 31);

    
    if (crc == crc_check) {
        printf("Recv pipe:%u msg:[%s]\n", pipe, data);
      
      if ((uint8_t)seq_num == last_seq_num[send_id])
        printf("[%u]Repeat seq num %u!\n", send_id, seq_num);
      else if ((uint8_t)seq_num != ((last_seq_num[send_id] + 1) % 256))
        printf("[%u]Lost frame! cur=%u last=%u\n", send_id, (uint8_t)seq_num, last_seq_num[send_id]);
      last_seq_num[send_id] = seq_num;
      
      comm_sum += 32;
      
    }
    else {
      printf("[%u] [%3u]Wrong crc 0x%02X != 0x%02X\n", send_id, seq_num, data, crc, crc_check);
    }
  
  }
  
}

