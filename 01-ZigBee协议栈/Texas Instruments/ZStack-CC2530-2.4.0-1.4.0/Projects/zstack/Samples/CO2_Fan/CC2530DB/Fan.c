#include "Fan.h"
#include <ioCC2530.h> 

void Fan_init(void)
{
  P0SEL &= ~0x02;          //P1.0为普通 I/O 口
  P0DIR |= 0x02;           //输出
  
  P0_1 = 0;                //风扇默认关闭
}


void Open_Fan(void)
{
  P0_1 = 1;
}


void Close_Fan(void)
{
  P0_1 = 0;
}