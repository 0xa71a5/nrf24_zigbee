#encoding=utf8
class AirconditionController: #空调控制器
    def __init__(self,IOT_Center,machineId):
        self.IOT = IOT_Center #获取通信接口
        self.machineId = machineId #获取本控制器的通信地址

    def getOnlineStatus(self): #获取控制器的状态
        result,data = self.IOT.communicateToNode(self.machineId,"get","status")
        return data["status"] if(result) else "offline" #如果获取到通信结果  那么返回当前状态

    def turnOnAc(self): #打开空调
        result,data = self.IOT.communicateToNode(self.machineId,"turnOnAc","none")
        return data["turnOnAcResult"] if(result) else "" 

    def turnOffAc(self): #关闭空调
        result,data = self.IOT.communicateToNode(self.machineId,"turnOffAc","none")
        return data["turnOffAcResult"] if(result) else ""  

    def setAcTemperature(self,temperature): #设定空调温度
        temperature = int(temperature)
        if(temperature<0): temperature = 0
        if(temperature>30): temperature = 30
        result,data = self.IOT.communicateToNode(self.machineId,"changeAcTemp",str(temperature))
        return data["changeAcTempResult"] if(result) else ""  