#encoding=utf-8
#author:oxa71a5
#date:2017/11/07
#description: A python communication library (with c shared library included) for nrf24l01+ 2.4GHZ wireless chip. It can work on raspberry pi.
import ctypes
import time

class nrf24:
    def __init__ (self):
        self.my_addr = "clie1" #相当于是本机的接收地址
        self.channel = 12 #通信频道
        #self.recv_payload = "\x00"*32 #接受缓冲区
        so = ctypes.cdll.LoadLibrary
        self.clib = so("cpp_src/nrf24.so") #这个so文件是树莓派3上的c共享库 其他操作系统需要另外编译

    def __init__(self,my_addr,channel):
        self.my_addr = my_addr
        self.channel = channel
        #self.recv_payload = "\x00"*32
        so = ctypes.cdll.LoadLibrary
        self.clib = so("cpp_src/nrf24.so")

    def begin(self): #开启设定
        try:
            self.clib.nrf24_setup(self.my_addr,self.channel)
            return True
        except Exception as e:
            print e
            return False #出错就返回False

    def set_my_addr(self,my_addr): #设定我的接收地址
        self.my_addr = my_addr
        self.begin()

    def set_target_addr(self,target_addr): #设定对方的接收地址
        self.clib.nrf24_tx_addr(target_addr)

    def send(self,message): #发送给对方
        self.clib.nrf24_send(message)

    def send_to(self,message,target_addr): #指定发送地址发送给对方
        self.set_target_addr(target_addr)
        self.send(message)

    def available(self): #检测接受缓冲区是否有数据 如果有数据返回True
        if( self.clib.nrf24_available() != 0 ):
            return True
        return False

    def read(self): #读取接受缓冲区的数据
        recv_payload = "\x00"*32
        self.clib.nrf24_read(recv_payload)
        return recv_payload

    def read_str(self): #按照\x00作为结束符
        recv_payload = "\x00"*32
        self.clib.nrf24_read(recv_payload)
        recv_payload = recv_payload.split("\x00")[0]
        return recv_payload







