#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

// Definiciones de pines (ajustados para EK-TM4C1294XL)
#define ENA_PIN        GPIO_PIN_2       // PWM en PF2 (puede variar según tu conexión)
#define IN1_PIN        GPIO_PIN_4       // Dirección IN1 (PF1)
#define IN2_PIN        GPIO_PIN_5       // Dirección IN2 (PF3)
#define FORWARD_BUTTON_PIN GPIO_PIN_0   // Botón PJ0 (SW1 en la placa)
#define BACKWARD_BUTTON_PIN GPIO_PIN_1  // Botón PJ1 (SW2 en la placa)

// Variables globales
uint32_t g_ui32SysClock;
uint32_t g_ui32PWMIncrement;

// Configuración del UART
void ConfigureUART(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 115200, g_ui32SysClock);
}

// Configuración del PWM
void ConfigurePWM(void) {
    uint32_t ui32PWMClockRate;
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinConfigure(GPIO_PF2_M0PWM2);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2);
    
    PWMClockSet(PWM0_BASE, PWM_SYSCLK_DIV_8);
    ui32PWMClockRate = g_ui32SysClock / 8;
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, ui32PWMClockRate / 250);  // 250Hz
    
    g_ui32PWMIncrement = ((ui32PWMClockRate / 250) * 95) / 100;  // 95% duty cycle
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, g_ui32PWMIncrement);
    
    PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, true);
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
}

// Configuración de pines de dirección y botones
void ConfigureDirectionAndButtons(void) {
    // Habilitar puertos F (motor) y J (botones)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    
    // Configurar pines de dirección como salidas
    GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, IN1_PIN | IN2_PIN);
    
    // Configurar botones con pull-up (los botones van a GND)
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, FORWARD_BUTTON_PIN | BACKWARD_BUTTON_PIN);
    GPIOPadConfigSet(GPIO_PORTJ_BASE, FORWARD_BUTTON_PIN | BACKWARD_BUTTON_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

int main(void) {
    g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_240), 120000000);
    
    ConfigureUART();
    ConfigurePWM();
    ConfigureDirectionAndButtons();
    
    UARTprintf("Control de Motor con Botones (PJ0=Adelante, PJ1=Atras)\n");
    
    while(1) {
        // Botón PJ0 (SW1) presionado → Motor adelante
        if (!GPIOPinRead(GPIO_PORTJ_BASE, FORWARD_BUTTON_PIN)) {
            GPIOPinWrite(GPIO_PORTK_BASE, IN1_PIN, IN1_PIN);  // IN1 = HIGH
            GPIOPinWrite(GPIO_PORTK_BASE, IN2_PIN, 0);        // IN2 = LOW
            UARTprintf("Motor: Adelante\n");
        }
        
        // Botón PJ1 (SW2) presionado → Motor atrás
        if (!GPIOPinRead(GPIO_PORTJ_BASE, BACKWARD_BUTTON_PIN)) {
            GPIOPinWrite(GPIO_PORTK_BASE, IN1_PIN, 0);        // IN1 = LOW
            GPIOPinWrite(GPIO_PORTK_BASE, IN2_PIN, IN2_PIN);  // IN2 = HIGH
            UARTprintf("Motor: Atras\n");
        }
        
        // Pequeño retardo para evitar rebotes
        SysCtlDelay(g_ui32SysClock / 1000);
    }
}