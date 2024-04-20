
/*
 * main.c
 */
#include "DSP28x_Project.h"
#include "LED_TM1638.h"
void init_epwm4GPIO(void);
void init_epwm4(int PWMFrequency, int duty ,int deadtime_percent);
void InitADC();
interrupt void EPWM4Int_isr(void);    //EPWM4
interrupt void MyAdcInt1_isr(void);   //ADCINT1
void LedShow(int no, int mode);
void DelaymS(int tm);
int PWMTimes = 0;
float v_pwm = 0.0;
float v_Rp = 0.0;
int SampleDeltaT = 15;
long int ADC_usDELAY = 10000;
long int ADCB7 = 0;
long int ADCA7 = 0;
long int ADCB3 = 0;

void DelaymS(int tm)
{
  int i;
  unsigned int j;
    for(i = 0;i < tm ;i++){
      j =  60000;
      while(j != 0)j--;
      }
}

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
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC =0;       //断开PWM模块的时钟
    EDIS;

    EPwm4Regs.TBPRD = PWMPRD;                   //设置PWM的周期数

    EPwm4Regs.TBPHS.half.TBPHS = 0;             //设置相位寄存器为0

    /**
     * time base 寄存器设置 P187
    */
    EPwm4Regs.TBCTL.bit.CLKDIV = 0;             //分频位，分频系数为2^n, 3bit
    EPwm4Regs.TBCTL.bit.HSPCLKDIV = 0;          //分频位，分频系数为2n+(n==0)(1,2,4,6,8,10,12,14),3bit
    EPwm4Regs.TBCTL.bit.CTRMODE = 2;            //计数模式
    EPwm4Regs.TBCTL.bit.PHSEN = 0;              //
    EPwm4Regs.TBCTL.bit.PRDLD = 0;              //不使用相位计时器
    EPwm4Regs.TBCTL.bit.SYNCOSEL = TB_CTR_ZERO; //CTR=0时装载

    /**
     * 比较器模块寄存器设置 P188-189
    */
    EPwm4Regs.CMPA.half.CMPA = (int)CMPA;            //CMPA
    EPwm4Regs.CMPB = (int)CMPB;                      //并未用到CMPB
    //EPwm4Regs.CMPA.half.CMPA = PWMPRD * duty /100;//bugs,haha
    //EPwm4Regs.CMPB = PWMPRD * duty /100;      //并未用到CMPB
    EPwm4Regs.CMPCTL.bit.SHDWAMODE =0;          //启动影子寄存器/双缓冲模式
    EPwm4Regs.CMPCTL.bit.SHDWBMODE =0;
    EPwm4Regs.CMPCTL.bit.LOADAMODE =2;          //T = PRD或 T = 0时装载
    EPwm4Regs.CMPCTL.bit.LOADBMODE =2;

    /**
     * AQ动作限定器子模块设置 P190
    */
    EPwm4Regs.AQCTLA.bit.CAU = 1;               //PRD=CMPA上升置0（A通道）
    EPwm4Regs.AQCTLA.bit.CAD = 2;               //PRD=CMPA下降置1（A通道）
    EPwm4Regs.AQCTLB.bit.CBU = 0;               //COMPB不用
    EPwm4Regs.AQCTLB.bit.CBD = 0;
    //只配置了A，没有配置B

    /**
     *  DB死区子模块设置 P193
    */
    EPwm4Regs.DBCTL.bit.OUT_MODE = 3;           //输出模式全使能死区
    EPwm4Regs.DBCTL.bit.POLSEL = 2;             //PWMB为输入源反向
    EPwm4Regs.DBFED = deadtime;                 //上升死区控制
    EPwm4Regs.DBRED = deadtime;                 //下降死区控制

    /**
     *  事件触发器设置 P194
    */
    EPwm4Regs.ETSEL.bit.SOCAEN = 1;  // 使能EPWM1中断
    EPwm4Regs.ETSEL.bit.SOCASEL = 1; // 时基计数器==0时使能事件
    EPwm4Regs.ETPS.bit.SOCAPRD = 1;  // 在第一个事件处触发中断
    EPwm4Regs.ETCLR.bit.SOCA = 1;    // 清中断标志

    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC =1;
    EDIS;
}

