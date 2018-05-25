#include <stdio.h>
#include <iostream>
#include <string.h>
#include <wiringPiSPI.h>
#include <unistd.h>
#include <time.h>
#include <wiringPi.h>
#include <nrf24l01.h>
#include <stdlib.h>

using namespace std;

int main(int argc, char *argv[])
{
    unsigned char reg_addr = 0;
    unsigned char value = 0;
    if(argc < 2) {
        printf("regtool \nTo read register : ./regtool read_addr \n");
        printf("To write register : ./regtool write_addr write_value\n");
        exit(0);
    }
    nrf_init();
    if(argc == 2) {
        reg_addr = strtol(argv[1], NULL, 16);
        readRegister(reg_addr, &value, 1);
        printf("0x%02X\n", value);
        exit(0);
    }

    if(argc == 3) {
        reg_addr = strtol(argv[1], NULL, 16);
        value = strtol(argv[2], NULL, 16);
        writeRegister(reg_addr, &value, 1);
        exit(0);
    }
    return 0;
}
