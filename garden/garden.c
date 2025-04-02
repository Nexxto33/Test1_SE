#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/adc.h"
#include "driverlib/interrupt.h"

uint32_t FS2 = 120000000;

void Timer1IntHandler(void);

int main(void)
{
    SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, 120000000);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOK));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER1));

    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, 0x03);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, 0x11);
    GPIOPinTypeADC(GPIO_PORTK_BASE, 0x08);

    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH19);
    ADCSequenceEnable(ADC0_BASE, 3);
    ADCIntClear(ADC0_BASE, 3);

    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER1_BASE, TIMER_A, FS2 - 1);

    IntEnable(INT_TIMER1A);
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER1_BASE, TIMER_A);

    IntMasterEnable();

    while (1);
}

void Timer1IntHandler(void)
{
    uint32_t ui32Value;

    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
            
    ADCProcessorTrigger(ADC0_BASE, 3);
            
    while (!ADCIntStatus(ADC0_BASE, 3, false));
    
    ADCIntClear(ADC0_BASE, 3);
    ADCSequenceDataGet(ADC0_BASE, 3, &ui32Value);

    if (ui32Value > 2047)
    {
        GPIOPinWrite(GPIO_PORTN_BASE, 0x03, 0x00);
        GPIOPinWrite(GPIO_PORTF_BASE, 0x11, 0x00);
    }
    else if (ui32Value > 1035)
    {
        GPIOPinWrite(GPIO_PORTN_BASE, 0x03, 0x03);
        GPIOPinWrite(GPIO_PORTF_BASE, 0x11, 0x00);
    }
    else
    {
        GPIOPinWrite(GPIO_PORTN_BASE, 0x03, 0x03);
        GPIOPinWrite(GPIO_PORTF_BASE, 0x11, 0x11);
    }
}
