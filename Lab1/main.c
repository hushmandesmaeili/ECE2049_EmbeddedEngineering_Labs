#include <msp430.h>
#include <stdio.h>
#include <stdlib.h>
#include "peripherals.h"
#include "String.h"


/**
 * main.c
 */

// Function Prototypes
void swDelay(char numLoops);
void PlaySequence(int seq_turn, int speed);
void DisplayCountDown();
void DisplaySoundNumber(int number, int speed);
void DisplayNumber(int number);
void DisplayNumCentered(int number);
void PlaySound(int number);
void CheckInput(int speed);
void ClearDisplay(void);
void SimonDisplay(void);
void GameOverDisplay(void);
void WonDisplay(void);

#define MAX 10          // max number of number in random sequence, i.e. numbers to guess, also max rounds to play
#define MAX_NOTES 6     // number of notes played in Game Over sequence. to be used for array
#define OFF 0           // default value to be used to turn off Buzzer, when PWM = 0

int sequence[MAX];      //
int sequence_turn = 0;  // stores current round number of displaying numbers or of inputting numbers, goes up to MAX - 1 = 9
bool flag = 0;          // variable to determine whether user input is correct, if correct then flag = 0, otherwise flag = 1
int count = 0;          // keeps track of the number of times user has inputted number for multiple inputs

// Array storing PWM values for Game Over tune.
int gameOverPitches[MAX_NOTES] = {64, 56, 52, 48, 68, 68};  // little song notes to be played in Game Over sequence

// Constant integers of PWM values for corresponding/associated number
const int SOUND_1 = 128;    // SOUND_1 is sound to be used for number '1'
const int SOUND_2 = 96;     // SOUND_2 is sound to be used for number '2'
const int SOUND_3 = 64;     // SOUND_3 is sound to be used for number '3'
const int SOUND_4 = 32;     // SOUND_4 is sound to be used for number '4'

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    int state = 0;          // state of state machine starts at 0
    int speed = 10;         // 10 is the lowest speed and 1 is the highest speed to display number

    unsigned char currKey = 0;

    // Useful code starts here
    initLeds();
    configDisplay();
    configKeypad();

    // Constants for lower and upper range of numbers to be generated
    const int lower = 1, upper = 4;

    // Loop to populate sequence array with random integers between lower and upper, i.e. between 1-4
    int i;
    for (i = 0; i < 10; i++) {

        sequence[i] = (rand() % (upper - lower + 1)) + lower;   // random integers between lower and upper stored in array index
    }

    // Forever loop
    while (1) {

        // State Machine. Switch statements take value of state and proceeds to Case accordingly
        switch (state) {

        // state 0 calls SimonDisplay() method and then does state++, i.e. state = 1, goes to next state
        case 0:
            SimonDisplay();
            state++;
            break;

        // state 1 waits until user inputs '*' by using method getKey()
        // if currKey == '*', i.e. if user presses '*' then the SIMON screen is cleared,
        // sequence_turn is set to 0 (round 0), flag = 0 (not incorrect user input),
        // speed = 10 (slowest speed), and finally state++, i.e. goes to state = 2
        case 1:
            currKey = getKey();
            if (currKey == '*') {
                ClearDisplay();
                sequence_turn = 0;
                flag = 0;
                speed = 10;
                state++;
            }
            break;

        // Calls method to display countdown before game begins, then state++, i.e.  state = 3
        case 2:
            DisplayCountDown();
            state++;
            break;

        // Calls PlaySequence() based on sequence_turn and speed, sets count = 0 which is used in next state
        // Then goes to next state, i.e. state = 4
        case 3:
            PlaySequence(sequence_turn, speed);
            count = 0;
            state++;
            break;

        // Calls CheckInput() method to check user input, if flag is triggered, i.e. flag == 1, then
        // state = 5, i.e. Game Over state; if sequence_turn == (MAX - 1), i.e. we reached end of game then
        // state = 6, i.e. You Won state; otherwise, ClearDisplay() sequence_turn++ (increase round number),
        // speed--, i.e. the actual speed is increased (goes faster), and state = 3, i.e. back to PlaySequence() state
        case 4:
            CheckInput(speed);

            if (flag == 1) {
                ClearDisplay();
                state = 5;
            }
            else if (sequence_turn == (MAX - 1)) {
                state = 6;
            }
            else {
                ClearDisplay();
                sequence_turn++;
                speed--;
                state = 3;
            }
            break;

        // Calls GameOverDisplay() to display end of game and humiliation message, then back to state = 0, i.e. initial state
        case 5:
            GameOverDisplay();
            state = 0;
            break;

        // Calls YouWon() to display You Won! message, then back to state = 0, i.e. initial state
        case 6:
            WonDisplay();
            state = 0;
            break;
        }
    }
}

void PlaySequence(int seq_turn, int speed) {

    int x;
    for (x = 0; x < (seq_turn + 1); x++) {

        int num = sequence[x];
        DisplaySoundNumber(num, speed);
    }

}

void DisplayCountDown() {
    int x;
    for (x = 3; x >= 0; x--) {
        DisplayNumCentered(x);
        swDelay(10);
        ClearDisplay();
    }
}

void DisplaySoundNumber(int number, int speed) {
    DisplayNumber(number);
    PlaySound(number);
    swDelay(speed);
    Graphics_clearDisplay(&g_sContext);
    PlaySound(OFF);       // no sound, pwm = 0
    swDelay(1);
}

