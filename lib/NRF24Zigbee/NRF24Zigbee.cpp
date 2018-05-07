#include "NRF24Zigbee.h"


static uint8_t group_addr[GROUP_ADDR_LENGTH] = {'m', 'a', 'c', '0'};
static uint8_t mac_addr[MAC_ADDR_LENGTH];
static uint8_t net_addr[NET_ADDR_LENGTH];

static uint8_t transfer_mode = 0;
static uint8_t ce_pin = 13;
static uint8_t csn_pin = 9;
static uint8_t channel = 0;
static uint8_t payload = 32;



uint8_t spi_transfer(uint8_t data)
{
    return SPI.transfer(data);
}

/* This is a new test function for spi transfer */
void spi_transfer_exchange(uint8_t *dataout,uint8_t *datain,uint8_t len){
        uint8_t i;
        for (i = 0;i < len;i++){
                datain[i] = spi_transfer(dataout[i]);
        }
}

/* This is a new test function for spi transfer */
void spi_transfer_noexchange(uint8_t *dataout,uint8_t len){
        uint8_t i;
        for (i = 0;i < len;i++){
            spi_transfer(dataout[i]);
        }
}

void read_reg(uint8_t reg, uint8_t * value, uint8_t len)
{
    csn_low();
    spi_transfer(R_REGISTER | (REGISTER_MASK & reg));
    spi_transfer_exchange(value,value,len);
    csn_high();
}

void write_reg(uint8_t reg, uint8_t * value, uint8_t len)
{
    csn_low();
    spi_transfer(W_REGISTER | (REGISTER_MASK & reg));
    spi_transfer_noexchange(value,len);
    csn_high();
}

void config_register(uint8_t reg, uint8_t value)
{
    write_reg(reg, &value, 1);
}

uint8_t read_register(uint8_t reg)
{
    uint8_t value = 0x00;
    read_reg(reg, &value, 1);
    return value;
}

void nrf_gpio_init(uint8_t ce_pin_num, uint8_t csn_pin_num) 
{
    ce_pin = ce_pin_num;
    csn_pin = csn_pin_num;
    pinMode(ce_pin,OUTPUT);
    pinMode(csn_pin,OUTPUT);
    ce_low();
    csn_high();
    SPI.begin();
}


void nrf_chip_config(uint8_t channel_num, uint8_t payload_num)
{
    channel = channel_num;
    payload = payload_num;
    config_register(RF_CH,channel);
    /* Set length of incoming payload */
    config_register(RX_PW_P0, payload);
    config_register(RX_PW_P1, payload);
    config_register(RX_PW_P2, payload);
    //config_register(EN_AA, 0x00);/* Disable shockburst mode:auto ack */
    config_register(EN_AA, 0x07);/* Enable pipe0 and pipe1 auto ack*/
    config_register(EN_RXADDR, 0x07);/* Enable pipe0 1 2 rx */

    /* Start receiver */
    enable_rx();
    flush_rx();
}

void nrf_set_retry_times(uint8_t max_retry_times)
{
    uint8_t rety_reg = 0x00;

    read_reg(SETUP_RETR, &rety_reg, 1);
    rety_reg = (rety_reg & 0xf0) | (0x0f & max_retry_times);
    config_register(SETUP_RETR, rety_reg);
}

void nrf_set_retry_durtion(uint32_t micro_senconds)
{
    uint8_t rety_reg = 0x00;

    if (micro_senconds > 4000)
        micro_senconds = 4000;
    else if (micro_senconds == 0)
        micro_senconds = 1;

    micro_senconds --;
    micro_senconds /= 250;

    read_reg(SETUP_RETR, &rety_reg, 1);
    rety_reg = (rety_reg & 0x0f) | ((uint8_t)micro_senconds << 4);
    config_register(SETUP_RETR, rety_reg);
}

void nrf_set_channel(uint8_t channel_num)
{
    channel_num &= 0x7f;
    config_register(RF_CH, channel_num);
}

uint8_t nrf_carrier_detect(void)
{
  uint8_t flush_data[32];
  enable_rx();
  delayMicroseconds(180);
  uint8_t regval = 0xff;
  read_reg(CD, &regval, 1);
  if (nrf_data_ready())
    nrf_get_data(flush_data);
  return regval;
}

void nrf_set_group_addr(uint8_t *group_addr_ptr)
{
    memcpy(group_addr, group_addr_ptr, GROUP_ADDR_LENGTH);
    nrf_set_net_addr();
}

void nrf_set_mac_addr(uint8_t *mac_addr_num)
{
    memcpy(mac_addr, mac_addr_num, MAC_ADDR_LENGTH);
    nrf_set_net_addr();
}

void nrf_set_net_addr()
{
    memcpy(net_addr, group_addr, GROUP_ADDR_LENGTH);
    memcpy(net_addr + GROUP_ADDR_LENGTH, mac_addr, MAC_ADDR_LENGTH);
    nrf_set_rx_addr(net_addr);
}

void nrf_set_rx_addr(uint8_t * addr) /* Sets the receiving address */
{
    uint8_t reverse_addr[NET_ADDR_LENGTH];

    /* RX_ADDR_P0 must be set to the sending addr for auto ack to work. */
    for (int i = 0; i < NET_ADDR_LENGTH; i ++) {
        reverse_addr[i] = addr[NET_ADDR_LENGTH-i-1];
    }

    ce_low();
    write_reg(RX_ADDR_P1, reverse_addr, NET_ADDR_LENGTH);
    ce_high();
}

