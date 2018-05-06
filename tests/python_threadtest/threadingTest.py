#encoding=utf-8
#author:oxa71a5-lxc

import threading
import time
import signal

def exitHandler(signalnum,frame):
    global A
    print "\nWait a moment for me to exit"
    A.stop()
    print signalnum

    #exit()

class testThread(threading.Thread):
    def __init__(self):
        self.__stop = False
        super(testThread,self).__init__()
    
    def run(self):
        f=open("test.txt","w")
        while True:
            print time.time()
            f.write(str(time.time())+"\n")
            time.sleep(1)
            if(self.__stop):
                self.__stop = False
                print "exit 1"
                break
    def stop(self):
        self.__stop = True
        while(self.__stop == True):
            pass
        print "exit2"

import Queue
a={}
key = 123
value = 223
try:
    a[key].put(value)
except KeyError:
    a[key] = Queue.Queue()
    a[key].put(value)
print a[key].queue 
