#include <msp430.h>
#include <stdio.h>
#include <stdlib.h>
#include "peripherals.h"
#include "string.h"


/**
 * main.c
 */

#define CALADC12_15V_30C *((unsigned int *)0x1A1A)
#define CALADC12_15V_85C *((unsigned int *)0x1A1C)

// Function Prototypes
void configButtons(void);

void configUCS(void);
void configTimerA2(void);
__interrupt void Timer_A2_ISR(void);
void stopTimerA2(void);
void enableTimerA2(void);

void displayTime(long unsigned int inTime);
char * monthNumToASCII(int monthNum);
void displayTimeFormat(unsigned int month, unsigned int day, unsigned int hours, unsigned int minutes, unsigned int seconds);

void configTempSensor(void);
void displayTemp(float inAvgTempC);
void sampleTemp(void);
float avgTempC();
void resetAvgTempC(void);


long unsigned int timer;                        // timer count for TimerA2, increased by TimerA2 ISR
long unsigned int initTime;                     // initial time, used for populating the tempC array
                                                // during the first 10 seconds

// Array storing days in each month (assuming non leap year)
const unsigned int monthDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30 ,31 , 30, 31};

unsigned int in_temp;       // holds the result from the conversion, the ADC counts from MEM0

// temperatureDegC stores the temp in degree C from the conversion from ADC counts (in_temp)
// degC_per_bit stores the value of resolution in degC/bit
volatile float temperatureDegC, degC_per_bit;

