/******************************************************************************
* INCLUDES
*/
#include "CO2.h"

void CO2_init(void)
{
  P0SEL |= 0x01;
  ADCCON3  = (0xB0);                    //选择AVDD5为参考电压；12分辨率；P0_0  ADC
  
  ADCCON1 |= 0x30;                      //选择ADC的启动模式为手动
}

float GetADC(void)
{
  float ret = 0.0f;
  unsigned int  value;
  P0SEL |= 0x01;
  ADCCON3  = (0xB0);                    //选择AVDD5为参考电压；12分辨率；P0_0  ADC
  
  ADCCON1 |= 0x30;                      //选择ADC的启动模式为手动
  ADCCON1 |= 0x40;                      //启动AD转化             
  
  while(!(ADCCON1 & 0x80));             //等待ADC转化结束
  
  value =  ADCL >> 2;
  value |= (ADCH << 6);                 //取得最终转化结果，存入value中
  ret = (float)(value / 4.0);
  return ret;  
}

