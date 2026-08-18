/**
 * @file UART1.c
 *
 * @brief Source code for the UART1 driver.
 *
 * This file contains the function definitions for the UART1 driver.
 *
 * @note For more information regarding the UART module, refer to the
 * Universal Asynchronous Receivers / Transmitters (UARTs) section
 * of the TM4C123GH6PM Microcontroller Datasheet.
 *   - Link: https://www.ti.com/lit/gpn/TM4C123GH6PM
 *
 * @note Assumes that the frequency of the system clock is 50 MHz.
 *
 * @author Gael Esparza Lobatos
 */

#include "UART1.h"

void UART1_Init(void)
{
	// Enable the clock to the UART1 module by setting the
  // R0 bit (Bit 0) in the RCGCUART register
  SYSCTL->RCGCUART |= 0x02;

  // Enable the clock to Port B by setting the
  // R0 bit (Bit 0) in the RCGCGPIO register
  SYSCTL->RCGCGPIO |= 0x02;

  // Disable the UART1 module before configuration by clearing
  // the UARTEN bit (Bit 0) in the CTL register
  UART1->CTL &= ~0x0001;

  // Configure the UART1 module to use the system clock (50 MHz)
  // divided by 16 by clearing the HSE bit (Bit 5) in the CTL register
  UART1->CTL &= ~0x0020;

  // Set the baud rate by writing to the DIVINT field (Bits 15 to 0)
  // and the DIVFRAC field (Bits 5 to 0) in the IBRD and FBRD registers, respectively.
  // The integer part of the calculated constant will be written to the IBRD register,
  // while the fractional part will be written to the FBRD register.
  // BRD = (System Clock Frequency) / (16 * Baud Rate)
  // BRD = (50,000,000) / (16 * 115200) = 27.12673611 (IBRD = 27)
  // BRDF = ((0.12673611 * 64) + 0.5) = 8.611 (FBRD = 8)
  UART1->IBRD = 27;
  UART1->FBRD = 8;

  // Write the rest of the initialization function below
  // Refer to the Pre-Lab section
	UART1->LCRH |= 0x60;
	
	UART1->LCRH |= 0x10;
	
	UART1->LCRH &= ~0x08;
	
	UART1->LCRH &= ~0x02;
	
	UART1->CTL |= 0x01;
	
	GPIOB->AFSEL |= 0x03;
	
	GPIOB->PCTL &= ~0x000000FF;
	
	GPIOB->PCTL |= 0x00000010;
	
	GPIOB->PCTL |= 0x00000001;
	
	GPIOB->DEN |= 0x03;
}

char UART1_Input_Character(void)
{	
	while((UART1->FR & UART1_RECEIVE_FIFO_EMPTY_BIT_MASK) != 0);
	
	return (char) (UART1->DR & 0xFF);
}

void UART1_Output_Character(char data)
{
	while((UART1->FR & UART1_TRANSMIT_FIFO_FULL_BIT_MASK) != 0);
	
	UART1->DR = data;
}

void UART1_Input_String(char *buffer_pointer, uint16_t buffer_size) 
{
	int length = 0;
	char character = UART1_Input_Character();
	
	while (character != UART1_CR)
	{
		if(character == UART1_BS)
		{
			if(length)
			{
				buffer_pointer--;
				length--;
				UART1_Output_Character(UART1_BS);
			}
		}
		else if(length < buffer_size)
		{
			*buffer_pointer = character;
			buffer_pointer++;
			length++;
			UART1_Output_Character(character);
		}
		character = UART1_Input_Character();
	}
	*buffer_pointer = 0;
}

void UART1_Output_String(char *pt)
{
	while(*pt)
	{
		UART1_Output_Character(*pt);
		pt++;
	}
}

uint32_t UART1_Input_Unsigned_Decimal(void)
{
	uint32_t number = 0;
	uint32_t length = 0;
	char character = UART1_Input_Character();
	
	// Accepts until <enter> is typed
	// The next line checks that the input is a digit, 0-9.
	// If the character is not 0-9, it is ignored and not echoed
	while (character != UART1_CR)
	{
		if ((character >= '0') && (character <= '9'))
		{
			// "number' will overflow if it is above 4,294,967,295
			number = (10 * number) + (character - '0');
			length++;
			UART1_Output_Character(character);
		}
		
		// If the input is a backspace, then the return number is
		// changed and a backspace is outputted to the screen
		else if ((character == UART1_BS) && length)
		{
			number /= 10;
			length--;
			UART1_Output_Character(character);
		}
		character = UART1_Input_Character();
	}
	
	return number;
}

void UART1_Output_Unsigned_Decimal(uint32_t n)
{
	// Use recursion to convert decimal number
	// of unspecified length as an ASCII string
	if (n >= 10)
	{
		UART1_Output_Unsigned_Decimal(n / 10);
		n = n % 10;
	}
	
	// n iis between 0 and 9
	UART1_Output_Character(n + '0');
}

uint32_t UART1_Input_Unsigned_Hexadecimal(void)
{
	uint32_t number = 0;
  uint32_t digit = 0;
  uint32_t length = 0;
  char character = UART1_Input_Character();

  while(character != UART1_CR)
  {
		// Initialize digit and assume that the hexadecimal character is invalid
    digit = 0x10;

    if ((character >= '0') && (character <= '9'))
    {
			digit = character - '0';
    }
    else if ((character >= 'A') && (character <= 'F'))
    {
      digit = (character - 'A') + 0xA;
    }
    else if ((character >= 'a') && (character <= 'f'))
    {
      digit = (character - 'a') + 0xA;
    }

    // If the character is not 0-9 or A-F, it is ignored and not echoed
    if (digit <= 0xF)
    {
      number = (number * 0x10) + digit;
      length++;
      UART1_Output_Character(character);
    }
    // Backspace outputted and return value changed if a backspace is inputted
    else if ((character == UART1_BS) && length)
    {
      number /= 0x10;
      length--;
      UART1_Output_Character(character);
    }

    character = UART1_Input_Character();
	}

	return number;
}

void UART1_Output_Unsigned_Hexadecimal(uint32_t number)
{
	// Use recursion to covert the umber of
	// unspecified length as an ASCII string
	if (number >= 0x10)
	{
		UART1_Output_Unsigned_Hexadecimal(number / 0x10);
		UART1_Output_Unsigned_Hexadecimal(number % 0x10);
	}
	else
	{
		if (number < 0xA)
		{
			UART1_Output_Character(number + '0');
		}
		else
		{
			UART1_Output_Character((number - 0x0A) +'A');
		}
	}
}

void UART1_Output_Newline(void)
{
	UART1_Output_Character(UART1_CR);
	UART1_Output_Character(UART1_LF);
}