// Array with 10 indices holds the value of the last 10 temp. readings
float tempC[10];


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    // Useful code starts here
    // Initialization and configuration of LEDs, Display, Keypad, UCS, TimerA2, Push Buttons and Temp Sensor
    initLeds();
    configDisplay();
    configKeypad();

    configButtons();
    configUCS();
    configTimerA2();
    configTempSensor();

    _BIS_SR(GIE);           // enables interrupts


    initTime = 0;                               // initial time set to 0 seconds
    timer = initTime;                           // timer set to initial time
    long unsigned int prevTime = timer - 1;     // prevTime declared and set to timer - 1

    unsigned char state = 0;                    // state of main state machine set to 0
    unsigned char editState = 0;                // state of edit state machine set to 0

    long unsigned int editedTimer = 0;          // editTimer stores the value in seconds of edit time
    long unsigned int editedMonth = 1;          // stores value of edited month
    long unsigned int editedDay = 1;            // stores value of edited day
    long unsigned int editedHour = 0;           // stores value of edited hour
    long unsigned int editedMin = 0;            // stores value of edited minutes
    long unsigned int editedSec = 0;            // stores value of edited seconds

    // Clears display from anything
    Graphics_clearDisplay(&g_sContext);
    Graphics_flushBuffer(&g_sContext);


    // Forever loop
    while (1) {

        // MAIN State Machine to switch between two states
        // Case 0 is sampling temperature readings, and displaying temperature, date and time
        // Case 1 is edit mode, to edit date and time
        switch (state) {

        case 0:

            // If a second has passed, then sample temperature, and display temp, date and time
            if (timer >= (prevTime + 1)) {
                sampleTemp();
                displayTemp(avgTempC());
                displayTime(timer);
                prevTime = timer;           // sets prevTime to current time
            }

            // If RIGHT BUTTON is pressed, it enters into edit mode
            // Before entering edit mode, it stops timer and
            // sets all the edited.... variables to corresponding values
            if ((P1IN & BIT1) == 0) {
                stopTimerA2();
                editedTimer = 0;
                editedMonth = 1;
                editedDay = 1;
                editedHour = 0;
                editedMin = 0;
                editedSec = 0;
                editState = 0;
                Graphics_clearDisplay(&g_sContext);                 // Clears display
                displayTimeFormat(editedMonth, editedDay,
                                  editedHour, editedMin, editedSec); // Display formatted edit time
                Graphics_drawLineH(&g_sContext, 44, 64, 85);        // Underlines the MONTH
                Graphics_flushBuffer(&g_sContext);                  // Refreshes display
                state++;        // state is incremented to enter edit mode
            }

            break;

        case 1:

            // EDIT State Machine to switch between six states
            // Case 0 to edit MONTH, Case 1 to edit DAYS, Case 2 to edit HOURS
            // Case 3 to edit MINUTES, Case 4 to edit SECONDS,
            // Case 5 to set global timer to new value of editedTimer, enable timer and transition back to State 0
            switch (editState) {

            // Configure MONTH
            case 0:

                // If RIGHT BUTTON is pressed, save MONTH and corresponding seconds
                // in editTimer and then increment editState to edit next element
                if ((P1IN & BIT1) == 0) {
                    long unsigned int totalDays = 0;
                    int i;
                    for (i = 1; i < editedMonth; i++) {
                        totalDays += monthDays[i - 1];          // Calculates total days based on chosen MONTH
                    }
                    editedTimer = editedTimer + (totalDays * 24 * 60 * 60); // Calculates seconds based on total days
                                                                            // Stores calculated seconds in editTimer

                    // Clears displays, updates edit time display, underlines DAY element, updates display
                    Graphics_clearDisplay(&g_sContext);
                    displayTimeFormat(editedMonth, editedDay, editedHour, editedMin, editedSec);
                    Graphics_drawLineH(&g_sContext, 64, 84, 85);
                    Graphics_flushBuffer(&g_sContext);

                    __delay_cycles(500000);     // very brief delay which serves as right button debounce delay
                    editState++;                // go to next edit state, to edit DAYS
                }

                // If LEFT BUTTON is pressed, increments MONTH value and editedMonth variable
                // and circles back to first value after max month (i.e. after DEC goes to JAN)
                if ((P2IN & BIT1) == 0) {

                    if (editedMonth < 12) {
                        editedMonth++;
                    } else {
                        editedMonth = 1;
                    }

                    // Clears displays, updates edit time display, underlines MONTH element, updates display
                    Graphics_clearDisplay(&g_sContext);
                    displayTimeFormat(editedMonth, editedDay, editedHour, editedMin, editedSec);
                    Graphics_drawLineH(&g_sContext, 44, 64, 85);
                    Graphics_flushBuffer(&g_sContext);
                }

                break;


            // Configure DAYS
            case 1:

                // If RIGHT BUTTON is pressed, save DAYS and corresponding seconds
                // in editTimer and then increment editState to edit next element
                if ((P1IN & BIT1) == 0) {

                    editedTimer = editedTimer + ((editedDay - 1) * 24 * 60 * 60);

                    // Clears displays, updates edit time display, underlines HOURS element, updates display
                    Graphics_clearDisplay(&g_sContext);
                    displayTimeFormat(editedMonth, editedDay, editedHour, editedMin, editedSec);
                    Graphics_drawLineH(&g_sContext, 40, 50, 95);
                    Graphics_flushBuffer(&g_sContext);

                    __delay_cycles(500000);     // very brief delay which serves as right button debounce delay
                    editState++;
                    break;
                }

                // If LEFT BUTTON is pressed, increments DAYS value and editedDay variable
                // and circles back to first value after max day depending on chosen MONTH
                if ((P2IN & BIT1) == 0) {

                    if (editedDay < monthDays[editedMonth - 1]) {
                        editedDay++;
                    } else {
                        editedDay = 1;
                    }

                    // Clears displays, updates edit time display, underlines DAY element, updates display
                    Graphics_clearDisplay(&g_sContext);
                    displayTimeFormat(editedMonth, editedDay, editedHour, editedMin, editedSec);
                    Graphics_drawLineH(&g_sContext, 64, 84, 85);
                    Graphics_flushBuffer(&g_sContext);
                }

                break;


            // Configure HOURS
            case 2:

                // If RIGHT BUTTON is pressed, save HOURS and corresponding seconds
                // in editTimer and then increment editState to edit next element
                if ((P1IN & BIT1) == 0) {

                    editedTimer = editedTimer + (editedHour * 60 * 60);

                    // Clears displays, updates edit time display, underlines MINUTES element, updates display
                    Graphics_clearDisplay(&g_sContext);
                    displayTimeFormat(editedMonth, editedDay, editedHour, editedMin, editedSec);
                    Graphics_drawLineH(&g_sContext, 58, 68, 95);
                    Graphics_flushBuffer(&g_sContext);

                    __delay_cycles(500000);     // very brief delay which serves as right button debounce delay
                    editState++;
                    break;
                }

                // If LEFT BUTTON is pressed, increments HOURS value and editedHour variable
                // and circles back to first value after max hour (i.e. after 23 goes to 0)
                if ((P2IN & BIT1) == 0) {

                    if (editedHour < 23) {
                        editedHour++;
                    } else {
                        editedHour = 0;
                    }

                    // Clears displays, updates edit time display, underlines HOURS element, updates display
                    Graphics_clearDisplay(&g_sContext);
                    displayTimeFormat(editedMonth, editedDay, editedHour, editedMin, editedSec);
                    Graphics_drawLineH(&g_sContext, 40, 50, 95);
                    Graphics_flushBuffer(&g_sContext);
                }

                break;


            // Configure MINUTES
            case 3:

                // If RIGHT BUTTON is pressed, save MINUTES and corresponding seconds
                // in editTimer and then increment editState to edit next element
                if ((P1IN & BIT1) == 0) {

                    editedTimer += editedMin * 60;

                    // Clears displays, updates edit time display, underlines SECONDS element, updates display
                    Graphics_clearDisplay(&g_sContext);
                    displayTimeFormat(editedMonth, editedDay, editedHour, editedMin, editedSec);
                    Graphics_drawLineH(&g_sContext, 76, 86, 95);
                    Graphics_flushBuffer(&g_sContext);

                    __delay_cycles(500000);     // very brief delay which serves as right button debounce delay
                    editState++;
                    break;
                }

                // If LEFT BUTTON is pressed, increments MINUTES value and editedMin variable
                // and circles back to first value after max minute (i.e. after 59 goes to 0)
                if ((P2IN & BIT1) == 0) {

                    if (editedMin < 59) {
                        editedMin++;
                    } else {
                        editedMin = 0;
                    }

                    // Clears displays, updates edit time display, underlines MINUTES element, updates display
                    Graphics_clearDisplay(&g_sContext);
                    displayTimeFormat(editedMonth, editedDay, editedHour, editedMin, editedSec);
                    Graphics_drawLineH(&g_sContext, 58, 68, 95);
                    Graphics_flushBuffer(&g_sContext);
                }

                break;


            // Configure SECONDS
            case 4:

                // If RIGHT BUTTON is pressed, save SECONDS and corresponding seconds
                // in editTimer and then increment editState to edit next element
                if ((P1IN & BIT1) == 0) {

                    editedTimer += editedSec;
                    editState++;
                    break;
                }

                // If LEFT BUTTON is pressed, increments SECONDS value and editedSec variable
                // and circles back to first value after max second (i.e. after 59 goes to 0)
                if ((P2IN & BIT1) == 0) {

                    if (editedSec < 59) {
                        editedSec++;
                    } else {
                        editedSec = 0;
                    }

                    // Clears displays, updates edit time display, underlines SECONDS element, updates display
                    Graphics_clearDisplay(&g_sContext);
                    displayTimeFormat(editedMonth, editedDay, editedHour, editedMin, editedSec);
                    Graphics_drawLineH(&g_sContext, 76, 86, 95);
                    Graphics_flushBuffer(&g_sContext);
                }

                break;


            // Updates global timer, re-enables timer and goes back to State 0
            case 5:

                initTime = editedTimer;     // set initTime (ie. initial time variable) to editedTimer
                timer = initTime;           // set timer to initTime, new timer based on recent edited settings
                prevTime = timer - 1;
                resetAvgTempC();            // resets (i.e. zeroes) all elements in tempC array
                enableTimerA2();            // re-enables timer interrupts
                state = 0;                  // exit EDIT state and go back to State 0
                break;
            }

        }

    }
}

