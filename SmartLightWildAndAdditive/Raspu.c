#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/uart.h"

#define LED_PWM_PIN GPIO_PIN_3  // PF3 para el control de la intensidad del LED (M0PWM3)
#define PWM_FREQUENCY 1000      // Frecuencia del PWM (en Hz)

char data[100];

// Función para enviar string por UART0
void UARTSendString(const char *str) {
    while (*str) {
        UARTCharPut(UART0_BASE, *str++);
    }
}

void ConfigurePWM(void) {
    // Habilitar el periférico PWM0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0));

    // Habilitar el periférico GPIO para el pin del LED
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));

    // Configurar PF3 como salida PWM
    GPIOPinConfigure(GPIO_PF3_M0PWM3);
    GPIOPinTypePWM(GPIO_PORTF_BASE, LED_PWM_PIN);

    // Configurar el generador de PWM
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    
    // Establecer el clock de precisión para el PWM
    uint32_t pwmClock = SysCtlClockGet() / 64;  // Divisor de reloj para el PWM
    uint32_t period = pwmClock / PWM_FREQUENCY;  // Periodo del PWM

    // Establecer el periodo para el PWM
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, period);  // Establecer el periodo
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, period * 90 / 100);  // Establecer el ciclo de trabajo al 90%

    // Habilitar el generador y la salida PWM
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);
    PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, true);  // Habilitar la salida de PWM
}

int main(void) {
    // Configuración del sistema a 50 MHz (ajusta según tu aplicación)
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480);

    // Configurar el PWM para controlar el LED
    ConfigurePWM();
    
    // Habilitar relojes para UART y GPIO
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configurar UART con reloj interno PIOSC (16 MHz)
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    UARTConfigSetExpClk(UART0_BASE, 16000000, 9600,
        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);

    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_7);
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);

    while (1) {
    
        if (UARTCharsAvail(UART0_BASE)) {
            memset(data, 0, sizeof(data));
            uint32_t i = 0;
            char c;
            
            // Leer los datos recibidos
            while (UARTCharsAvail(UART0_BASE)) {
                c = UARTCharGet(UART0_BASE);
                if (c == '\n' || i >= sizeof(data) - 1)
                    break;
                data[i++] = c;
            }

            data[i] = '\0'; // finalizar cadena

            // Enviar de vuelta el dato recibido para analizar
            UARTSendString("Dato recibido: ");
            UARTSendString(data);
            UARTSendString("\n");

            // Verificar si el dato recibido es menor a 5 y apagar el PWM
            int received_value = atoi(data); // Convertir el dato recibido a entero
            if (received_value < 5) {
                // Apagar el PWM
                PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, false);  // Desactivar la salida de PWM
                UARTSendString("PWM apagado porque el valor es menor a 5\n");
            } 
            else {
                // Establecer el PWM al 90% si el valor es 5 o mayor
                PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, PWMGenPeriodGet(PWM0_BASE, PWM_GEN_1) * 90 / 100);  // Establecer el ciclo de trabajo al 90%
                PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, true);  // Asegurarse de que la salida PWM esté habilitada
                UARTSendString("PWM al 90%\n");
            }
        }
    }
}