#include <msp430.h>
#include <stdio.h>
#include <stdlib.h>
#include "peripherals.h"
#include "String.h"
#include <pitches.h>
#include <songs.h>


/**
 * main.c
 */

// Function Prototypes
void playSong(int notes[], int durations[], float speed);

void configUCS(void);
void configTimerA2(void);
__interrupt void Timer_A2_ISR(void);
void resetTimer(void);
unsigned int getMS();
void stopTimerA2(void);
void enableTimerA2(void);

void ClearDisplay(void);
void WelcomeDisplay(void);
void SongMenuDisplay(void);
void SettingsDisplay(void);

void GoMainMenu(void);

void SongResetVars(void);

void ledFunction(char inbits);


const unsigned int defaultSpeed = 1500;         // default speed of a whole note is 1500 ms
const unsigned int one_second = 1000;           // one second in ms

int chosenSongSize;                             // stores size of array of song to be played
int songToPlay;                                 // stores song number to be played

int noteNumber = 0;                             // note number to be played, used for arrays storing notes and durations
int note;                                       // intermediate variable stores current note in hz to be played
int noteDuration;                               // intermediate variable stores duration in milliseconds (ms) of current note to be played
bool firstTime = 1;                             // stores whether it is first time playing note

const char led1ON = BIT0;                       // stores BIT0 for LED 1
const char led2ON = BIT1;                       // stores BIT1 for LED 2
const char OFF = 0;                             // used to turn off both LEDs

unsigned int timer = 0;                         // timer count for TimerA2, increased by TimerA2 ISR
unsigned int leapCount = 0;                     // leap count for TimerA2, for ISR
const int resolutionMS = 5;                     // resolution of TimerA2 in milliseconds

bool isPaused;                                  // state of song, isPaused = 0 song not paused, isPaused = 1 song is paused
float speed = 1.0;                              // default playing speed
const float increment = 0.15;                   // const increment of speed to play faster or slower

