#encoding=utf-8
#author:oxa71a5
#date:2017/11/07
import time
import lxc_nrf24

target_addr = "mac01"
machine = lxc_nrf24.nrf24(my_addr = "mac00",channel = 12)
machine.begin()
#machine.set_target_addr("serv1")
while( machine.available() ): 
        recv = machine.read_str()
        print "Recv:",recv
while True:
    words = raw_input("Enter:")
    if(words == "exit" ):break
    if(words.find(":")==1):
        target_addr = target_addr[:-1]+words[0]
        print "New addr:",target_addr
    machine.send_to(words,target_addr)
    check_point = time.time()
    while( machine.available()==False ):
        if(time.time() - check_point > 0.01):print "Timeout!";break
    while( machine.available() ):
        recv = machine.read_str()
        print "Recv:",recv