#include <stdio.h>
#include <iostream>
#include <string.h>
#include <wiringPiSPI.h>
#include <unistd.h>
#include <wiringPi.h>
using namespace std;
#include <nrf24l01.h>

uint8_t PTX=0;
uint8_t channel=0;
uint8_t payload=32;

#define debug_printf(...)
//#define debug_printf(...) printf( __VA_ARGS__)

void nrf_init() 
{   
    wiringPiSetup();
    wiringPiSPISetup(SPI_CHANNEL,500000);//Initialize spi
    pinMode(CE_PIN,OUTPUT);
    ceLow();
}


void nrf_config() 
{
    configRegister(RF_CH,channel);
    // Set length of incoming payload 
    configRegister(RX_PW_P0, payload);
    configRegister(RX_PW_P1, payload);
    configRegister(RX_PW_P2, payload);
    //configRegister(EN_AA, 0x00);//disable shockburst mode:auto ack
    configRegister(EN_AA, 0x00);/* Enable pipe0 and pipe1 auto ack*/
    configRegister(EN_RXADDR, 0x03);/* Enable pipe0 1 2 rx */

    // Start receiver 
    powerUpRx();
    flushRx();
}

void nrf_set_broadcast_addr(uint8_t addr)
{
  configRegister(RX_ADDR_P2, addr);
}

void nrf_set_retry_times(uint8_t max_retry_times)
{
    uint8_t rety_reg = 0x00;

    readRegister(SETUP_RETR, &rety_reg, 1);
    rety_reg = (rety_reg & 0xf0) | (0x0f & max_retry_times);
    configRegister(SETUP_RETR, rety_reg);
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

    readRegister(SETUP_RETR, &rety_reg, 1);
    rety_reg = (rety_reg & 0x0f) | ((uint8_t)micro_senconds << 4);
    configRegister(SETUP_RETR, rety_reg);
}

void setRADDR(uint8_t * addr) // Sets the receiving address
{
    uint8_t reverse_addr[5];
    debug_printf("setRADDR=[0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]\n", 
        addr[0], addr[1], addr[2], addr[3], addr[4]);
    /* RX_ADDR_P0 must be set to the sending addr for auto ack to work. */
    for (int i = 0; i < 5; i ++) {
        reverse_addr[i] = addr[5-i-1];
    }

    ceLow();
    writeRegister(RX_ADDR_P1, reverse_addr, 5);
    ceHi();
}

void setTADDR(uint8_t * addr) // Sets the transmitting address
{
    uint8_t reverse_addr[5];
    /* RX_ADDR_P0 must be set to the sending addr for auto ack to work. */
    for (uint8_t i = 0; i < 5; i ++) {
        reverse_addr[i] = addr[5-i-1];
    }
    writeRegister(RX_ADDR_P0, reverse_addr, 5);
    writeRegister(TX_ADDR, reverse_addr, 5);
    debug_printf("setTADDR=[%c %c %c %c %c]\n",
        addr[0], addr[1], addr[2], addr[3], addr[4]);
}

bool dataReady() // Checks if data is available for reading
{
    uint8_t status = getStatus();
    if ( status & (1 << RX_DR) ) return 1;
    return !rxFifoEmpty();
}

bool rxFifoEmpty(){
    uint8_t fifoStatus;
    readRegister(FIFO_STATUS,&fifoStatus,sizeof(fifoStatus));
    return (fifoStatus & (1 << RX_EMPTY));
}

void getData(uint8_t * data) // Reads payload bytes into data array
{
    uint8_t *temp_buffer = (uint8_t *)malloc(payload+1);
    memset(temp_buffer,0,payload+1);
    temp_buffer[0] = R_RX_PAYLOAD;
    wiringPiSPIDataRW(SPI_CHANNEL,temp_buffer,payload+1);
    memcpy(data,temp_buffer+1,payload);
    configRegister(STATUS,(1<<RX_DR));   // Reset status register
    free(temp_buffer);
    debug_printf("getData=[%s]\n", data);
}

void configRegister(uint8_t reg, uint8_t value)
{
    uint8_t temp_buffer[2];
    temp_buffer[0] = (W_REGISTER | (REGISTER_MASK & reg));
    temp_buffer[1] = value;
    wiringPiSPIDataRW(SPI_CHANNEL,temp_buffer,2);
}