// Configure LaunchPad buttons for edit settings
void configButtons() {

    P1SEL &= ~(BIT1);           // P1.1 configured as digital I/0
    P2SEL &= ~(BIT1);           // P2.1 configured as digital I/0

    P1DIR &= ~(BIT1);           // P1.1 configured as input
    P2DIR &= ~(BIT1);           // P2.1 configured as input

    P1REN |= (BIT1);            // P1.1, pull u/d resistors enabled
    P2REN |= (BIT1);            // P2.1, pull u/d resistors enabled

    P1OUT |= (BIT1);            // P1.1, pull-up resistor configured
    P2OUT |= (BIT1);            // P2.1, pull-up resistor configured

}

// configures UCS
void configUCS() {
    P5SEL |= (BIT5 | BIT4 | BIT3 |BIT2);    // enables XT1CLK and XT2CLK, both crystal clocks
}

// configures TimerA2 with one second resolution
// including source clk, divider, up mode; also sets Max count to 32767; enables TimerA2 interrupt
void configTimerA2 () {
    TA2CTL = TASSEL_1 + ID_0 + MC_1;        // sets ACLK as TimerA2 source, divider is 1, up mode
    TA2CCR0 = 32767;                          // TA2CCR0 = Max_count = ACLK_ticks - 1 = 32768 - 1 = 32767
    TA2CCTL0 = CCIE;                        // TA2CCR0 interrupt enabled
}