bool lastButtonState = 0;                       // stores last button state, whether pressed or unpressed
unsigned int lastDebounceTime = 0;              // time since last button press, for debounce
const unsigned int debounceDelay = 75;          // debounce delay in milliseconds


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    int state = 0;                  // state of state machine starts at 0
    int countState = 1;             // state of countdown state machine starts at 1

    unsigned char currKey = 0;

    // Useful code starts here
    // Initialization and configuration of LEDs, Display, Keypad, UCS, TimerA2
    initLeds();
    configDisplay();
    configKeypad();
    configUCS();
    configTimerA2();

    BuzzerOff();
    WelcomeDisplay();       // starts with welcome display

    _BIS_SR(GIE);           // enables interrupts


    // Forever loop
    while (1) {

        // State Machine. Switch statements take value of state and proceeds to Case accordingly
        switch (state) {

        // Waits until '*' is pressed to move to next state
        case 0:
            currKey = getKey();

            // If '#' is pressed, returns to main menu, i.e. state = 0
            if (currKey == '*') {
                ClearDisplay();             // clears display of any previous stuff
                SongMenuDisplay();          // displays song menu
                state++;                    // moves to next state
            }

            break;

        // Prompts user input to determine which song to play
        case 1:
            currKey = getKey();

            // If '1' is pressed, then chooses settings to play song 1
            if (currKey == '1') {
                ClearDisplay();
                SongResetVars();
                countState = 1;                                     // sets countState = 1 for countdown state machine in case 2
                chosenSongSize = sizeof(melody)/sizeof(int);        // sets chosenSongSize to size of array for song 1
                songToPlay = 1;                                     // sets songToPlay to song 1
                resetTimer();
                state++;
            }

            // If '2' is pressed, then chooses settings to play song 2
            else if (currKey == '2') {
                ClearDisplay();
                SongResetVars();
                countState = 1;                                     // sets countState = 1 for countdown state machine in case 2
                chosenSongSize = sizeof(melody2)/sizeof(int);       // sets chosenSongSize to size of array for song 2
                songToPlay = 2;                                     // sets songToPlay to song 2
                resetTimer();
                state++;
            }

            // If '#' is pressed, returns to main menu, i.e. state = 0
            if (currKey == '#') {
                GoMainMenu();
                state = 0;
            }
            break;

        // Triggers countdown through use of countdown state machine
        case 2:

            // COUNTDOWN State machine
            switch (countState) {


                    // If countState = 1, turn LED 1 ON and display '3' for one second
                    case 1:

                        ledFunction(led1ON);
                        Graphics_drawStringCentered(&g_sContext, "3", AUTO_STRING_LENGTH, 64, 64, TRANSPARENT_TEXT);
                        Graphics_flushBuffer(&g_sContext);

                        // Checks for one second to pass, then clears display, resets timer and countState++
                        if(getMS() >= one_second) {
                            ClearDisplay();
                            resetTimer();
                            countState++;
                        }

                        break;

                    // If countState = 2, turn LED 2 ON and display '2' for one second
                    case 2:

                        ledFunction(led2ON);
                        Graphics_drawStringCentered(&g_sContext, "2", AUTO_STRING_LENGTH, 64, 64, TRANSPARENT_TEXT);
                        Graphics_flushBuffer(&g_sContext);

                        // Checks for one second to pass, then clears display, resets timer and countState++
                        if(getMS() >= one_second) {
                            ClearDisplay();
                            resetTimer();
                            countState++;
                        }

                        break;

                    // If countState = 3, turn LED 1 ON and display '1' for one second
                    case 3:

                        ledFunction(led1ON);
                        Graphics_drawStringCentered(&g_sContext, "1", AUTO_STRING_LENGTH, 64, 64, TRANSPARENT_TEXT);
                        Graphics_flushBuffer(&g_sContext);

                        // Checks for one second to pass, then clears display, resets timer and countState++
                        if(getMS() >= one_second) {
                            ClearDisplay();
                            resetTimer();
                            countState++;
                        }

                        break;

                    // If countState = 4, turn LED 1 and LED 2 ON and display 'GO' for one second
                    case 4:

                        ledFunction(led1ON | led2ON);
                        Graphics_drawStringCentered(&g_sContext, "GO", AUTO_STRING_LENGTH, 64, 64, TRANSPARENT_TEXT);
                        Graphics_flushBuffer(&g_sContext);

                        // Checks for one second to pass, then clears display, resets timer and state++ for general (not countdown) state machine
                        if(getMS() >= one_second) {
                            ClearDisplay();
                            ledFunction(OFF);
                            state++;
                        }

                        break;

            }

            currKey = getKey();

            // If '#' is pressed, returns to main menu, i.e. state = 0
            if (currKey == '#') {
                GoMainMenu();
                state = 0;
            }

            break;

        // Display song settings (i.e. play/pause, faster, slower, return), resets timer, goes next state
        case 3:
            SettingsDisplay();                      // Display song settings (i.e. play/pause, faster, slower, return)
            resetTimer();                           // resets TimerA2
            lastButtonState = 0;                    // sets lastButtonState = 0, i.e. button has not been pressed
            state++;
            break;

        // Plays song note, checks to see if song has not finished
        case 4:
            ledFunction(led2ON);                    // turns green LED on to indiciate song is playing

            // If note number is bigger or equal to the array of song chosen to be played, then go back to main menu
            if (noteNumber >= chosenSongSize) {
                GoMainMenu();
                state = 0;
                break;
            }

            // Determines which song to play based on songToPlay, which was determined in Case 1
            if (songToPlay == 1)
                playSong(melody, tempo, speed);         // Calls playSong() with melody, tempo, speed as parameters, i.e. play song 1 at speed
            else if (songToPlay == 2)
                playSong(melody2, tempo2, speed);       // Calls playSong() with melody2, tempo2, speed as parameters, i.e. play song 2 at speed

            currKey = getKey();

            // If '#' is pressed, returns to main menu, i.e. state = 0
            if (currKey == '#') {
                GoMainMenu();
                state = 0;
            }
            else {
                state++;
            }
            break;

        // Extra case, just skips it
        case 5:
            state++;
            break;

        // Receives user input for Song Settings menu and determines whether to play/pause, faster, slower or return to song options menu
        case 6:
            currKey = getKey();
            int currKeyInt = currKey - '0';

            switch (currKeyInt) {

            // If user input is '1', it either pauses or plays song, depending on current state of song
            case 1:

                // Sets lastDebounceTime to current time in ms if lastButtonState = 0, not pressed
                if (!lastButtonState) {
                    lastDebounceTime = getMS();         // Sets lastDebounceTime to current time in ms
                    lastButtonState = 1;                // Sets lastButtonState = 1, i.e. button has been pressed
                }

                if ((getMS() - lastDebounceTime) > debounceDelay) {     // Checks for time debounceDelay (75 ms) to have passed
                    if (!isPaused) {                // if song is not currently paused
                        isPaused = 1;               // set isPaused = 1, i.e. song is now paused
                        ledFunction(OFF);           // turn off all LEDs
                        ledFunction(led1ON);        // turn on red LED ON
                        stopTimerA2();              // stop TimerA2
                        BuzzerOff();                // turn buzzer off, paused buzzer
                    }
                    else if (isPaused) {            // is song is currently paused
                        isPaused = 0;               // set isPaused = 0, song is now not paused
                        enableTimerA2();            // enables TimerA2 (which was stopped)
                        lastButtonState = 0;        // sets lastButtonState = 0, i.e. button is not pressed
                        state = 4;                  // goes back to Case/State 4
                    }
                    lastButtonState = 0;            // sets lastButtonState = 0, i.e. button is not pressed
                }
                break;

            // If user input is '1', it either pauses or plays song, depending on state of song
            case 2:
                // Sets lastDebounceTime to current time in ms if lastButtonState = 0, not pressed
                if (!lastButtonState) {
                    lastDebounceTime = getMS();
                    lastButtonState = 1;
                }

                if ((getMS() - lastDebounceTime) > debounceDelay) {     // debounce for button
                    speed = speed + increment;      // increments speed by 'increment' interval
                    lastButtonState = 0;
                }
                break;

            // If user input is '1', it either pauses or plays song, depending on state of song
            case 3:
                // Sets lastDebounceTime to current time in ms if lastButtonState = 0, not pressed
                if (!lastButtonState) {
                    lastDebounceTime = getMS();
                    lastButtonState = 1;
                }

                if ((getMS() - lastDebounceTime) > debounceDelay) {     // debounce for button
                    speed = speed - increment;      // decrements speed by 'increment' interval
                    lastButtonState = 0;
                }
                break;

            // If user input is '1', it either pauses or plays song, depending on state of song
            case 4:
                BuzzerOff();
                ClearDisplay();
                SongMenuDisplay();
                ledFunction(OFF);
                state = 1;
                break;

            // If no user input, if not paused, go back to State 4
            default:
                if (!isPaused)      // if song is not currently paused
                    state = 4;      // go back to Case 4
                break;
            }

            break;
        }
    }
}


