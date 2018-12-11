#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDProfile.h"
#include "ZDObject.h"
#include <string.h>
#include "Coordinator.h"
#include "DebugTrace.h"

#if !defined(WIN32)
#include "OnBoard.h"
#endif

#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"
#include "stdio.h"
#include "CO2.h"
#include "Fan.h"

#include "aps_groups.h"
#define  SEND_DATA_EVENT 0X01

const cId_t GenericApp_ClusterList[GENERICAPP_MAX_CLUSTERS] = 
{
  GENERICAPP_CLUSTERID_1,GENERICAPP_CLUSTERID_2
};
const SimpleDescriptionFormat_t GenericApp_SimpleDesc = 
{
  GENERICAPP_ENDPOINT,             //端口号
  GENERICAPP_PROFID,               //应用规范ID
  GENERICAPP_DEVICEID,             //应用设备ID
  GENERICAPP_DEVICE_VERSION,       //应用设备版本号
  GENERICAPP_FLAGS,
  0,                              //输入簇包含的命令个数
  (cId_t *)NULL,                  //输入簇列表
  GENERICAPP_MAX_CLUSTERS,        //输出簇包含的命令个数
  (cId_t *)GenericApp_ClusterList //输出簇列表
};

endPointDesc_t GenericApp_epDesc;
byte GenericApp_TaskID;
byte GenericApp_TransID;
devStates_t GenericApp_NwkState;

void Send_Value(void);
void MSG_Handler(afIncomingMSGPacket_t *pkt);


/*函数功能：协议栈系统功能初始化*/
void GenericApp_Init(byte task_id)
{
  halUARTCfg_t uartConfig;
  
  GenericApp_TaskID       = task_id;   //初始化任务的优先级
  GenericApp_NwkState     = DEV_INIT; //定义该节点为终端节点
  GenericApp_TransID      = 0;       //将发送数据包的序号初始化为0，在ZigBee协议栈中，每发送一个数据包，该发送序号自动加1
  GenericApp_epDesc.endPoint  = GENERICAPP_ENDPOINT;
  GenericApp_epDesc.task_id   = &GenericApp_TaskID;
  GenericApp_epDesc.simpleDesc = (SimpleDescriptionFormat_t *)&GenericApp_SimpleDesc;
  GenericApp_epDesc.latencyReq  = noLatencyReqs; //上述四行是对节点描述符进行初始化，格式较为固定，一般不需要修改
  afRegister(&GenericApp_epDesc);               //使用afRegister函数将节点描述符进行注册，只有注册以后，才可以使用OSAL提供的系统服务；
  
  CO2_init();
  Fan_init();
  HalLedBlink(HAL_LED_2,0,50,250);   //使LED灯2开始闪烁
}
/*函数功能：终端节点状态发生变化，进行组网*/
UINT16 GenericApp_ProcessEvent(byte task_id,UINT16 events)
{
  afIncomingMSGPacket_t *MSGpkt; //定义一个指向接收消息结构体的指针MSGpkt；
  if(events & SYS_EVENT_MSG)
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(GenericApp_TaskID);  //使用osal_msg_receive函数从消息队列上接收消息，该消息中包含了接收到的无线数据包
    while(MSGpkt)
    {
      switch(MSGpkt->hdr.event)
      {
      case ZDO_STATE_CHANGE: //对接收到的消息类型进行判断。终端节点收到ZDO_STATE_CHANGE消息表明成功加入网络
          osal_set_event(GenericApp_TaskID,SEND_DATA_EVENT); //设置一个SEND_DATA_EVENT，用于启动一个定时器
          HalLedSet(HAL_LED_2,HAL_LED_MODE_ON); //终端节点加入网络后，LED灯2常亮
        break;
        
      case AF_INCOMING_MSG_CMD:  //当终端节点收到AF_INCOMING_MSG_CMD消息，表明终端节点接收到其它节点传过来的数据。
        MSG_Handler(MSGpkt);   //对收到的数据进行处理
        break;
          
      default:
        break;
      }
      osal_msg_deallocate((uint8 *)MSGpkt); //接收到的消息处理完后，就需要释放消息所占据的存储空间
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(GenericApp_TaskID);  //处理完一个消息后，再从消息队列里接收消息，然后对其进行相应的处理，直到所有消息都处理完为止
    }
    return (events ^ SYS_EVENT_MSG);
  }
  if(events & SEND_DATA_EVENT)   //判断事件为用户自己定义的事件
  {               
    Send_Value();
    osal_start_timerEx(GenericApp_TaskID,SEND_DATA_EVENT,5000);//启动一个定时器，当5000ms后，会再次产生一个SEND_DATA_EVENT事件
    return(events ^ SEND_DATA_EVENT);
  }
  return 0;
}


void MSG_Handler(afIncomingMSGPacket_t *pkt)
{
  char buf[30] = 0;      //接收数据缓冲区
  switch(pkt->clusterId)
  {
  case GENERICAPP_CLUSTERID_1:  //数据号为GENERICAPP_CLUSTERID_1表明是用户数据
    osal_memcpy(buf,pkt->cmd.Data,pkt->cmd.DataLength); //将收到的从协调器传过来的数据拷贝到buf中
    //HalUARTWrite(0,buf,pkt->cmd.DataLength); //从串口0打印出接收到的数据
    if(buf[0] == 0x01)    //如果收到的数据为0x01,则打开水泵
    {
      Open_Fan();
    }else if(buf[0] == 0x00)  //如果收到的数据为0x00,则关闭水泵
    {
      Close_Fan();
    }
    break;  
  }

}
void Send_Value(void)
{
  float CO2_value; 
  uint16 nwk;
  uint8 buf[128]={0},addr[3]={0};
  afAddrType_t my_DstAddr;      //定义一个afAddrType_t类型的结构体，用于定义发送节点的发送方式，与目标节点的地地址
  CO2_value = GetADC();   //获取二氧化碳传感器数据
  nwk = NLME_GetShortAddr();
  addr[0] = 0x02;
  addr[1] = (uint8)(nwk>>8);
  addr[2] = (uint8)(nwk & 0x00ff);
   
  sprintf(buf,"二氧化碳浓度：%0.2f",CO2_value);
  my_DstAddr.addrMode = (afAddrMode_t)Addr16Bit; //定义发送方式为单播方式；相应的还有广播、组播的方式
  my_DstAddr.endPoint = GENERICAPP_ENDPOINT;    //初始化端口号
  my_DstAddr.addr.shortAddr = 0x0000;          //目标节点短地址。协调器地短地址始终为0x0000
  AF_DataRequest(&my_DstAddr,&GenericApp_epDesc,GENERICAPP_CLUSTERID_1,
                 strlen(buf),buf,&GenericApp_TransID,AF_DISCV_ROUTE,AF_DEFAULT_RADIUS);
  
  AF_DataRequest(&my_DstAddr,&GenericApp_epDesc,GENERICAPP_CLUSTERID_2,
                 3,addr,&GenericApp_TransID,AF_DISCV_ROUTE,AF_DEFAULT_RADIUS);
  
  
  /*AF_DataRequest原形为：
    afStatus_t AF_DataRequest( afAddrType_t *dstAddr, endPointDesc_t *srcEP,
                           uint16 cID, uint16 len, uint8 *buf, uint8 *transID,
                           uint8 options, uint8 radius )
    stAddr为：发送目的地址＋端点地址（端点号）和传送模式
    cID：被Profile指定的有效的集群号；可简单理解为命令ID号
    len:发送数据长度
    buf :发送数据缓冲区
    transID:任务ID号
    */
}



                          