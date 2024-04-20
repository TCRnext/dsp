/**
 * 
 * main.c
 */
#include "DSP28x_Project.h"
#include "LED_TM1638.h"
void init_epwm4GPIO(void);
void init_epwm4(int PWMFrequency, int duty ,int deadtime_percent);
void init_eCAP1GPIO(void);
void init_eCAP1(void);


interrupt void EPWM4Int_isr(void);    //EPWM4
interrupt void Ecap1Int_isr(void);    //ECAP1
// interrupt void MyAdcInt1_isr(void);   //ADCINT1

int PWMTimes = 0;

void init_epwm4GPIO(void)
{
	EALLOW;
	GpioCtrlRegs.GPAPUD.bit.GPIO6 = 0;
	GpioCtrlRegs.GPAPUD.bit.GPIO7 = 0;
  	GpioCtrlRegs.GPAMUX1.bit.GPIO6 = 1;
  	GpioCtrlRegs.GPAMUX1.bit.GPIO7 = 1;
	EDIS;
}

void init_epwm4(int PWMFrequency, int duty ,int deadtime_percent)//频率(Khz)，占空比1-99，死区占比0-25%
{
	init_epwm4GPIO();
  	int PWMPRD = 60000/PWMFrequency/2;
  	int deadtime = 0;
  	if (deadtime_percent >25)
  	{
  	  deadtime = PWMPRD *25 /100;
  	}
  	else if (deadtime_percent <=0)
  	{
  	  deadtime = 0;
  	}
  	else
  	{
  	  deadtime = PWMPRD *deadtime_percent/100;
  	}
  	//deadtime = 120;
  	Uint32 CMPA = (Uint32)PWMPRD *(Uint32) duty /100UL;
  	Uint32 CMPB = (Uint32)PWMPRD *(Uint32) duty /100UL;
  	EALLOW;
  	SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC =0;		//断开PWM模块的时钟
  	EDIS;

  	EPwm4Regs.TBPRD = PWMPRD;					//设置PWM的周期数

  	EPwm4Regs.TBPHS.half.TBPHS = 0;				//设置相位寄存器为0

	/**
	 * time base 寄存器设置 P187
	*/
  	EPwm4Regs.TBCTL.bit.CLKDIV = 0;				//分频位，分频系数为2^n, 3bit
  	EPwm4Regs.TBCTL.bit.HSPCLKDIV = 0;			//分频位，分频系数为2n+(n==0)(1,2,4,6,8,10,12,14),3bit
  	EPwm4Regs.TBCTL.bit.CTRMODE = 2;			//计数模式
  	EPwm4Regs.TBCTL.bit.PHSEN = 0;				//
  	EPwm4Regs.TBCTL.bit.PRDLD = 0;				//不使用相位计时器
  	EPwm4Regs.TBCTL.bit.SYNCOSEL = TB_CTR_ZERO;	//CTR=0时装载

	/**
	 * 比较器模块寄存器设置 P188-189
	*/
    EPwm4Regs.CMPA.half.CMPA = (int)CMPA;            //CMPA
    EPwm4Regs.CMPB = (int)CMPB;                      //并未用到CMPB
  	EPwm4Regs.CMPCTL.bit.SHDWAMODE =0; 			//启动影子寄存器/双缓冲模式
  	EPwm4Regs.CMPCTL.bit.SHDWBMODE =0;
  	EPwm4Regs.CMPCTL.bit.LOADAMODE =2;			//T = PRD或 T = 0时装载
  	EPwm4Regs.CMPCTL.bit.LOADBMODE =2;

	/**
	 * AQ动作限定器子模块设置 P190
	*/
  	EPwm4Regs.AQCTLA.bit.CAU = 1;				//PRD=CMPA上升置0（A通道）
  	EPwm4Regs.AQCTLA.bit.CAD = 2;				//PRD=CMPA下降置1（A通道）
  	EPwm4Regs.AQCTLB.bit.CBU = 0;				//COMPB不用
  	EPwm4Regs.AQCTLB.bit.CBD = 0;
	//只配置了A，没有配置B

	/**
	 *  DB死区子模块设置 P193
	*/
  	EPwm4Regs.DBCTL.bit.OUT_MODE = 3;			//输出模式全使能死区
  	EPwm4Regs.DBCTL.bit.POLSEL = 2;				//PWMB为输入源反向
  	EPwm4Regs.DBFED = deadtime;					//上升死区控制
  	EPwm4Regs.DBRED = deadtime;					//下降死区控制

	/**
	 *  事件触发器设置 P194
	*/
  	EPwm4Regs.ETSEL.bit.INTEN = 0;				//中断使能位
  	EPwm4Regs.ETSEL.bit.INTSEL = 4;				//上升沿COMPA启动时间
  	EPwm4Regs.ETPS.bit.INTPRD = 0;				//在第一个事件处触发中断
  	EPwm4Regs.ETCLR.bit.INT = 1;				//清空中断

  	EALLOW;
  	SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC =1;
  	EDIS; 
}

