/******************************************************************************
* INCLUDES
*/
#include "SHT10.h"


/**************SHT10使用的CC2530端口定义 ********************************/
#define SDA                    P1_6		//温度传感器数据引脚的定义
#define SDAPxSEL               P1SEL
#define SDAPxDIR               P1DIR
#define SDAPORTBIT             0x40

#define SCL                     P1_7		//温度传感器时钟引脚的定义
#define SCLPxSEL                P1SEL
#define SCLPxDIR                P1DIR
#define SCLPORTBIT              0x80


#define Clr_IIC_SCL()   SCL=0
#define Set_IIC_SCL()   SCL=1

#define Set_IIC_SDA()   SDA =1
#define Clr_IIC_SDA()   SDA =0
#define READ_SDA()      SDA
#define WRITE_SDA(x)    SDA=x    


/*********************************************************/
#define noACK 0
#define ACK   1

#define STATUS_REG_W 0x06   //000   0011    0
#define STATUS_REG_R 0x07   //000   0011    1
#define MEASURE_TEMP 0x03   //000   0001    1
#define MEASURE_HUMI 0x05   //000   0010    1



enum {TEMP,HUMI};

void IIC_GPIOConfig(void);
void SDA_OUT(void);
void SDA_IN(void);

static char WriteByte(unsigned char value);
static char ReadByte(unsigned char ack);
void ConnectionReset(void);
static void TransStart(void);

void SCLtest(void);
void SDAtest(void);


/*********************************************************************
* GLOBAL VARIABLES
*/



/************************************************************************
函数名称：IIC_GPIOConfig
函数功能：初始化SDA和SCL端口
函数传参：无
函数返回值：无
函数作者：2016-4 zxg
************************************************************************/
void IIC_GPIOConfig(void)
{  
  SDAPxSEL &= ~SDAPORTBIT;  //DATA口作为通用IO口
  SCLPxSEL &= ~SDAPORTBIT;  //SCK口作为通用IO口
    
  SCLPxDIR  |= SCLPORTBIT;    //SCK设置为输出
  SDAPxDIR  |= SDAPORTBIT;    //DATA设置为输出
  
}


/************************************************************************
函数名称：SDA_IN
函数功能：设置SDA端口为输入口
函数传参：无
函数返回值：无
函数作者：2016-4 zxg
************************************************************************/
void SDA_IN(void)
{
  SDAPxDIR &= ~SDAPORTBIT ;
}

/************************************************************************
函数名称：SDA_OUT
函数功能：设置SDA端口为输出口
函数传参：无
函数返回值：无
函数作者：2016-4 zxg
************************************************************************/
void SDA_OUT(void)
{
  SDAPxDIR |= SDAPORTBIT;
}


/***********************************************************
* 名称: Delay1us()
* 功能: 延时1us函数
* 参数: 输入 需要延时的微秒数
* 返回: 无
* 修改:
* 注释: 
***********************************************************/
static void Delay1us(uint16 t)
{
    uint8 i, j;
    for (j = t; j > 0; j--)
        for (i = 8; i > 0; i--);
}

/***********************************************************
* 名称: SHTXX_Init()
* 功能: 温湿度传感器初始化函数
* 参数: 无
* 返回: 无
* 修改:
* 注释: 
***********************************************************/
void SHTXX_Init(void)
{
   IIC_GPIOConfig();
}


/***********************************************************
* 名称: ConnectionReset()
* 功能: 重置链接
* 参数: 无
* 返回: 无
* 修改:
* 注释: 
***********************************************************/
void ConnectionReset(void)
{
    uint8 i;
    SDA_OUT();
    Set_IIC_SDA();
    Delay1us(10);
    Clr_IIC_SCL();
    Delay1us(10);
    for(i = 0; i < 9; i++){
        Set_IIC_SCL();
        Delay1us(10);
        Clr_IIC_SCL();
        Delay1us(10);
    }
    TransStart();
}

/***********************************************************
* 名称: WriteByte()
* 功能: 写入1字节命令
* 参数: 输入 value 需要写入的命令的数据
* 返回: 无
* 修改:
* 注释: 
***********************************************************/
static char WriteByte(unsigned char value)
{
    unsigned char i, error = 0;
    SDA_OUT();
    for (i=0x80;i>0;i/=2)        //shift bit for masking
    { 
      if (i & value) 
        Set_IIC_SDA();          //masking value with i , write to SENSI-BUS
      else 
        Clr_IIC_SDA();
    Delay1us(10);
    Set_IIC_SCL();                          //clk for SENSI-BUS
    Delay1us(10);                          //pulswith approx. 5 us  	
    Clr_IIC_SCL();
    Delay1us(10);
  }
  SDA_IN();
  Set_IIC_SDA();                           //release DATA-line
  Delay1us(10);
  Set_IIC_SCL();                            //clk #9 for ack 
  Delay1us(10);
  error=READ_SDA();                       //check ack (DATA will be pulled down by SHT11)
  Clr_IIC_SCL();        
  return error;                           //error=1 in case of no acknowledge
}

/***********************************************************
* 名称: ReadByte()
* 功能: 读取1字节数据
* 参数: 输入 ack ack=1表示需要向MMA传输停止位，为0则表示不需要向MMA传输停止位
* 返回: 无
* 修改:
* 注释: 
***********************************************************/
static char ReadByte(unsigned char ack)
{ 
  unsigned char i,val=0;

  Set_IIC_SDA();   //release DATA-line
  Delay1us(10);
  SDA_IN();
  for (i=0x80;i>0;i/=2)             //shift bit for masking
  { 
    Set_IIC_SCL();  //clk for SENSI-BUS
    Delay1us(10);
    if (READ_SDA()) val=(val | i);        //read bit  
    Clr_IIC_SCL();
    Delay1us(10);
  }
  SDA_OUT();
  WRITE_SDA(!ack);                 //in case of "ack==1" pull down DATA-Line
  Delay1us(10);
  Set_IIC_SCL();   //clk #9 for ack
  Delay1us(10);
  Clr_IIC_SCL();
  Delay1us(10);
  Set_IIC_SDA();                           //release DATA-line
  Delay1us(10);
  return val;
}

