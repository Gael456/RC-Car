/**
 * @file Motor.c
 *
 * @brief Source file for the DC (TT) motor driver.
 *
 * This file drives four TT DC motors as two sides (skid-steer) through an
 * L298N-style H-bridge (OSOYOO Model-X). PWM Module 0 (generator 1) produces
 * two speed/enable signals on PB4 (ENA) and PB5 (ENB), and four GPIO lines on
 * PD0-PD3 set the H-bridge direction inputs (IN1-IN4).
 *
 * The following components are used:
 *  -   3.3V - 6V TT DC Motor
 *  -   L298N-based motor driver (OSOYOO Model-X)
 *
 * @author Gael Esparza Lobatos
 */

#include "Motor.h"

static uint16_t g_period = 16000;


/**
 * @brief Initializes the motor driver: PWM speed outputs and GPIO direction lines.
 *
 * Configures PWM Module 0 (generator 1) to produce two enable/speed signals on
 * PB4 (ENA) and PB5 (ENB), each driven HIGH at LOAD and LOW at its compare
 * match. Also sets PD0-PD3 as GPIO direction outputs (IN1-IN4) for the H-bridge.
 *
 * @param period_constant PWM period in timer counts (sets the frequency).
 * @param duty_cycle      Initial duty cycle in counts; must be less than
 *                        period_constant, or the function returns early.
 *
 * @return None
 */
void Motor_Init(uint16_t period_constant, uint16_t duty_cycle)
{

    g_period = period_constant;

    // Reject an invalid duty cycle: it must be less than the period
    // (below 100%). Otherwise return without configuring anything.
    if (duty_cycle >= period_constant) return;

    // Enable the clock to PWM Module 0 (R0 bit of the RCGCPWM register)
    SYSCTL->RCGCPWM |= 0x01;

    // ---- PWM speed (enable) outputs: PB4 = ENA, PB5 = ENB ----

    SYSCTL->RCGCGPIO |= 0x02; // Enable the clock to GPIO Port B

    GPIOB->DIR |= 0x30; // Set PB4 and PB5 as outputs

    GPIOB->AFSEL |= 0x30; // Select the alternate (PWM) function on PB4 and PB5

    GPIOB->PCTL &= ~0xFFFF0000; // Clear the PCTL fields for PB4 and PB5

    GPIOB->PCTL |= 0x00440000; // Assign PB4/PB5 to M0PWM2/M0PWM3

    GPIOB->DEN |= 0x30; // Enable the digital function on PB4 and PB5


    PWM0->_1_CTL &= ~0x01; // Disable generator 1 while configuring

    // Configure generator 1's outputs: drive each pin HIGH at LOAD and LOW at
    // its compare match, producing a standard duty-cycle PWM.
    // CMPA -> PB4 (ENA), CMPB -> PB5 (ENB).
    PWM0->_1_GENA = 0x00000008C; // Set on LOAD, clear on CMPA
    PWM0->_1_GENB = 0x00000080C; // Set on LOAD, clear on CMPB

    // Set the PWM period via the LOAD register (bits 15:0): the number of
    // clock cycles the counter takes to count down to zero.
    PWM0->_1_LOAD = (period_constant - 1);

    // Set the initial duty on both enable channels (CMPA/CMPB, bits 15:0). The
    // output goes low when the counter reaches this value, so a smaller value
    // means a higher duty.
    PWM0->_1_CMPA = (period_constant - duty_cycle);
    PWM0->_1_CMPB = (period_constant - duty_cycle);


    // Enable generator 1 (ENABLE bit of the PWM1CTL register)
    PWM0->_1_CTL |= 0x01;

    // Route the two PWM signals (M0PWM2/M0PWM3) out to pins PB4/PB5 (ENA/ENB)
    PWM0->ENABLE |= 0x0C;

    // ---- Direction outputs: PD0-PD3 = IN1-IN4 ----

    SYSCTL->RCGCGPIO |= 0x08; // Enable the clock to GPIO Port D (bit 3)

    GPIOD->DIR |= 0x0F; // Set PD0-PD3 as outputs

    GPIOD->AFSEL &= ~0x0F; // Use PD0-PD3 as plain GPIO (clear alternate function)

    GPIOD->DEN |= 0x0F; // Enable the digital function on PD0-PD3

    GPIOD->DATA &= ~0x0F; // Initialize the direction outputs low

}


/**
 * @brief Drives both sides of the car from signed speed values.
 *
 * Positive = forward, negative = reverse, zero = stop; the magnitude sets
 * the PWM duty (speed). Turning comes from giving the two sides different or
 * opposite values (e.g. one positive and one negative pivots in place).
 *
 * @param left  Signed speed for the left side (ENA / PB4).
 * @param right Signed speed for the right side (ENB / PB5).
 *
 * @return None
 */
void drive(int16_t left, int16_t right)
{
    // Command each side independently; the sign of each value sets direction.
    set_side('l', left);

    set_side('r', right);

}

/**
 * @brief Sets one side's speed and direction from a signed value.
 *
 * The magnitude sets the PWM duty on the side's enable pin (speed), and the
 * sign sets the two direction (IN) pins to opposite levels for forward/reverse,
 * or both low to stop. The magnitude is first clamped below the period.
 *
 * Left  -> ENA (_1_CMPA / PB4), direction on PD2/PD3.
 * Right -> ENB (_1_CMPB / PB5), direction on PD0/PD1.
 *
 * @param side         'l' = left, 'r' = right.
 * @param signed_speed >0 forward, <0 reverse, 0 stop; magnitude = speed.
 *
 * @return None
 */
void set_side(char side, int16_t signed_speed)
{
    // Clamp the magnitude below the period (in both directions) so that
    // g_period - |speed| can never underflow.
    if (signed_speed >= g_period) signed_speed = g_period - 1;
    if (signed_speed <= -g_period) signed_speed = -(g_period - 1);

    // Left side: speed on ENA (_1_CMPA / PB4), direction on PD2/PD3
    if(side == 'l')
    {
        if(signed_speed > 0)        // Forward: PD3 high, PD2 low
        {
            PWM0->_1_CMPA = g_period - signed_speed;
            GPIOD->DATA = (GPIOD->DATA & ~0x0C) | 0x08;
        }
        else if(signed_speed < 0)   // Reverse: PD2 high, PD3 low
        {
            PWM0->_1_CMPA = g_period - abs(signed_speed);
            GPIOD->DATA = (GPIOD->DATA & ~0x0C) | 0x04;
        }
        else                        // Stop: PD2/PD3 low (motor off)
        {
            PWM0->_1_CMPA = 0;
            GPIOD->DATA &= ~0x0C;
        }
    }

    // Right side: speed on ENB (_1_CMPB / PB5), direction on PD0/PD1
    if(side == 'r')
    {
        if(signed_speed > 0)        // Forward: PD1 high, PD0 low
        {
            PWM0->_1_CMPB = g_period - abs(signed_speed);
            GPIOD->DATA = (GPIOD->DATA & ~0x03) | 0x02;
        }
        else if(signed_speed < 0)   // Reverse: PD0 high, PD1 low
        {
            PWM0->_1_CMPB = g_period - abs(signed_speed);
            GPIOD->DATA = (GPIOD->DATA & ~0x03) | 0x01;
        }
        else                        // Stop: PD0/PD1 low (motor off)
        {
            PWM0->_1_CMPB = 0;
            GPIOD->DATA &= ~0x03;
        }
    }

}
