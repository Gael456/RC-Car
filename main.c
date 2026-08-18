/**
 * @file main.c
 *
 * @brief Test program for Motor A (PWM generator 0 on PB6/PB7) using
 *        Timer0A interrupts for a millisecond tick.
 *
 * @author Gael Esparza Lobatos
 */

#include "TM4C123GH6PM.h"
#include "Motor.h"
#include "Timer_Interrupt.h"

#define SYSTEM_CLOCK 50000000U  // 50 MHz system clock

// millisecond tick counter (incremented in the Timer0A ISR)
static volatile uint32_t g_msTicks = 0;

/**
 * @brief User callback for Timer0A interrupts (1 ms period).
 */
static void Timer0ATask(void)
{
    g_msTicks++;
}

/**
 * @brief Delay for the given number of milliseconds using the 1 ms tick.
 */
static void delay_ms(uint32_t ms)
{
    uint32_t start = g_msTicks;
    while ((g_msTicks - start) < ms) {
        // simply wait
    }
}


int main(void)
{
    // 1) Set up a 1 ms tick via Timer0A
    Timer_0A_Interrupt_Init(Timer0ATask);

    // 2) Initialize the motor PWM driver (PB4�PB7)
    //    period = 16000 ? ~1 kHz (assuming 50 MHz/16 divider).
    //    initial duty = 8000 (50%) � will be overridden by our test calls.
    Motor_Init(16000, 8000);

    // 3) Main test loop for Motor A
    while (1) {
        drive(8000, 8000);  // 50% forward
        delay_ms(1000);

        drive(-8000, -8000);  // 50% reverse
        delay_ms(1000);

        drive(0,0);
    }

    // never reached
    return 0;
}