void InitADC()
{
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.ADCENCLK = 1;
    (*Device_cal)();
    EDIS;
    DelaymS(5);

    EALLOW;
    // 上电
    AdcRegs.ADCCTL1.bit.ADCBGPWD = 1;  // 内核内的带隙缓冲器电路上电
    AdcRegs.ADCCTL1.bit.ADCREFPWD = 1; // 内核内的参考缓冲器电路上电
    AdcRegs.ADCCTL1.bit.ADCPWDN = 1;   // 内核内的模拟电路上电

    AdcRegs.ADCCTL1.bit.ADCENABLE = 1; // ADC使能
    // AdcRegs.ADCCTL1.bit.ADCREFSEL = 1; // 使用外部VREFHI/VREFLO参考电压
    AdcRegs.ADCCTL1.bit.ADCREFSEL = 0; // 使用内部带隙产生参考电压
    EDIS;

    DelaymS(5); // 转换ADC通道前延迟
    //AdcOffsetSelfCal();
    DelaymS(5); // 转换ADC通道前延迟

    EALLOW;
    // 设置控制寄存器
    AdcRegs.ADCCTL1.bit.INTPULSEPOS = 1;  // 转换完成前一个ADC时钟周期产生EOC
    AdcRegs.INTSEL1N2.bit.INT1E = 1;      // ADCINT1使能
    AdcRegs.INTSEL1N2.bit.INT1CONT = 0;   // 禁用ADCINT 1连续模式
    AdcRegs.INTSEL1N2.bit.INT1SEL = 0x0f; // 设置EOC 15以触发ADCINT 1启动
    AdcRegs.ADCSAMPLEMODE.all = 0xff;     // SOCAx和SOCBx同步采样

    // 对ADCINA7、ADCINB7过采样
    AdcRegs.ADCSOC0CTL.bit.CHSEL = 0x07; // A7,Result0; B7,Result1
    AdcRegs.ADCSOC2CTL.bit.CHSEL = 0x07; // A7,Result2; B7,Result3
    AdcRegs.ADCSOC4CTL.bit.CHSEL = 0x07; // A7,Result4; B7,Result5
    AdcRegs.ADCSOC6CTL.bit.CHSEL = 0x07; // A7,Result6; B7,Result7

    // 对ADCINA3、ADCINB3过采样 实际上未使用
    AdcRegs.ADCSOC8CTL.bit.CHSEL = 0x03;  // A3,Result8; B3,Result9
    AdcRegs.ADCSOC10CTL.bit.CHSEL = 0x03; // A3,Result10; B3Result11
    AdcRegs.ADCSOC12CTL.bit.CHSEL = 0x03; // A3,Result12; B3,Result13
    AdcRegs.ADCSOC14CTL.bit.CHSEL = 0x03; // A3,Result14; B3,Result15

    // ADCTRIG5 – ePWM4, ADCSOCA，用一个触发源触发所有中断
    AdcRegs.ADCSOC0CTL.bit.TRIGSEL = 0x0B;
    AdcRegs.ADCSOC2CTL.bit.TRIGSEL = 0x0B;
    AdcRegs.ADCSOC4CTL.bit.TRIGSEL = 0x0B;
    AdcRegs.ADCSOC6CTL.bit.TRIGSEL = 0x0B;
    AdcRegs.ADCSOC8CTL.bit.TRIGSEL = 0x0B;
    AdcRegs.ADCSOC10CTL.bit.TRIGSEL = 0x0B;
    AdcRegs.ADCSOC12CTL.bit.TRIGSEL = 0x0B;
    AdcRegs.ADCSOC14CTL.bit.TRIGSEL = 0x0B;

    // 设置SOC0~1采样窗口的长度
    AdcRegs.ADCSOC0CTL.bit.ACQPS = SampleDeltaT;
    AdcRegs.ADCSOC2CTL.bit.ACQPS = SampleDeltaT;
    AdcRegs.ADCSOC4CTL.bit.ACQPS = SampleDeltaT;
    AdcRegs.ADCSOC6CTL.bit.ACQPS = SampleDeltaT;
    AdcRegs.ADCSOC8CTL.bit.ACQPS = SampleDeltaT;
    AdcRegs.ADCSOC10CTL.bit.ACQPS = SampleDeltaT;
    AdcRegs.ADCSOC12CTL.bit.ACQPS = SampleDeltaT;
    AdcRegs.ADCSOC14CTL.bit.ACQPS = SampleDeltaT;
    EDIS;
}


void main(void)
{
    int j ;
    InitSysCtrl();      //初始化系统时钟，选择内部晶振1，10MHZ，12倍频，2分频，初始化外设时钟，低速外设,4分频
    DINT;               //关总中断
    IER = 0x0000;       //关CPU中断使能
    IFR = 0x0000;       //清CPU中断标志
    InitPieCtrl();      //关pie中断
    InitPieVectTable(); //清中断向量表

    EALLOW;             /**配置中断向量表*****/

    PieVectTable.EPWM4_INT = &EPWM4Int_isr;
    PieVectTable.ADCINT1 = &MyAdcInt1_isr;
    EDIS;
    

     //  MemCopy(&RamfuncsLoadStart, &RamfuncsLoadEnd, &RamfuncsRunStart);
    InitFlash();

    InitCpuTimers();    // 初始化定时器
    EALLOW;
    ConfigCpuTimer(&CpuTimer0,60,10000);

    EDIS;

    //Xint1_Init();
    TM1638_Init();      //初始化LED
    init_epwm4(20,50,0);
    InitADC();
    PieCtrlRegs.PIECTRL.bit.ENPIE = 1;
    PieCtrlRegs.PIEIER3.bit.INTx4 = 1;      // EPWM4_INT
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;      // ADC1_INT
    IER |= M_INT3;                          //ePWM
    IER |= M_INT1;                          //ADC1
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

interrupt void MyAdcInt1_isr(void)
{
    ADCA7 = AdcResult.ADCRESULT0 + AdcResult.ADCRESULT2;
    ADCA7 += AdcResult.ADCRESULT4 + AdcResult.ADCRESULT6;

    ADCB7 = AdcResult.ADCRESULT1 + AdcResult.ADCRESULT3;
    ADCB7 += AdcResult.ADCRESULT5 + AdcResult.ADCRESULT7;

    v_Rp = ADCA7 *3.3/(4096*4);
    v_pwm = ADCB7 *3.3/(4096*4);

    EPwm4Regs.CMPA.half.CMPA = (int)((float)EPwm4Regs.TBPRD * (v_Rp/3.3));

    LedShow((int)(v_Rp*1000),1);
    LedShow((int)(v_pwm*1000),2);
    EPwm1Regs.ETCLR.bit.SOCA = 1;
    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
    EINT;
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
        for (i = 5; i<9 ; i++)
        {
            j = no % 10;
            LED_Show(i, j, 0);
            no = no / 10;
        }
    }
}
