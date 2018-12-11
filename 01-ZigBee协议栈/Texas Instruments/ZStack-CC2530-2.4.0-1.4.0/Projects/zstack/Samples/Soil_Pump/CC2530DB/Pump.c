#include "Pump.h"
#include <ioCC2530.h> 

void Pump_init(void)
{
  P0SEL &= ~0x02;          //P1.0为普通 I/O 口
  P0DIR |= 0x02;           //输出
  
  P0_1 = 0;                //水泵默认关闭
}


void Open_Pump(void)
{
  P0_1 = 1;
}


void Close_Pump(void)
{
  P0_1 = 0;
}