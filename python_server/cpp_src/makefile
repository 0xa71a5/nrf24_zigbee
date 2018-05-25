main: nrf24l01.o testTx.cpp
	g++ nrf24l01.o testTx.cpp -o main -I. -lwiringPi -fpermissive -w

nrf24l01.o: nrf24l01.cpp nrf24l01.h
	g++ -c nrf24l01.cpp -I. -lwiringPi -fpermissive -w

clean:
	rm *.o