// TimerA2 ISR configuration
// No leap counting needed for timer
#pragma vector=TIMER2_A0_VECTOR
__interrupt void Timer_A2_ISR(void) {
    timer++;
}


// stops timer A2
void stopTimerA2() {
    TA2CTL |= MC_0;         // stop timer, enable stop mode
}

// enables TimerA2 after being stopped
void enableTimerA2() {
    TA2CTL |= MC_1;        // up mode
}

// Function which takes a copy of global time count as its input argument
// Convert the time in seconds that was passed in to Month, Day, Hour, Minutes and Seconds
// Takes above variables and passes them to displayTimeFormat() method that creates ASCII arrays that are displayed
void displayTime(long unsigned int inTime) {

    unsigned int dayNum = 0;
    dayNum = (int)((inTime) / (24.0 * 60.0 * 60.0));
    bool done = 0;
    unsigned int month, day, hours, minutes, seconds;

    // Determines month and day number
    int i;
    for (i = 1; i <= 12 && !done; i++) {

        if (dayNum < monthDays[i - 1]) {
            day = dayNum + 1;
            month = i;
            done = 1;
        } else {
            dayNum = dayNum - monthDays[i - 1];
        }
    }

    // Determines hours, minutes, seconds as an integer
    hours = ((long int)(inTime / (60 * 60))) % 24;
    minutes = ((long int)(inTime / 60)) % 60;
    seconds = inTime % 60;

    displayTimeFormat(month, day, hours, minutes, seconds);

}

