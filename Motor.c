/**
 * @file Motor.c
 *
 * @brief Source file for the DC (TT) motor driver.
 *
 * This file contains the function definitions for the motor driver. It uses
 * PWM Module 0 on pins PB4-PB7 to generate the four control signals for two
 * DRV8833 H-bridges, driving the left and right sides of the car.
 *
 * The following components are used:
 *  -   3.3V - 6V TT DC Motor
 *  -   DRV8833 Motor Driver
 *
 * @author Gael Esparza Lobatos
 */

#include "Motor.h"

static uint16_t g_period = 16000;


/**
 * @brief Initializes PWM Module 0 to drive the DC motors via the DRV8833.
 *
 * Configures PB4-PB7 as M0PWM outputs and sets up PWM generators 0 (left
 * side) and 1 (right side) in count-down mode: each output is driven high
 * at LOAD and low at its compare match. Sets the PWM period and an initial
 * duty cycle, then enables the generators and passes the signals to the pins.
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


    SYSCTL->RCGCGPIO |= 0x02; // Enable the clock to GPIO Port B

    GPIOB->DIR |= 0xF0; // Set PB4-PB7 as outputs

    GPIOB->AFSEL |= 0xF0; // Select the alternate (PWM) function on PB4-PB7

    GPIOB->PCTL &= ~0xFFFF0000; // Clear the PCTL fields for PB4-PB7

    GPIOB->PCTL |= 0x44440000; // Assign PB4-PB7 to their M0PWM functions

    GPIOB->DEN |= 0xF0; // Enable the digital function on PB4-PB7

    PWM0->_0_CTL &= ~0x01; // Disable generator 0 while configuring
    PWM0->_1_CTL &= ~0x01; // Disable generator 1 while configuring

    // Configure each generator's output actions: drive the pin HIGH when the
    // counter reloads at LOAD, and LOW when it counts down to the compare
    // value. This produces a standard duty-cycle PWM on each channel.
    // Left side (PWM generator 0)
    PWM0->_0_GENA |= 0x0000008C; // Set on LOAD, clear on CMPA
    PWM0->_0_GENB |= 0x0000080C; // Set on LOAD, clear on CMPB

    // Right side (PWM generator 1)
    PWM0->_1_GENA = 0x00000008C; // Set on LOAD, clear on CMPA
    PWM0->_1_GENB = 0x00000080C; // Set on LOAD, clear on CMPB

    // Set the PWM period via the LOAD registers (bits 15:0): the number of
    // clock cycles the counter takes to count down to zero.
    PWM0->_0_LOAD = (period_constant - 1);
    PWM0->_1_LOAD = (period_constant - 1);

    // Set the initial duty cycle on both channels of both sides via the
    // compare registers (COMPA/COMPB, bits 15:0). The output goes low when
    // the counter reaches this value, so a smaller value means a higher duty.
    PWM0->_0_CMPA = (period_constant - duty_cycle);
    PWM0->_0_CMPB = (period_constant - duty_cycle);

    PWM0->_1_CMPA = (period_constant - duty_cycle);
    PWM0->_1_CMPB = (period_constant - duty_cycle);


    // Enable both generators (ENABLE bit of each PWMnCTL register)
    PWM0->_0_CTL |= 0x01;
    PWM0->_1_CTL |= 0x01;

    // Pass the four PWM signals through to pins PB4-PB7 (M0PWM0-M0PWM3)
    PWM0->ENABLE |= 0x0F;

}


/**
 * @brief Drives both sides of the car from signed speed values.
 *
 * Positive = forward, negative = reverse, zero = stop; the magnitude sets
 * the PWM duty (speed). Turning comes from giving the two sides different or
 * opposite values (e.g. one positive and one negative pivots in place).
 *
 * @param left  Signed speed for the left side (PWM generator 0).
 * @param right Signed speed for the right side (PWM generator 1).
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
 * @brief Sets one side's direction and speed from a signed value.
 *
 * The sign selects direction (forward drives CMPA, reverse drives CMPB) and
 * the magnitude sets the PWM duty; the opposite channel is zeroed. The
 * magnitude is first clamped below the period to prevent underflow.
 *
 * @param side         'l' = left (generator 0), 'r' = right (generator 1).
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

    // Left side (PWM generator 0)
    if(side == 'l')
    {
        if(signed_speed > 0)        // Forward: forward channel on, reverse off
        {
            PWM0->_0_CMPA = g_period - signed_speed;
            PWM0->_0_CMPB = 0;
        }
        else if(signed_speed < 0)   // Reverse: reverse channel on, forward off
        {
            PWM0->_0_CMPA = 0;
            PWM0->_0_CMPB = g_period - abs(signed_speed);
        }
        else                        // Stop: both channels off
        {
            PWM0->_0_CMPA = 0;
            PWM0->_0_CMPB = 0;
        }
    }

    // Right side (PWM generator 1)
    if(side == 'r')
    {
        if(signed_speed > 0)        // Forward: forward channel on, reverse off
        {
            PWM0->_1_CMPA = g_period - signed_speed;
            PWM0->_1_CMPB = 0;
        }
        else if(signed_speed < 0)   // Reverse: reverse channel on, forward off
        {
            PWM0->_1_CMPA = 0;
            PWM0->_1_CMPB = g_period - abs(signed_speed);
        }
        else                        // Stop: both channels off
        {
            PWM0->_1_CMPA = 0;
            PWM0->_1_CMPB = 0;
        }
    }

}
