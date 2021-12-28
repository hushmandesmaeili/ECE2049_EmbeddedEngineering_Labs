/************** ECE2049 DEMO CODE ******************/
/**************  28 March 2020   ******************/
/*********Redesigned by Hamayel Qureshi********/
/***************************************************/

#include <msp430.h>

/* Peripherals.c and .h are where the functions that implement
 * the LEDs and keypad, etc are. It is often useful to organize
 * your code by putting like functions together in files.
 * You include the header associated with that file(s)
 * into the main file of your project. */
#include "peripherals.h"
#include "stdio.h"

// Function Prototypes
void swDelay(char numLoops);

// Main
void main(void)

{
    unsigned char currKey=0, dispSz = 3;
    unsigned char dispThree[3];

    // Define some local variables
    float a_flt = 190.68;
    int  test = 0x0600, i=0;     // In C prefix 0x means the number that follows is in hex
    long unsigned X= 123456;    // No prefix so number is assumed to be in decimal
    unsigned char myGrade='A';
    unsigned char initial='S';
    unsigned char your_name[18] = "Hushmand Esmaeili";
                                    // What happens when you change the array length?
                                    // What should it be? Do you need null terminator /n ?


    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer. Always need to stop this!!
                                 // You can then configure it properly, if desired

    // Some utterly useless instructions -- Step through them
    // What size does the Code Composer MSP430 Compiler use for the
    // following variable types? A float, an int, a long integer and a char?
    a_flt = a_flt*test;
    X = test+X;
    test = test-myGrade;    // A number minus a letter?? What's actually going on here?
                            // What value stored in myGrade (i.e. what's the ASCII code for "A")?
                            // Thus, what is the new value of test? Explain?


    // Useful code starts here
    initLeds();
    configDisplay();
    configKeypad();

    // *** Intro Screen ***
    Graphics_clearDisplay(&g_sContext); // Clear the display

    // Write some text to the display
    Graphics_drawStringCentered(&g_sContext, "Welcome", AUTO_STRING_LENGTH, 64, 50, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "to", AUTO_STRING_LENGTH, 64, 60, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "ECE2049-C21!", AUTO_STRING_LENGTH, 64, 70, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, your_name, AUTO_STRING_LENGTH, 64, 80, TRANSPARENT_TEXT);

    // Draw a box around everything because it looks nice
    Graphics_Rectangle box = {.xMin = 3, .xMax = 125, .yMin = 3, .yMax = 125 };
    Graphics_drawRectangle(&g_sContext, &box);

    // We are now done writing to the display.  However, if we stopped here, we would not
    // see any changes on the actual LCD.  This is because we need to send our changes
    // to the LCD, which then refreshes the display.
    // Since this is a slow operation, it is best to refresh (or "flush") only after
    // we are done drawing everything we need.
    Graphics_flushBuffer(&g_sContext);

    dispThree[0] = ' ';
    dispThree[2] = ' ';

    while (1)    // Forever loop
    {
        // Check if any keys have been pressed on the 3x4 keypad
        currKey = getKey();

        if (currKey == '*')
        {
            BuzzerOn();
            P1OUT |= BIT0;      // Set the P1.0 as 1 (High)
        }

        if (currKey == '#')
        {
            BuzzerOff();
            P4OUT |= BIT7;      // Set the P4.7 as 1 (High)
        }

        if (currKey)
        {
            dispThree[1] = currKey;
            // Draw the new character to the display
            Graphics_drawStringCentered(&g_sContext, dispThree, dispSz, 64, 90, OPAQUE_TEXT);

            // Refresh the display so it shows the new data
            Graphics_flushBuffer(&g_sContext);

            // wait awhile before clearing LEDs
            swDelay(1);
            setLeds(0);
        }

    }  // end while (1)
}


void swDelay(char numLoops)
{
	// This function is a software delay. It performs
	// useless loops to waste a bit of time
	//
	// Input: numLoops = number of delay loops to execute
	// Output: none
	//
	// smj, ECE2049, 25 Aug 2013
    // hamayel qureshi, 28 march 2020

	volatile unsigned int i,j;	// volatile to prevent removal in optimization
			                    // by compiler. Functionally this is useless code

	for (j=0; j<numLoops; j++)
    {
    	i = 50000 ;					// SW Delay
   	    while (i > 0)				// could also have used while (i)
	       i--;
    }
}