// Function which takes Month, Day, Hour, Minutes and Seconds as its input argument
// Create ascii array(s) and displays date and time to the LCD in the format specified
void displayTimeFormat(unsigned int month, unsigned int day, unsigned int hours, unsigned int minutes, unsigned int seconds) {

    // Format array for DATE
    unsigned char dateASCII[7];
    dateASCII[3] = ' ';
    char *monthName = monthNumToASCII(month);

    int i;
    for (i = 0; i < 3; i++) {
        dateASCII[i] = monthName[i];
    }

    dateASCII[4] = (day / 10) + '0';
    dateASCII[5] = (day % 10) + '0';
    dateASCII[6] = '\0';


    // Format array for TIME
    unsigned char timeASCII[9];
    timeASCII[2] = ':';
    timeASCII[5] = ':';

    timeASCII[0] = (hours / 10) + '0';
    timeASCII[1] = (hours % 10) + '0';
    timeASCII[3] = (minutes / 10) + '0';
    timeASCII[4] = (minutes % 10) + '0';
    timeASCII[6] = (seconds / 10) + '0';
    timeASCII[7] = ((seconds % 10) + '0');
    timeASCII[8] = '\0';

    Graphics_drawStringCentered(&g_sContext, dateASCII, AUTO_STRING_LENGTH, 64, 80, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, timeASCII, AUTO_STRING_LENGTH, 64, 90, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
}

// Function which takes Month as an integer number as its input argument
// Based on input number, copies the corresponding Month name as a string (char array)
// Returns string as a char pointer
char * monthNumToASCII(int monthNum) {

    static char monthStr[3];

    switch (monthNum) {

        case 1:
            strcpy(monthStr, "JAN");
            break;
        case 2:
            strcpy(monthStr, "FEB");
            break;
        case 3:
            strcpy(monthStr, "MAR");
            break;
        case 4:
            strcpy(monthStr, "APR");
            break;
        case 5:
            strcpy(monthStr, "MAY");
            break;
        case 6:
            strcpy(monthStr, "JUN");
            break;
        case 7:
            strcpy(monthStr, "JUL");
            break;
        case 8:
            strcpy(monthStr, "AUG");
            break;
        case 9:
            strcpy(monthStr, "SEP");
            break;
        case 10:
            strcpy(monthStr, "OCT");
            break;
        case 11:
            strcpy(monthStr, "NOV");
            break;
        case 12:
            strcpy(monthStr, "DEC");
            break;
        }

    return monthStr;
}

// Function to configure temperature sensor
void configTempSensor() {

    // Temperature sensor calibration
    degC_per_bit = ((float)(85.0 - 30.0)) / ((float)(CALADC12_15V_85C - CALADC12_15V_30C));

    REFCTL0 &= ~REFMSTR;

    // ADC12 Sample and Hold clock cycle is set to 384 cycles
    // ADC12 internal reference voltage is enabled
    // ADC12 ADC12 is turned on
    // Vref = 1.5 V internal
    ADC12CTL0 = ADC12SHT0_9 | ADC12REFON | ADC12ON;

    // ADC12SHP = 1 = SAMPCON signal sourced from sampling timer
    // Enable sample timer
    ADC12CTL1 = ADC12SHP;

    // Memory location 0 is used for input channel A10 (i.e. temp sensor)
    // Reference voltage is set to VREF+
    ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_10;

    __delay_cycles(100);                        // delay to allow Ref to settle
    ADC12CTL0 |= ADC12ENC;                      // Enable conversion

}

// Function which takes (average) temperature in degree C as its input argument
// Calculates (average) temperature in in degree F
// Creates ascii arrays and displays temperature in degree C and F to the LCD in the format specified
void displayTemp(float inAvgTempC) {

    // Temperature in Fahrenheit = (9/5)*Tc + 32
    float inAvgTempF = inAvgTempC * (9.0 / 5.0) + 32.0;

    // Formatting of temperature display array in Celsius
    unsigned char tempC_ASC[8];
    tempC_ASC[3] = '.';
    tempC_ASC[5] = ' ';
    tempC_ASC[6] = 'C';
    tempC_ASC[7] = '\0';

    tempC_ASC[0] = ((int)inAvgTempC / 100.0) + '0';
    tempC_ASC[1] = ((int)(inAvgTempC / 10.0) % 10) + '0';
    tempC_ASC[2] = ((int)inAvgTempC % 10) + '0';
    tempC_ASC[4] = ((int)(inAvgTempC * 10) % 10) + '0';

    // Formatting of temperature display array in Fahrenheit
    unsigned char tempF_ASC[8];
    tempF_ASC[3] = '.';
    tempF_ASC[5] = ' ';
    tempF_ASC[6] = 'F';
    tempF_ASC[7] = '\0';

    tempF_ASC[0] = ((int)inAvgTempF / 100.0) + '0';
    tempF_ASC[1] = ((int)(inAvgTempF / 10.0) % 10) + '0';
    tempF_ASC[2] = ((int)inAvgTempF % 10) + '0';
    tempF_ASC[4] = ((int)(inAvgTempF * 10) % 10) + '0';

    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, tempC_ASC, AUTO_STRING_LENGTH, 64, 50, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, tempF_ASC, AUTO_STRING_LENGTH, 64, 60, TRANSPARENT_TEXT);

}

// Function to sample temperature, and convert and store ADC value to temp in degree C
void sampleTemp() {

    ADC12CTL0 &= ~ADC12SC;  // clear the start bit
    ADC12CTL0 |= ADC12SC;   // Sampling and conversion start
                            // Single conversion (single channel)

    // Poll busy bit waiting for conversion to complete
    while (ADC12CTL1 & ADC12BUSY)
        __no_operation();

    in_temp = ADC12MEM0; // Read results from conversion

    // Convert ADC code to temperature in degree C
    temperatureDegC = (float)(((long)in_temp - CALADC12_15V_30C) * degC_per_bit + 30.0);

    tempC[(timer % 10)] = temperatureDegC;      // store temp in degree C into tempC[] array
                                                // uses circular indexing to store values in the
                                                // index equivalent to timer % 10

    __no_operation(); // SET BREAKPOINT HERE
}

// Function that returns the average temperature in degree C as a float
float avgTempC() {

    float outTempC = 0;

    int i;
    for (i = 0; i < 10; i++)        // Add all values stored in tempC[] array
        outTempC += tempC[i];

    // If not first 10 readings, divide outTempC by 10 (i.e. tempC[] array is full)
    // Else, tempC[] array is not fully populated, so divide by number of elements in it
    if ((timer - initTime) >= 10)
        outTempC /= 10;
    else
        outTempC /= (timer - initTime + 1);


    return outTempC;

}

// Function that resets the tempC[] array, sets all array elements to 0
void resetAvgTempC() {
    int i;
    for (i = 0; i < 10; i++)
        tempC[i] = 0;
}


