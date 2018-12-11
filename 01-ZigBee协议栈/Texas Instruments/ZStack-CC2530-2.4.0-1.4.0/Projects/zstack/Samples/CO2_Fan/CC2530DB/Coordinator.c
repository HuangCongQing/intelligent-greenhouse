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
#include "OSAL_Nv.h"
#include "aps_groups.h"

#define SEND_TO_ALL_EVENT 0X01

const cId_t GenericApp_ClusterList[GENERICAPP_MAX_CLUSTERS]  = 
{
  GENERICAPP_CLUSTERID_1,GENERICAPP_CLUSTERID_2
};

const SimpleDescriptionFormat_t GenericApp_SimpleDesc = {
  GENERICAPP_ENDPOINT,                      //端串口号
  GENERICAPP_PROFID,                        //应用规范ID
  GENERICAPP_DEVICEID,                      //应用设备ID
  GENERICAPP_DEVICE_VERSION,                //应用设备版本号
  GENERICAPP_FLAGS,                          
  GENERICAPP_MAX_CLUSTERS,                   //输入簇包含的命令个数
  (cId_t *)GenericApp_ClusterList,           //输入簇列表
  0,                                         //输出簇包含的命令个数
  (cId_t *)NULL                              //输出簇列表
};

endPointDesc_t GenericApp_epDesc;
devStates_t GenericApp_NwkState;
byte GenericApp_TaskID;
byte GenericApp_TransID;
uint8 nodenum = 0;
uint16 addr[16];


void GenericApp_MessageMSGCB(afIncomingMSGPacket_t *pckt);
void GenericApp_SendTheMessage(void);
static void rxCB(uint8 port,uint8 event);
/*函数功能：协调器协议栈相关功能初始化*/
void GenericApp_Init(byte task_id)
{
  halUARTCfg_t uartConfig;
  GenericApp_TaskID       = task_id;                     //初始化任务的优先级
  GenericApp_TransID      = 0;                           //将发送数据包的序号初始化为0，在ZigBee协议栈中，每发送一个数据包，该发送序号自动加1
  GenericApp_epDesc.endPoint = GENERICAPP_ENDPOINT;    
  GenericApp_epDesc.task_id = &GenericApp_TaskID;
  GenericApp_epDesc.simpleDesc = (SimpleDescriptionFormat_t *)&GenericApp_SimpleDesc;
  GenericApp_epDesc.latencyReq = noLatencyReqs;          //上述四行是对节点描述符进行初始化，格式较为固定，一般不需要修改
  afRegister(&GenericApp_epDesc);                          //使用afRegister函数将节点描述符进行注册，只有注册以后，才可以使用OSAL提供的系统服务；
  
  uartConfig.configured     = TRUE;                      //确定对串口参数进行配置
  uartConfig.baudRate       = HAL_UART_BR_115200;        //串口波特率为115200
  uartConfig.flowControl    = FALSE;                     //没有流控制
  uartConfig.callBackFunc   = rxCB;                      //串口回调函数
  HalUARTOpen(0,&uartConfig);                            //根据uartConfig结构体变量中的成员，初始化串口0
  
  HalLedBlink(HAL_LED_2,0,50,250);                       //使LED灯2开始闪烁
}
/*函数功能：协调器建立网络，并启动事件处理进程
*/
UINT16 GenericApp_ProcessEvent(byte task_id,UINT16 events)
{
 
  afIncomingMSGPacket_t *MSGpkt;                          //定义一个指向接收消息结构体的指针MSGpkt；
  if(events & SYS_EVENT_MSG)
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(GenericApp_TaskID);  ////使用osal_msg_receive函数从消息队列上接收消息，该消息中包含了接收到的无线数据包
    while(MSGpkt)
    {
      switch(MSGpkt->hdr.event)
      {
       case ZDO_STATE_CHANGE:                           //对接收到的消息类型进行判断。协调器收到ZDO_STATE_CHANGE消息表明组网成功
          HalLedSet(HAL_LED_2,HAL_LED_MODE_ON);         //协调器组网成功后，LED灯2常亮         
        break;
      case AF_INCOMING_MSG_CMD:                         //当协调器收到AF_INCOMING_MSG_CMD消息，表明协调器收到其它节点传过来的数据。
          GenericApp_MessageMSGCB(MSGpkt);              //对收到的数据进行处理
        break;
      default:
        break;
      }
      osal_msg_deallocate((uint8 *)MSGpkt);             //接收到的消息处理完后，就需要释放消息所占据的存储空间
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(GenericApp_TaskID);  //处理完一个消息后，再从消息队列里接收消息，然后对其进行相应的处理，直到所有消息都处理完为止
    }
    return (events ^ SYS_EVENT_MSG);
  }
  return 0;
}
/*函数功能：协调器收到节终端点发过来的数据并从串口0打印出来，
 协调器保存由终端节点上传的自身短地址
*/
void GenericApp_MessageMSGCB(afIncomingMSGPacket_t *pkt)
{
  char buf[128] = 0;                          //接收数据缓冲区
  char i;
  switch(pkt->clusterId)
  {
  case GENERICAPP_CLUSTERID_1:              //数据号为GENERICAPP_CLUSTERID_2表明是用户数据
    osal_memcpy(buf,pkt->cmd.Data,pkt->cmd.DataLength);  //将收到的从终端节点传过来的数据拷贝到buf中
    HalUARTWrite(0,buf,pkt->cmd.DataLength);     //从串口0打印出接收到的数据 
    break;
  case GENERICAPP_CLUSTERID_2:                  //节点自身地址信息的数据号为GENERICAPP_CLUSTERID_1
    osal_memcpy(buf,pkt->cmd.Data,pkt->cmd.DataLength);
    i = buf[0];
    addr[i] = buf[1] *256 + buf[2];
    break;
  
  }
}
/*函数功能：用于协调器读取串口数据并将数据发送出去*/
static void rxCB(uint8 port,uint8 event)
{
    uint8 buf[128];
    uint8 len = 0,i=0;
    afAddrType_t my_DstAddr;               //定义一个afAddrType_t类型的结构体，用于定义发送节点的发送方式，与目标节点的地地址
    len = HalUARTRead(0,buf,128);           //读取串口数据，并将数据存储在buf缓冲区中
    if(len != 0)
    {  
      osal_memcpy(&i,buf,1);
      my_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;  //定义发送方式为单播方式；相应的还有广播、组播的方式
      my_DstAddr.endPoint = GENERICAPP_ENDPOINT;  //初始化端口号
      my_DstAddr.addr.shortAddr = addr[i];        //目标节点短地址
      AF_DataRequest(&my_DstAddr,&GenericApp_epDesc,GENERICAPP_CLUSTERID_1,
                   len-1,buf+1,&GenericApp_TransID,AF_DISCV_ROUTE,AF_DEFAULT_RADIUS);

    }
}