void playSong(int notes[], int durations[], float speed) {
    note = notes[noteNumber];
    noteDuration = defaultSpeed / (durations[noteNumber] * speed);      // noteduraiton in ms

    if (firstTime) {           //this if-statement ensures that the note is only played once
      BuzzerOnFreq(note);
      firstTime = 0;
    } else if (getMS() > (noteDuration)) {    //songTimer reset and noteNumber is increased to play next note
      resetTimer();
      noteNumber = noteNumber + 1;          // increments noteNumber to play next note
      firstTime = 1;
    }
}


// configures UCS
void configUCS() {
    P5SEL |= (BIT5 | BIT4 | BIT3 |BIT2);    // enables XT1CLK and XT2CLK, both crystal clocks
}

// configures TimerA2, including source clk, divider, up mode; also sets Max count to 164; enables TimerA2 interrupt
void configTimerA2 () {
    TA2CTL = TASSEL_1 + ID_0 + MC_1;         // sets ACLK as TimerA2 source, divider is 1, up mode
    TA2CCR0 = 163;                         // TA2CCR0 = MaxCount - 1 = 164 - 1 = 163
    TA2CCTL0 = CCIE;                        // TA2CCR0 interrupt enabled
}

// TimerA2 ISR configuration
// Leap counting implemented for timer
#pragma vector=TIMER2_A0_VECTOR
__interrupt void Timer_A2_ISR(void) {

    if (leapCount < 1024) {     // after 1024 interrupts, timer is off by resolution, i.e. 0.005 seconds
        timer++;
        leapCount++;
    }
    else {
        timer = timer + 2;      // increase timer by 2 at 1024 interrupt count
        leapCount = 0;          // leap count is reset to 0 at 1024 interrupt count
    }
}

