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

int main(void)
{
    // Initialize the motor driver (PWM speed on PB4/PB5, direction on PD0-PD3)
    Motor_Init(16000, 8000);

    // Initialize UART1 (serial link that receives the movement commands)
    UART1_Init();

    // Continuously receive a command, parse it, and drive the car
    while (1)
    {
        parser();
    }

    return 0;
}
