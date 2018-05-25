#include <stdio.h>
#include <iostream>
#include <string.h>
#include <wiringPiSPI.h>
#include <unistd.h>
#include <time.h>
#include <wiringPi.h>
#include "nrf24l01.h"
using namespace std;

void rx_test()
{
	char data[32];
	while(1)
	{
		if(dataReady())
		{
			getData(data);
			cout<<"Get packet->"<<data<<endl;
		}
		delay(10);
	}
}

uint8_t toSend[32];
int count = 0 ;
void tx_test()
{
	setTADDR((uint8_t *)"serv1");
	int pac_count=0;
	while(1)
	{
		sprintf(toSend,"%d",pac_count);
		nrf_send(toSend);
		cout<<"Send packet "<<pac_count++<<endl;
		delay(100);
	}
}

void setup(uint8_t *my_addr,int channel)
{
  setChannel(channel);
  nrf_init();
  setRADDR(my_addr);
  setPayloadLength(32);
  nrf_config();
  //printf("Begining!\n");
}


int main()//tx
{
    char data[32];
    setup("clie1",12);
    setTADDR((uint8_t *)"serv1");
    char words[32];

    getData(data);

    while(1)
    {
      cin.getline(words,32);
      nrf_send(words);
      printf("Success sended\n");
      
      long check_point = clock();
      while(!dataReady())
      {
        if(clock() - check_point > 1000)
        {
          printf("break\n");
          break;
        }
      }
      
      while(dataReady())
      {
        getData(data);
        cout<<"Recv:"<<data<<endl;
      }
    } 
    return 0;
}


