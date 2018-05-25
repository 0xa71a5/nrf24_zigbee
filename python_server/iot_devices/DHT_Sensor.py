#encoding=utf-8
class DHT_Sensor: #温湿度传感器
    def __init__(self,IOT_Center,machineId):
        self.IOT = IOT_Center #获取通信接口
        self.machineId = machineId #获取本传感器的通信地址

    def getOnlineStatus(self): #获取传感器的在线状态
        result,data = self.IOT.communicateToNode(self.machineId,"get","status")
        return data["status"] if(result) else "offline" #如果获取到通信结果  那么返回当前状态

    def getHumidity(self): #获取传感器的湿度数值
        try:
            result,data = self.IOT.communicateToNode(self.machineId,"get","humidity")
            return data["humidity"] if(result) else ""   
        except:
            print("err@getTemperature, recv result={} data=[{}]".format(result, data))
            return ""

    def getTemperature(self):
        result,data = self.IOT.communicateToNode(self.machineId,"get","temperature")
        try:
            return data["temperature"] if(result) else ""
        except:
            print("err@getTemperature, recv result={} data=[{}]".format(result, data))
            return ""
