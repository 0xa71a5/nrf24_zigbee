#encoding=utf8
class Router:
    def __init__(self,IOT_Center,machineId):
        self.IOT = IOT_Center #获取通信接口
        self.machineId = machineId #获取本控制器的通信地址

    def getOnlineStatus(self): #获取控制器的状态
        result,data = self.IOT.communicateToNode(self.machineId, "get", "status")
        return data["status"] if(result) else "offline" #如果获取到通信结果  那么返回当前状态