void readRegister(uint8_t reg, uint8_t * value, uint8_t len)// Reads an array of bytes from the given start position in the MiRF registers.
{
    uint8_t *temp_buffer = (uint8_t *)malloc(len+1);
    memcpy(temp_buffer+1,value,len);
    temp_buffer[0] = (R_REGISTER | (REGISTER_MASK & reg));
    wiringPiSPIDataRW(SPI_CHANNEL,temp_buffer,len+1);
    memcpy(value,temp_buffer+1,len);
    free(temp_buffer);
}

void writeRegister(uint8_t reg, uint8_t * value, uint8_t len) // Writes an array of bytes into inte the MiRF registers.
{
    uint8_t *temp_buffer = (uint8_t *)malloc(len+1);
    temp_buffer[0] = (W_REGISTER | (REGISTER_MASK & reg));
    memcpy(temp_buffer+1,value,len);
    wiringPiSPIDataRW(SPI_CHANNEL,temp_buffer,len+1);
    free(temp_buffer);
}

void nrf_send(uint8_t * value) // Sends a data package to the default address. Be sure to send the correct
{
// amount of bytes as configured as payload on the receiver.
    uint8_t status;
    status = getStatus();
    while (PTX) {
            status = getStatus();

            if((status & ((1 << TX_DS)  | (1 << MAX_RT)))){
                    PTX = 0;
                    break;
            }
    }                  // Wait until last paket is send
    ceLow();
    powerUpTx();       // Set to transmitter mode , Power up

    uint8_t temp = FLUSH_TX;
    wiringPiSPIDataRW(SPI_CHANNEL,&temp,1);

    uint8_t *temp_buffer = (uint8_t *) malloc(payload+1);
    temp_buffer[0] = W_TX_PAYLOAD;
    memcpy ( temp_buffer+1 , value , payload);
    wiringPiSPIDataRW(SPI_CHANNEL,temp_buffer,payload+1);

    ceHi();                     // Start transmission
    while(isSending());
    debug_printf("nrf_send payload=[%s]\n", value);
}
bool isSending(){
        uint8_t status;
        if(PTX){
                status = getStatus();
                /*
                 *  if sending successful (TX_DS) or max retries exceded (MAX_RT).
                 */
                if((status & ((1 << TX_DS)  | (1 << MAX_RT)))){
                        powerUpRx();
                        return false; 
                }
                return true;
        }
        return false;
}

uint8_t getStatus(){
        uint8_t rv;
        readRegister(STATUS,&rv,1);
        return rv;
}

void powerUpRx(){
        PTX = 0;
        ceLow();
        configRegister(CONFIG, mirf_CONFIG | ( (1<<PWR_UP) | (1<<PRIM_RX) ) );
        ceHi();
        configRegister(STATUS,(1 << TX_DS) | (1 << MAX_RT)); 
}

void flushRx(){
    uint8_t temp = FLUSH_RX;
    wiringPiSPIDataRW(SPI_CHANNEL,&temp,1);
}

void powerUpTx(){
        PTX = 1;
        configRegister(CONFIG, mirf_CONFIG | ( (1<<PWR_UP) | (0<<PRIM_RX) ) );
}

void ceHi(){
        digitalWrite(CE_PIN,1);
}

void ceLow(){
        digitalWrite(CE_PIN,0);
}

void powerDown(){
        ceLow();
        configRegister(CONFIG, mirf_CONFIG );
}


void setChannel(int channel_)
{
    channel = channel_;
    configRegister(RF_CH,channel);
}
void setPayloadLength(int length)
{
    payload = length;
    configRegister(RX_PW_P0, payload);
    configRegister(RX_PW_P1, payload);
}


extern "C"{
  void nrf24_spi_init(void)
  {
    nrf_init();
  }

  void nrf24_setup(uint8_t *my_addr,int channel)
  {
    setChannel(channel);
    nrf_init();
    setRADDR(my_addr);
    setPayloadLength(32);
    nrf_config();
  }

  void nrf24_tx_addr(uint8_t *target_addr)
  {
    setTADDR(target_addr);

  }

  void nrf24_send(uint8_t *data)
  {
    nrf_send(data);
  }

  uint8_t nrf24_available()
  {
    return dataReady();
  }

  void nrf24_read(uint8_t *data)
  {
      getData(data);
  }

  void nrf_test(uint8_t *data)
  {
    uint8_t *t = "hello";
    memcpy(data,t,strlen(t)+1);
  }

  void read_status(void)
  {
    return getStatus();
  }
}
