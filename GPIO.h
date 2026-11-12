#include "TM4C123GH6PM.h"

// Constant definitions for the user LED (RGB) colors
extern const uint8_t RGB_LED_OFF;
extern const uint8_t RGB_LED_RED;
extern const uint8_t RGB_LED_BLUE;
extern const uint8_t RGB_LED_GREEN;

/**
 * @brief The RGB_LED_Init function initializes the RGB LED (PF1 - PF3)
 *
 * This function initializes the following RGB LED pins, configures the digital functionality for the pins,
 * and sets the direction of the pins as output. The RGB LED is off by default upon initialization.
 *  - LED_R     (PF1)
 *  - LED_B     (PF2)
 *  - LED_G     (PF3)
 *
 * @param None
 *
 * @return None
 */
void RGB_LED_Init(void);

/**
 * @brief The RGB_LED_Output function sets the output of the RGB LED.
 *
 * This function sets the output of the RGB LED based on the value of the input, led_value.
 * A bitwise AND operation (& 0xF1) is performed to mask the Bits 1 to 3 of the GPIOF's DATA register
 * to preserve the state of other pins connected to Port F while keeping the RGB LED pins unaffected.
 * Then, a bitwise OR operation is performed with led_value to set the RGB LED pins to the desired state
 * specified by led_value.
 *
 * @param led_value An 8-bit unsigned integer that determines the output of the RGB LED. To turn off
 *                  the RGB LED, set led_value to 0. The following values determine the color of the RGB LED:
 *
 *  Color       LED(s)   led_value
 *  Off         ---         0x00
 *  Red         R--         0x02
 *  Blue        -B-         0x04
 *  Green      	--G         0x08
 *
 * @return None
 */
void RGB_LED_Output(uint8_t led_value);

/**
 * @brief The RGB_LED_Status function indicates the status of the RGB LED
 * located at pins PF1, PF2, and PF3.
 *
 * @param None
 *
 * @return uint8_t The value representing the status of the RGB LED.
 *
 *  Color       LED(s)   led_value
 *  Off         ---         0x00
 *  Red         R--         0x02
 *  Blue        -B-         0x04
 *  Green       --G         0x08
 *
 */
uint8_t RGB_LED_Status(void);