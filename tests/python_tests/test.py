import ctypes
import time
so = ctypes.cdll.LoadLibrary
lib = so("./nrf24.so")
my_addr = "clie1"
my_channel = 12
target_addr = "serv1"
lib.nrf24_setup( my_addr , my_channel )
lib.nrf24_tx_addr(target_addr)

while True:
    words = raw_input("Enter:")
    if(words == "exit" ):break
    lib.nrf24_send( words )
    
    check_point = time.time()
    while( lib.nrf24_available() == 0 ):
        if(time.time() - check_point > 0.5):break
    while( lib.nrf24_available() != 0 ):
        recv = "\x00"*32
        lib.nrf24_read(recv)
        recv = recv.split("\x00")[0]
        print "Recv:",recv




