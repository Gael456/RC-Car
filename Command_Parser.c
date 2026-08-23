#include "Command_Parser.h"


/**
 * @brief Reads one movement message from UART1 and drives the car.
 *
 * Blocks until a full line arrives (e.g. "-8000,6000"), parses it into two
 * signed side speeds separated by a comma, and passes them to drive(). Each
 * value's sign sets that side's direction; its magnitude sets the speed.
 *
 * @return None
 */
void parser(void)
{
	char buffer[16];
	UART1_Input_String(buffer, 16);   // Receive one line (blocks until CR, null-terminated)

	int i = 0;          // Index into the buffer
	int k = 0;          // Which value is being built: 0 = left, 1 = right
	int16_t left = 0;   // Left side speed (magnitude accumulated here)
	int16_t right = 0;  // Right side speed
	int16_t sign = 1;   // Sign of the value currently being built (+1 or -1)
	int16_t digit;      // Numeric value of the current digit character

	// Walk the received string one character at a time
	while(buffer[i] != '\0')
	{
		// The comma ends the left value: apply its sign, reset the sign,
		// and switch to building the right value.
		if(buffer[i] == ',')
		{
			k = 1;
			left = left * sign;
			sign = 1;
		}

		// Left value: record a minus sign, or accumulate a digit
		if (k == 0)
		{
			if(buffer[i] == '-') sign = -1;
			if(isdigit(buffer[i]))
			{
				digit = buffer[i] - '0';   // Convert the ASCII character to its value
				left = (left * 10) + digit;
			}
		}

		// Right value: same handling, accumulated into right
		if (k == 1)
		{
			if(buffer[i] == '-') sign = -1;
			if(isdigit(buffer[i]))
			{
				digit = buffer[i] - '0';
				right = (right * 10) + digit;
			}
		}

		i++;
	}

	right = right * sign;   // Apply the right value's sign (no comma follows it)

	drive(left, right);     // Send both side speeds to the motor driver
}