void nrf_set_tx_addr(uint8_t * addr) /* Sets the transmitting address */
{
    uint8_t reverse_addr[NET_ADDR_LENGTH];

    /* RX_ADDR_P0 must be set to the sending addr for auto ack to work. */
    for (int i = 0; i < NET_ADDR_LENGTH; i ++) {
        reverse_addr[i] = addr[NET_ADDR_LENGTH-i-1];
    }
    write_reg(RX_ADDR_P0, reverse_addr, NET_ADDR_LENGTH);
    write_reg(TX_ADDR, reverse_addr, NET_ADDR_LENGTH);
}

bool nrf_data_ready() /* Checks if data is available for reading */
{
    uint8_t status = nrf_get_status();
    if (status & (1 << RX_DR)) return 1;
    return !rx_fifo_empty();
}

bool rx_fifo_empty(){
        uint8_t fifoStatus;
        read_reg(FIFO_STATUS,&fifoStatus,sizeof(fifoStatus));
        return (fifoStatus & (1 << RX_EMPTY));
}

void nrf_get_data(uint8_t * data) /* Reads payload bytes into data array */
{
    csn_low();
    spi_transfer( R_RX_PAYLOAD );
    spi_transfer_exchange(data,data,payload);
    csn_high();
    config_register(STATUS,(1<<RX_DR)); /* Reset status register */
}



uint8_t nrf_send(uint8_t * value)
{
    uint8_t status;
    volatile uint32_t loop_times = 0;
    uint32_t max_loop_time = 1000;
    status = nrf_get_status();
    while (transfer_mode) {
            status = nrf_get_status();
            if((status & ((1 << TX_DS)  | (1 << MAX_RT)))){
                    transfer_mode = 0;
                    break;
            }
    }                  /* Wait until last paket is send */
    ce_low();
    enable_tx();       /* Set to transmitter mode , power up */
    csn_low();                    /* Pull down chip select */
    spi_transfer(FLUSH_TX);
    csn_high();                    /* Pull up chip select */
    csn_low();                    /* Pull down chip select */
    spi_transfer( W_TX_PAYLOAD );
    spi_transfer_noexchange(value,payload);   /* Write payload */
    csn_high();
    ce_high();
    while ((status = get_sending_status()) == TX_SENDING) {
        if (loop_times ++ > max_loop_time )
            return TX_TIMEOUT;
    }
    enable_rx();
    return status;
}

uint8_t get_sending_status(void)
{
        uint8_t status;
        if (transfer_mode){
                status = nrf_get_status();
                if (status & (1 << TX_DS))
                    return TX_REACH_DST;
                if (status & (1 << MAX_RT))
                    return TX_TIMEOUT;
                return TX_SENDING;
        }
        return TX_REACH_DST;
}

uint8_t nrf_get_status(){
        uint8_t rv;
        read_reg(STATUS,&rv,1);
        return rv;
}

void enable_rx(){
        transfer_mode = 0;
        ce_low();
        config_register(CONFIG, CRC_CONFIG | ( (1<<PWR_UP) | (1<<PRIM_RX) ) );
        ce_high();
        config_register(STATUS,(1 << TX_DS) | (1 << MAX_RT)); 
}

void flush_rx(){
    csn_low();
    spi_transfer(FLUSH_RX);
    csn_high();
}

void enable_tx(){
        transfer_mode = 1;
        config_register(CONFIG, CRC_CONFIG | ( (1<<PWR_UP) | (0<<PRIM_RX) ) );
}

void ce_high(){
        digitalWrite(ce_pin,HIGH);
}

void ce_low(){
        digitalWrite(ce_pin,LOW);
}

void csn_high(){
        digitalWrite(csn_pin,HIGH);
}

void csn_low(){
        digitalWrite(csn_pin,LOW);
}

void power_down(){
        ce_low();
        config_register(CONFIG, CRC_CONFIG );
}

uint8_t crc_calculate(uint8_t *data_area, uint32_t length)
{
    uint8_t result = 0x00;
    uint32_t i = 0;
    for (i = 0; i < length; i ++) {
        result ^= data_area[i];
    }
    return result;
}


void nrf_retreat()
{
  uint16_t retreat_time = 1;
  uint16_t max_retreat_time = 20000; /* 20ms */
  uint32_t max_block_time = 100000; /* We can only blocked within this time*/
  uint32_t check_time = millis();
  
  while (nrf_carrier_detect()) {
    delayMicroseconds(retreat_time + random(retreat_time>>2));
    if (retreat_time < max_retreat_time)
      retreat_time <<= 1;
    if (millis() - check_time > max_block_time)
      break; /* Consider of bad communication condition,we may just return failed */
  }
}

/* 
 * If reliable send success then return true ,otherwise return false 
 * Reliable send shall consider complex wireless environment
 * and retreat when neccessary.
*/
bool nrf_reliable_send(uint8_t *data, uint32_t length = 32, uint32_t max_fail_time = 100)
{
  uint8_t tx_status = 0x00;
  uint8_t failed_times = 0;
  
  nrf_retreat();
  do {
        tx_status = nrf_send(data);
        failed_times ++;
  }
  while (tx_status != TX_REACH_DST && failed_times < max_fail_time);
  return tx_status == TX_REACH_DST;
}

void nrf_broad(uint8_t *data, uint32_t length = 32)
{
  nrf_retreat();
  nrf_send(data);
}