void init_eCAP1GPIO(void)
{
	EALLOW;
    GpioCtrlRegs.GPAPUD.bit.GPIO19 = 0;  // GPIO19使能上拉
    GpioCtrlRegs.GPAMUX2.bit.GPIO19 = 3; // 将GPIO19选做eCAP1的输入引脚I/O
    EDIS;
}

void init_eCAP1(void)
{
	init_eCAP1GPIO();
    ECap1Regs.ECEINT.all = 0x0000;    // 禁能所有捕获事件引发的中断
    ECap1Regs.ECCLR.all = 0xFFFF;     // 清除所有捕获产生的中断标志
    // 绝对时间戳，上下沿捕捉
    ECap1Regs.ECCTL1.bit.CAP1POL = 0; // 上升沿捕获事件1
    ECap1Regs.ECCTL1.bit.CAP2POL = 1; // 下降沿捕获事件2
    ECap1Regs.ECCTL1.bit.CAP3POL = 0; // 上升沿捕获事件3
    ECap1Regs.ECCTL1.bit.CAP4POL = 1; // 下降沿捕获事件4
    ECap1Regs.ECCTL1.bit.CTRRST1 = 0; // 在捕获事件1出现时，不复位计数器，绝对时间戳
    ECap1Regs.ECCTL1.bit.CTRRST2 = 0;
    ECap1Regs.ECCTL1.bit.CTRRST3 = 0;
    ECap1Regs.ECCTL1.bit.CTRRST4 = 0;
    ECap1Regs.ECCTL1.bit.CAPLDEN = 1;     // 在捕获事件时使能装载CAP1~CAP4寄存器
    ECap1Regs.ECCTL1.bit.PRESCALE = 0;    // 事件过滤器1分频，即：PSout=eCAP1

    ECap1Regs.ECCTL2.bit.CAP_APWM = 0;    // 用作捕获模式，eCAP1引脚做捕获输入 (P157)
    ECap1Regs.ECCTL2.bit.CONT_ONESHT = 0; // 连续模式操作
    ECap1Regs.ECCTL2.bit.SYNCO_SEL = 2;   // 禁止同步输出信号
    ECap1Regs.ECCTL2.bit.SYNCI_EN = 0;    // 禁止同步输入
    ECap1Regs.ECCTL2.bit.TSCTRSTOP = 1;   // 允许TSCTR运行
    ECap1Regs.ECEINT.bit.CEVT4 = 1;       // CEVT4 触发 ISR 调用
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

	PieVectTable.ECAP1_INT = &Ecap1Int_isr;
	PieVectTable.EPWM4_INT = &EPWM4Int_isr;
	  // PieVectTable.ADCINT1 = &MyAdcInt1_isr;
	EDIS;
    
	InitFlash();

	InitCpuTimers();   	// 初始化定时器
    EALLOW;		
    ConfigCpuTimer(&CpuTimer0,60,10000);

    EDIS;

    //Xint1_Init();
	TM1638_Init(); 		//初始化LED
    init_epwm4(20,30,4);
	init_eCAP1();

    PieCtrlRegs.PIEIER3.bit.INTx4 = 1;      // EPWM4_INT
    PieCtrlRegs.PIEIER4.bit.INTx1 = 1;      // ECAP1_INT
    IER |= M_INT3; 							//ePWM
    IER |= M_INT4;							//eCAP
	EINT;
	ERTM;


	while(1)
	{
	}
}




interrupt void EPWM4Int_isr(void)
{
    PWMTimes++;
    EALLOW;
    EPwm4Regs.ETCLR.bit.INT = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;
    EDIS;
}

interrupt void Ecap1Int_isr(void)
{
    int t1 = ECap1Regs.CAP1;        // t1为时间戳1
    int t2 = ECap1Regs.CAP2;
    int t3 = ECap1Regs.CAP3;
    int t4 = ECap1Regs.CAP4;
    int Period1 = t3 - t1;          // 计算第1个周期
    int DutyOnTime1 = t2 - t1;      // 计算导通时间
    int DutyOffTime1 = t3 - t2;     // 计算断开时间

	float duty = (float)DutyOnTime1/(float)Period1*100;
    LedShow(Period1, 1);
    LedShow((int)duty, 2);
    EALLOW;
    ECap1Regs.ECCLR.bit.CEVT4 = 1;          // 清除CEVT4标志
    ECap1Regs.ECCLR.bit.INT = 1;            // 清除INT标志
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP4;
    EDIS;
}

void LedShow(int no, int mode)
{
    int i;
    int j;



    if(mode == 1)
    {
        for (i = 1; i<5; i++)
        {
            j = no % 10;
            LED_Show(i, j, 0);
            no = no / 10;
        }
    }
    else if(mode == 2)
    {
        for (i = 6; i<9 ; i++)
        {
            j = no % 10;
            LED_Show(i, j, 0);
            no = no / 10;
        }
    }
}