/***********************************************************
* 名称: TransStart()
* 功能: 开始传送数据
* 参数: 无
* 返回: 无
* 修改:
* 注释: 
***********************************************************/
static void TransStart(void)
{
    SDA_OUT();
    Set_IIC_SDA();
    Delay1us(10);
    Clr_IIC_SCL();
    Delay1us(10);
    Set_IIC_SCL();
    Delay1us(10);
    Clr_IIC_SDA();
    Delay1us(10);
    Clr_IIC_SCL();
    Delay1us(10);
    Set_IIC_SCL();
    Delay1us(10);
    Set_IIC_SDA();
    Delay1us(10);
    Clr_IIC_SCL();
    Delay1us(10);
}

/***********************************************************
* 名称: Measure()
* 功能: 得到测量的值
* 参数: 输出 p_value 输出的数据
        输出 p_checksum 输出的数据校验值
        输入 模式 有温度模式和湿度模式
* 返回: 错误的值 0为无错误
* 修改:
* 注释: 
***********************************************************/
uint8 Measure(unsigned char *p_value, unsigned char *p_checksum, unsigned char mode)
{
    unsigned error = 0;
    //TransStart();
    
   ConnectionReset();

    switch(mode)
    {
        case TEMP: error = WriteByte(MEASURE_TEMP);break;
        case HUMI: error = WriteByte(MEASURE_HUMI);break;
        default: break;
    }
  
    SDA_IN(); 
    error = 0;
     while((error++<67000) && READ_SDA()); //wait until sensor has finished the measurement

    if(READ_SDA())  {
      //error+=1;                // or timeout (~2 sec.) is reached
      return 1;
    }
    *(p_value +1)  =ReadByte(ACK);    //read the first byte (MSB)
    *(p_value)=ReadByte(ACK);    //read the second byte (LSB)
    *p_checksum =ReadByte(noACK);  //read checksum
    return 0;  
}

/***********************************************************
* 名称: SHTXX_Cal()
* 功能: 转化得到的温湿度的值为标准单位额
* 参数: 输入 temp 得到的温度的值
        输入 p_checksum 得到的湿度的值
        输出 f_temp 转化后的温度的值
        输出 f_humi 转化后的温度的值
* 返回: 无
* 修改:
* 注释: 
***********************************************************/
void SHTXX_Cal(uint16 *temp, uint16 *humi, float *f_temp, float *f_humi )
{

  const float C1=-4.0;              // for 12 Bit
  const float C2=+0.0405;           // for 12 Bit
  const float C3=-0.0000028;        // for 12 Bit
  const float T1=+0.01;             // for 14 Bit @ 5V
  const float T2=+0.00008;           // for 14 Bit @ 5V	

  float rh;             // rh:      Humidity [Ticks] 12 Bit 
  float t;           // t:       Temperature [Ticks] 14 Bit
  float rh_lin;                     // rh_lin:  Humidity linear
  float rh_true;                    // rh_true: Temperature compensated humidity
  float t_C;                        // t_C   :  Temperature  

  rh = (float)(*humi);
  t = (float)(*temp);
 
  t_C=t*0.01 - 40;                  //calc. temperature from ticks 
  rh_lin=C3*rh*rh + C2*rh + C1;     //calc. humidity from ticks to [%RH]
  rh_true=(t_C-25)*(T1+T2*rh)+rh_lin;   //calc. temperature compensated humidity [%RH]
  if(rh_true>100)rh_true=100;       //cut if the value is outside of
  if(rh_true<0.1)rh_true=0.1;       //the physical possible range

  *f_temp=t_C;               //return temperature 
  *f_humi=rh_true;              //return humidity[%RH]
  
}



void SDAtest(void)
{
  IIC_GPIOConfig();
  SDA_IN();
  SDA =1;
  Delay1us(1000);
  Delay1us(1000);
   Delay1us(1000);
    Delay1us(1000);

    SDA =0;
  Delay1us(1000);
  Delay1us(1000);
   Delay1us(1000);
    Delay1us(1000);

}


void SCLtest(void)
{
  IIC_GPIOConfig();
  SCL =1;
  Delay1us(1000);
  Delay1us(1000);
  Delay1us(1000);
  Delay1us(1000);

  SCL =0;
  Delay1us(1000);
  Delay1us(1000);
  Delay1us(1000);
  Delay1us(1000);
}

/************************************************************************
*名  称：GH_ReadSoilTemp_Humi
*功  能：获取土壤温湿度数据
*参  数：pTemp：读到的温度值
         pHumi：读到的湿度值
*返回值：无
************************************************************************/
void GH_ReadSoilTemp_Humi(float *pTemp,float *pHumi)
{
   uint16 humi_val=0, temp_val =0; 
   uint8 error,checksum;
   error = 0;
     
   error=Measure((unsigned char*) &temp_val,&checksum,0);  //measure temperature
   error+=Measure((unsigned char*) &humi_val,&checksum,/*HUMI*/1);  //measure humidity
  
   if (error == 0) 
    {
      SHTXX_Cal(&temp_val, &humi_val, pTemp, pHumi);
    }
}