/********************************************************************************
* Copyright (c) 2015，联创中控（北京）科技有限公司
* All rights reserved.
*
* 文件名称：SHT10.h
* 摘	要：
*
* 当前版本：V1.0
* 作	者：张小刚
* 完成日期：2016年3月31日
* 修改摘要：
********************************************************************************/
#ifndef SHT10_H
#define SHT10_H
#include <ioCC2530.h> 
typedef signed   char   int8;
typedef unsigned char   uint8;

typedef signed   short  int16;
typedef unsigned short  uint16;

typedef signed   long   int32;
typedef unsigned long   uint32;


void IIC_GPIOConfig(void);
void SDA_OUT(void);
void SDA_IN(void);

static char WriteByte(unsigned char value);
static char ReadByte(unsigned char ack);
void ConnectionReset(void);
static void TransStart(void);

void SCLtest(void);
void SDAtest(void);
void GH_ReadSoilTemp_Humi(float *pTemp,float *pHumi);

void SHTXX_Init(void);
unsigned char Measure(unsigned char *p_value, unsigned char *p_checkum, unsigned char mode);
void SHTXX_Cal(unsigned short *temp, unsigned short *humi, float *f_temp, float *f_humi );

#endif