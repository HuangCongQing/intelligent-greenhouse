#ifndef COORDINATOR_H
#define COORDINATOR_H

#include "zComDef.h"

#define GENERICAPP_ENDPOINT       10

#define GENERICAPP_PROFID         0X0F04
#define GENERICAPP_DEVICEID       0X0001
#define GENERICAPP_DEVICE_VERSION 0
#define GENERICAPP_FLAGS          0
#define GENERICAPP_MAX_CLUSTERS   2
#define GENERICAPP_CLUSTERID_1      1
#define GENERICAPP_CLUSTERID_2      2

typedef struct RFTXBUF
{
    uint8 type[3];
    uint16 myNWK;
}RFTX;

extern void GenericApp_Init(byte task_id);
extern UINT16 GenericApp_ProcessEvent(byte task_id,UINT16 events);

#endif