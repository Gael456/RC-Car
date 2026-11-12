/**
 * @file main.c
 *
 * @brief Main program for the RC car.
 *
 * Initializes the motor and UART drivers, then continuously receives movement
 * commands over UART1 and executes them via the command parser. Each command is
 * two signed, comma-separated side speeds that are passed to the motor driver.
 *
 * @author Gael Esparza Lobatos
 */

#include "TM4C123GH6PM.h"
#include "Motor.h"
#include "UART1.h"
#include "Command_Parser.h"
#include "Timer_Interrupt.h"
#include "GPIO.h"

static volatile uint32_t g_msTicks = 0;

static void Timer0ATask(void)
{
	g_msTicks++;
}

static void delay_ms(uint32_t ms)
{
	uint32_t start = g_msTicks;
	while((g_msTicks - start) < ms)
	{
	}
}


int main(void)
{
		RGB_LED_Init();
	
		RGB_LED_Output(RGB_LED_RED);
    // Initialize the motor driver (PWM speed on PB4/PB5, direction on PD0-PD3)
    Motor_Init(16000, 8000);
		
		RGB_LED_Output(RGB_LED_BLUE);

    // Initialize UART1 (serial link that receives the movement commands)
    UART1_Init();
	
		Timer_0A_Interrupt_Init(Timer0ATask);
		
		RGB_LED_Output(RGB_LED_GREEN);
    // Continuously receive a command, parse it, and drive the car
    while (1)
    {
			// parser();
			drive(15000, 15000);
			delay_ms(1000);
			
			drive(-15000, -15000);
			delay_ms(1000);
			
			drive(-15000, 15000);
			delay_ms(1000);
			
			drive(15000, -15000);
			delay_ms(1000);
			
			drive(0,0);
			delay_ms(1000);
    }

    return 0;
}
