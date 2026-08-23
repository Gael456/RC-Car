/**
 * @file Command_Parser.h
 *
 * @brief Header for the movement command parser.
 *
 * The parser reads a movement message over UART1 and drives the car. A message
 * is two signed, comma-separated side speeds (e.g. "-8000,6000") ending in a
 * carriage return; the sign sets each side's direction and the magnitude its speed.
 *
 * @author Gael Esparza Lobatos
 */

#include <stdbool.h>   // bool type
#include "Motor.h"     // drive() - sends the parsed speeds to the motors
#include "UART1.h"     // UART1_Input_String() - receives the command line
#include <ctype.h>     // isdigit() - used while parsing digits


/**
 * @brief Reads one movement message from UART1 and drives the car.
 *
 * Blocks until a full line is received, parses it into left/right signed
 * speeds, and calls drive().
 *
 * @return None
 */
void parser(void);