// resets TimerA2
void resetTimer() {
    timer = 0;
    leapCount = 0;
}

// returns time in milliseconds
unsigned int getMS() {
    unsigned int ms = timer * resolutionMS;     // milliseconds is current imer count * 5 ms, resolution of timer in ms

    return ms;
}

// stops timer A2
void stopTimerA2() {
    TA2CTL |= MC_0;         // stop timer, enable stop mode
}

// enables TimerA2 after being stopped
void enableTimerA2() {
    TA2CTL |= MC_1;        // up mode
}


void ClearDisplay() {
    Graphics_clearDisplay(&g_sContext);
    Graphics_flushBuffer(&g_sContext);
}

void WelcomeDisplay() {
    Graphics_drawStringCentered(&g_sContext, "MUSIC PLAYER", AUTO_STRING_LENGTH, 64, 50, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "Go to Menu", AUTO_STRING_LENGTH, 64, 70, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "Press *", AUTO_STRING_LENGTH, 64, 80, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
}

void SongMenuDisplay() {
    Graphics_drawStringCentered(&g_sContext, "CHOOSE SONG", AUTO_STRING_LENGTH, 64, 50, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "1. Mario Theme Song", AUTO_STRING_LENGTH, 64, 70, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "2. HBD Song", AUTO_STRING_LENGTH, 64, 80, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
}

void SettingsDisplay() {
    Graphics_drawStringCentered(&g_sContext, "CHOOSE SETTING", AUTO_STRING_LENGTH, 64, 50, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "1. Pause/Play", AUTO_STRING_LENGTH, 64, 70, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "2. Play faster", AUTO_STRING_LENGTH, 64, 80, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "3. Play slower", AUTO_STRING_LENGTH, 64, 90, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "4. Return", AUTO_STRING_LENGTH, 64, 100, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
}

void GoMainMenu() {
    BuzzerOff();
    ClearDisplay();
    ledFunction(OFF);
    WelcomeDisplay();
}

void SongResetVars() {
    firstTime = 1;
    noteNumber = 0;
    isPaused = 0;
    speed = 1.0;
}

// Function that turns LED 1 or LED 2 on (and off)
// based on parameter passed
void ledFunction(char inbits) {

    switch(inbits) {

    case 0:
        P1OUT &= ~BIT0;        // logic 0 sets LED 1 off
        P4OUT &= ~BIT7;        // logic 0 sets LED 2 off
        break;

    case 1:
        P1OUT |= BIT0;         // logic 1 sets LED 1 on
        P4OUT &= ~BIT7;        // logic 0 sets LED 2 off
        break;

    case 2:
        P4OUT |= BIT7;         // logic 1 sets LED 2 on
        P1OUT &= ~BIT0;        // logic 1 sets LED 1 off
        break;

    case 3:
        P1OUT |= BIT0;         // logic 1 sets LED 1 on
        P4OUT |= BIT7;         // logic 1 sets LED 2 on
        break;

    }

}