// Displays a number based on the int parameter that is passed
// Uses an equation to determine where on the display the number
// will be located
void DisplayNumber(int number) {

    int location = number * 32 - 16;    // Determines where the number to be displayed will be located
                                        // Should display numbers equally spaced, e.g. |  1  2  3  4  |

    switch (number) {

    case 1:
        Graphics_drawStringCentered(&g_sContext, "1", AUTO_STRING_LENGTH, location, 70, TRANSPARENT_TEXT);
        break;
    case 2:
        Graphics_drawStringCentered(&g_sContext, "2", AUTO_STRING_LENGTH, location, 70, TRANSPARENT_TEXT);
        break;
    case 3:
        Graphics_drawStringCentered(&g_sContext, "3", AUTO_STRING_LENGTH, location, 70, TRANSPARENT_TEXT);
        break;
    case 4:
        Graphics_drawStringCentered(&g_sContext, "4", AUTO_STRING_LENGTH, location, 70, TRANSPARENT_TEXT);
        break;
    default:
        Graphics_clearDisplay(&g_sContext);
        break;
    }

    Graphics_flushBuffer(&g_sContext);
}

// Displays a centered number based on the int parameter that is passed
// This is used for countdown sequence
void DisplayNumCentered(int number) {

    switch (number) {
        case 0:
            Graphics_drawStringCentered(&g_sContext, "0", AUTO_STRING_LENGTH, 64, 70, TRANSPARENT_TEXT);
            break;
        case 1:
            Graphics_drawStringCentered(&g_sContext, "1", AUTO_STRING_LENGTH, 64, 70, TRANSPARENT_TEXT);
            break;
        case 2:
            Graphics_drawStringCentered(&g_sContext, "2", AUTO_STRING_LENGTH, 64, 70, TRANSPARENT_TEXT);
            break;
        case 3:
            Graphics_drawStringCentered(&g_sContext, "3", AUTO_STRING_LENGTH, 64, 70, TRANSPARENT_TEXT);
            break;
        default:
            Graphics_clearDisplay(&g_sContext);
            break;
        }

        Graphics_flushBuffer(&g_sContext);
}

// This method plays a sound depending on the parameter
// It takes an int number as parameter and is used to play a sound
// with an specific pitch for each number from 1-4
// If any other number that is not between 1-4,
// then it plays the buzzer at a PWM equal to number.
void PlaySound(int number) {

    switch (number) {
        case 1:
            BuzzerOnP(SOUND_1);     // PWM = SOUND_1
            break;
        case 2:
            BuzzerOnP(SOUND_2);     // PWM = SOUND_2
            break;
        case 3:
            BuzzerOnP(SOUND_3);     // PWM = SOUND_3
            break;
        case 4:
            BuzzerOnP(SOUND_4);     // PWM = SOUND_4
            break;
        default:
            BuzzerOnP(number);      // PWM = number
            break;
        }
}

void CheckInput(int speed) {

    unsigned char currKey;

    while (!flag & (count <= sequence_turn)) {

        currKey = getKey();
        int currKeyInt = currKey - '0';

        if (currKeyInt < 5 & currKeyInt > 0) {

            DisplaySoundNumber(currKeyInt, speed);

            if (currKeyInt == sequence[count]) {
                ClearDisplay();
                count++;
            }
            else {
                flag = 1;
            }
        } else if (currKey != 0) {              // default value that getKey() returns is 0, so this is checking to see if
            flag = 1;                           // a key other than 1-4 is pressed and NOT default return of 0 which is when nothing
        }                                       // is pressed. if that is the case, then flag is raised and it is GAME OVER
    }
}


void swDelay(char numLoops) {
    // This function is a software delay. It performs
    // useless loops to waste a bit of time
    //
    // Input: numLoops = number of delay loops to execute
    // Output: none
    //
    // smj, ECE2049, 25 Aug 2013
    // hamayel qureshi, 28 march 2020

    volatile unsigned int i,j;  // volatile to prevent removal in optimization
                                // by compiler. Functionally this is useless code

    for (j = 0; j < numLoops; j++) {

        i = 5000 ;                 // SW Delay
        while (i > 0)               // could also have used while (i)
           i--;
    }
}


void ClearDisplay() {

    Graphics_clearDisplay(&g_sContext);
    Graphics_flushBuffer(&g_sContext);
}


void SimonDisplay() {

    Graphics_drawStringCentered(&g_sContext, "SIMON", AUTO_STRING_LENGTH, 64, 60, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "Start Game", AUTO_STRING_LENGTH, 64, 80, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "Press *", AUTO_STRING_LENGTH, 64, 90, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
}


void GameOverDisplay(void) {

    Graphics_drawStringCentered(&g_sContext, "GAME OVER!", AUTO_STRING_LENGTH, 64, 70, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "YOU LOST!", AUTO_STRING_LENGTH, 64, 80, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);

    int x;
    for (x = 0; x < MAX_NOTES; x++) {
        PlaySound(gameOverPitches[x]);
        swDelay(4);
    }
    swDelay(40);
    PlaySound(OFF);
    ClearDisplay();
}


void WonDisplay(void) {

    Graphics_drawStringCentered(&g_sContext, "CONGRATULATIONS!", AUTO_STRING_LENGTH, 64, 70, TRANSPARENT_TEXT);
    Graphics_drawStringCentered(&g_sContext, "YOU WON!", AUTO_STRING_LENGTH, 64, 90, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
    swDelay(40);
    ClearDisplay();
}

