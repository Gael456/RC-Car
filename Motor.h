/**
 * @file Motor.h
 *
 * @brief Header file for the Motor driver.
 *
 * This file contains the function definitions for the Stepper_Motor driver. It uses
 * GPIO pins to provide output signals to the ULN2003 stepper motor driver.
 *
 * The following components are used:
 *  -   3V-6V TT Motor
 *  - DRV8833 Motor Driver
 *
 * @author Gael Esparza Lobatos
 */

#include "TM4C123GH6PM.h"
#include <stdlib.h>

/**
 * @brief Initializes GPIO pins required for controlling the stepper motor.
 *
 * @param Configure the necessary GPIO pins as outputs and sets them up as digital
 * functionality to control the motor through the connected motor driver.
 *
 * @return None
 */
void Motor_Init(uint16_t period_constant, uint16_t duty_cycle);


/**
 * @brief Drives both sides of the car from signed speed values
 * 
 * Positive = forward, negative = reverse, zero = stop; the magnitude sets
 * the PWM duty (speed). Turning comes from giving the two sides different
 * or opposite values (e.g. one positive, one negative pivots in place).
 *
 * @param left  Signed speed for the left side (PWM generator 0).
 * @param right Signed speed for the right side (PWM generator 1).
 *
 * @return None
 */
void drive(int16_t left, int16_t right);


/**
 * @brief Sets one side's direction and speed from a signed value.
 *
 * The sign selects direction (forward drives CMPA, reverse drives CMPB),
 * the magnitude sets the PWM duty, and the opposite channel is zeroed.
 * The magnitude is clamped below the period to avoid underflow.
 *
 * @param side         'l' = left (generator 0), 'r' = right (generator 1).
 * @param signed_speed >0 forward, <0 reverse, 0 stop.
 *
 * @return None
 */
void set_side(char side, int16_t signed_speed);
