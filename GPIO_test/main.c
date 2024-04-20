/*
 * main.c
 */
#include "DSP28x_Project.h"
#include "LED_TM1638.h"

interrupt void cpu_timer1_isr(void);  //timer1
interrupt void myXint1_isr(void);     //xint1
interrupt void cpu_timer0_isr(void);  //timer0
// interrupt void EPWM4Int_isr(void);    //EPWM4
// interrupt void Ecap1Int_isr(void);    //ECAP1
// interrupt void MyAdcInt1_isr(void);   //ADCINT1
void HorseRunning_1(int no);
void HorseRunning_2(int no);
void HorseRunning_3(int no);
int keytime = 0 ;
int is_setting = 0;
int suminput = 0;



void HorseRunning(int16 no,int sum_input);

#define Led0Blink() GpioDataRegs.GPACLEAR.bit.GPIO0 = 1
#define Led1Blink() GpioDataRegs.GPACLEAR.bit.GPIO1 = 1
#define Led2Blink() GpioDataRegs.GPACLEAR.bit.GPIO2 = 1
#define Led3Blink() GpioDataRegs.GPACLEAR.bit.GPIO3 = 1
#define Led0Blank() GpioDataRegs.GPASET.bit.GPIO0 = 1
#define Led1Blank() GpioDataRegs.GPASET.bit.GPIO1 = 1
#define Led2Blank() GpioDataRegs.GPASET.bit.GPIO2 = 1
#define Led3Blank() GpioDataRegs.GPASET.bit.GPIO3 = 1


void  Xint1_Init()
{
    EALLOW;
    XIntruptRegs.XINT1CR.bit.POLARITY = 0;
    XIntruptRegs.XINT1CR.bit.ENABLE = 1;
    EDIS;
}

void  Xint2_Init()
{
    EALLOW;
    XIntruptRegs.XINT2CR.bit.POLARITY = 0;
    XIntruptRegs.XINT2CR.bit.ENABLE = 1;
    EDIS;
}

void HorseIO_Init()
{
	EALLOW;
    GpioDataRegs.GPASET.bit.GPIO0 = 1;
    GpioDataRegs.GPASET.bit.GPIO1 = 1;
    GpioDataRegs.GPASET.bit.GPIO2 = 1;
    GpioDataRegs.GPASET.bit.GPIO3 = 1;
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO0 = 1;
    GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO1 = 1;
    GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO2 = 1;
    GpioCtrlRegs.GPAMUX1.bit.GPIO3 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO3 = 1;
    EDIS;
	
}

void IO_12_INIT()
{
    EALLOW;
    GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 0;
    GpioCtrlRegs.GPADIR.bit.GPIO12 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO12 = 0;
    GpioIntRegs.GPIOXINT1SEL.bit.GPIOSEL = 12;
    GpioCtrlRegs.GPACTRL.bit.QUALPRD1 = 0xFF;
    EDIS;
}


void DelaymS(int tm)
{
  int i;
  unsigned int j;
    for(i = 0;i < tm ;i++){
      j =  60000;
      while(j != 0)j--;
      }
}
void main(void)
{
    int j ;
      InitSysCtrl();  	//初始化系统时钟，选择内部晶振1，10MHZ，12倍频，2分频，初始化外设时钟，低速外设,4分频
	  DINT; 				//关总中断
	  IER = 0x0000;  	 	//关CPU中断使能
	  IFR = 0x0000;   	//清CPU中断标志
	  InitPieCtrl();  	//关pie中断
	  InitPieVectTable();	//清中断向量表

	  EALLOW;				/**配置中断向量表*****/
	  PieVectTable.TINT0 = &cpu_timer0_isr;
	  PieVectTable.XINT1 = &myXint1_isr;
    //PieVectTable.TINT1 = &cpu_timer1_isr;
	  // PieVectTable.ECAP1_INT = &Ecap1Int_isr;
	  // PieVectTable.EPWM4_INT = &EPWM4Int_isr;
	  // PieVectTable.ADCINT1 = &MyAdcInt1_isr;
	  EDIS;
    

	 //  MemCopy(&RamfuncsLoadStart, &RamfuncsLoadEnd, &RamfuncsRunStart);
	  InitFlash();

	  InitCpuTimers();   	// 初始化定时器
    EALLOW;		
    ConfigCpuTimer(&CpuTimer0,60,10000);
    CpuTimer0Regs.TCR.bit.TSS = 0;
    CpuTimer0Regs.TCR.bit.TRB = 1;
    CpuTimer0.InterruptCount = 0;
    EDIS;
    IO_12_INIT();
    HorseIO_Init();
    Xint1_Init();
	  TM1638_Init(); 		//初始化LED

    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;
    PieCtrlRegs.PIEIER1.bit.INTx4 = 1;
    IER |= M_INT1;

	  EINT;
	  ERTM;


	while(1){

	  DelaymS(5);
	  j++;
	  j = j & 0xf;
	  HorseRunning(j,suminput);
      LED_Show(1,suminput%10,0);
      LED_Show(2,(suminput/10)%10,0);
	  }
}

void HorseRunning(int no,int sum_input)
{
  switch (sum_input %3)
  {
  case 1:
    HorseRunning_1(no);
    break;
  case 2:
    HorseRunning_2(no);
    break;
  default:
    HorseRunning_3(no);
    break;
  }
}

void HorseRunning_1(int no)
{
    if(no & 0x1)Led0Blink();
    else Led0Blank();
    if(no & 0x2)Led1Blink();
    else Led1Blank();
    if(no & 0x4)Led2Blink();
    else Led2Blank();
    if(no & 0x8)Led3Blink();
    else Led3Blank();
}
void HorseRunning_2(int no)
{
    if((no%4)==0)Led0Blink();
    else Led0Blank();
    if((no%4)==1)Led1Blink();
    else Led1Blank();
    if((no%4)==2)Led2Blink();
    else Led2Blank();
    if((no%4)==3)Led3Blink();
    else Led3Blank();
}

void HorseRunning_3(int no)
{
    if((no%4)==3)Led0Blink();
    else Led0Blank();
    if((no%4)==2)Led1Blink();
    else Led1Blank();
    if((no%4)==1)Led2Blink();
    else Led2Blank();
    if((no%4)==0)Led3Blink();
    else Led3Blank();
}
interrupt void myXint1_isr(void)
{
   
   if ((is_setting == 0)&&(keytime > 20))
   {
    keytime = 0;
    is_setting = 1;
   }
   else if ((is_setting == 1)&&(keytime > 20))
   {
    keytime = 0;
    is_setting = 0;
    suminput = suminput +1;
   }
   PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
   
}

interrupt void cpu_timer1_isr(void) {

  	PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

interrupt void cpu_timer0_isr(void) {
    if(keytime >255)
    {
      keytime = 25;
    }
    else
    {
      keytime = keytime + 1;
    }

  	PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}
