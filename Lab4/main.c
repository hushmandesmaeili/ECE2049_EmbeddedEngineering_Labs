#include <msp430.h>
#include <stdio.h>
#include <stdlib.h>
#include "peripherals.h"
#include "string.h"


/**
 * main.c
 */

//This is to use for slave configurations
#define SLAVE_PORT_SPI_SEL  P4SEL
#define SLAVE_PORT_SPI_DIR  P4DIR
#define SLAVE_PORT_SPI_OUT  P4OUT

#define SLAVE_PORT_CS_SEL   P4SEL
#define SLAVE_PORT_CS_DIR   P4DIR
#define SLAVE_PORT_CS_OUT   P4OUT
#define SLAVE_PORT_CS_REN   P4REN

#define SLAVE_PIN_SPI_MOSI  BIT1
#define SLAVE_PIN_SPI_MISO  BIT2
#define SLAVE_PIN_SPI_SCLK  BIT3
#define SLAVE_PIN_SPI_CS    BIT0

#define SLAVE_SPI_REG_CTL0      UCB1CTL0
#define SLAVE_SPI_REG_CTL1      UCB1CTL1
#define SLAVE_SPI_REG_BRL       UCB1BR0
#define SLAVE_SPI_REG_BRH       UCB1BR1
#define SLAVE_SPI_REG_IFG       UCB1IFG
#define SLAVE_SPI_REG_STAT      UCB1STAT
#define SLAVE_SPI_REG_TXBUF     UCB1TXBUF
#define SLAVE_SPI_REG_RXBUF     UCB1RXBUF

//This is needed to configure P8.2 to use it as CS by MSP430
#define MSP_PORT_CS_SEL     P8SEL
#define MSP_PORT_CS_DIR     P8DIR
#define MSP_PORT_CS_OUT     P8OUT
#define MSP_PIN_CS          BIT2

#define MSP_SPI_REG_TXBUF   UCB0TXBUF
#define MSP_SPI_REG_RXBUF   UCB0RXBUF
#define MSP_SPI_REG_IFG     UCB0IFG

//This is to configure the voltmeter to P6.0 to use in function mode for ADC
#define VOLT_PORT_SEL       P6SEL
#define VOLT_PIN_FUNC       BIT0


// Function Prototypes

void InitSlaveSPI(void);
unsigned char SlaveSPIRead();
void MasterSPIWrite(unsigned int data);

void configUCS(void);
void configTimerA2(void);
__interrupt void Timer_A2_ISR(void);
void stopTimerA2(void);
void enableTimerA2(void);

void configVoltmeter(void);

void sampleVoltage(void);
float readVoltage(unsigned int adc_out);

long unsigned int TimerSetValue(long unsigned int tempTimer);
float VoltageSetValue(float in_voltage);
long unsigned int Send_4_Bytes(long unsigned int data_4_bytes);

void displayTime(long unsigned int inTime);
void displayVoltage(unsigned int inVolt);


const float vref_pos = 3.3;

long unsigned int timer;                        // timer count for TimerA2, increased by TimerA2 ISR
unsigned int in_volt;                           // ADC value from A0 channel, voltmeter


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    // Useful code starts here
    // Initialization and configuration of LEDs, Display, Keypad, UCS, TimerA2, Push Buttons and Temp Sensor
    initLeds();
    configDisplay();
    configKeypad();

    InitSlaveSPI();

    configUCS();
    configTimerA2();

    configVoltmeter();

    _BIS_SR(GIE);           // enables interrupts


    timer = 2;                           // timer set to initial time
    long unsigned int prevTime = timer - 1;     // prevTime declared and set to timer - 1

    // Clears display from anything
    Graphics_clearDisplay(&g_sContext);
    Graphics_flushBuffer(&g_sContext);


    // Forever loop
    while (1) {

        if (timer >= (prevTime + 1)) {
            sampleVoltage();
            long unsigned int tempTimer = timer;

            // Send timer data, receives timer data and reconstructs bytes,
            // then returns reconstructed data and stores in rTimer variable which is displayed
            long unsigned int rTimer = TimerSetValue(tempTimer);

            unsigned char w, r;
            unsigned int sendVolt = (int)(readVoltage(in_volt) * 10);
            unsigned int rVoltage = 0;

            w = (unsigned char) (sendVolt & 0xFF);
            MasterSPIWrite(w);
            r = SlaveSPIRead();
            rVoltage |= (int)r;

            sendVolt >>= 8;
            w = (unsigned char) (sendVolt & 0xFF);
            MasterSPIWrite(w);
            r = SlaveSPIRead();
            rVoltage |= ((int)r << 8);

            Graphics_clearDisplay(&g_sContext);
            displayTime(rTimer);
            displayVoltage(rVoltage);
            Graphics_flushBuffer(&g_sContext);

            prevTime = timer;
        }

    }
}


void InitSlaveSPI() {

    // Configure SCLK, MISO and MOSI for peripheral mode
    SLAVE_PORT_SPI_SEL |=
            (SLAVE_PIN_SPI_MOSI|SLAVE_PIN_SPI_MISO|SLAVE_PIN_SPI_SCLK);

    // Configure the Slave CS as an Input P4.0
    SLAVE_PORT_CS_SEL &= ~SLAVE_PIN_SPI_CS;
    SLAVE_PORT_CS_DIR &= ~SLAVE_PIN_SPI_CS;
    SLAVE_PORT_CS_REN |= SLAVE_PIN_SPI_CS;
    SLAVE_PORT_CS_OUT |= SLAVE_PIN_SPI_CS;

    // Configure the CS output of MSP430 P8.2. It will set P4.0 high or low.
    MSP_PORT_CS_SEL &= ~MSP_PIN_CS;
    MSP_PORT_CS_DIR |= MSP_PIN_CS;
    MSP_PORT_CS_OUT |= MSP_PIN_CS;

    // Disable the module so we can configure it
    SLAVE_SPI_REG_CTL1 |= UCSWRST;
    SLAVE_SPI_REG_CTL0 &= ~(0xFF); // Reset the controller config parameters
    SLAVE_SPI_REG_CTL1 &= ~UCSSEL_3; // Reset the clock configuration

    // Set SPI clock frequency (which is the same frequency as SMCLK so
    // this can apparently be 0)
    SPI_REG_BRL = ((uint16_t)SPI_CLK_TICKS) & 0xFF; // Load the low byte
    SPI_REG_BRH = (((uint16_t)SPI_CLK_TICKS) >> 8) & 0xFF; // Load the high byte

    //capture data on first edge
    //inactive low polarity
    //MSB first
    //8 bit
    //Slave Mode//
    //4 wire - SPI active low
    //Synchronous mode
    SLAVE_SPI_REG_CTL0 |= (UCCKPH | UCMSB | UCMODE_2 | UCSYNC);
    SLAVE_SPI_REG_CTL0 &= ~(UCCKPL | UC7BIT | UCMST);

    // Reenable the module
    SLAVE_SPI_REG_CTL1 &= ~UCSWRST;
    SLAVE_SPI_REG_IFG &= ~UCRXIFG;
}

unsigned char SlaveSPIRead() {
    unsigned char c;
    while (!(SLAVE_SPI_REG_IFG & UCRXIFG)){
        c = SLAVE_SPI_REG_RXBUF;
    }

    return (c & 0xFF);
}

void MasterSPIWrite (unsigned int data) {
    // Start SPI transmission by de-asserting CS
    MSP_PORT_CS_OUT &= ~MSP_PIN_CS;

    // Write data/ 1-byte at a time
    uint8_t byte = (unsigned char) ((data)&0xFF);

    // Send byte
    MSP_SPI_REG_TXBUF = byte;

    // Wait for SPI peripheral to finish transmitting
    while (!(MSP_SPI_REG_IFG & UCTXIFG)) {
        _no_operation();
    }

    // Assert CS
    MSP_PORT_CS_OUT |= MSP_PIN_CS;
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


// Function to configure voltage meter
void configVoltmeter() {

    // Set Port 6 Pin 0 to FUNCTION mode for ADC
    VOLT_PORT_SEL |= VOLT_PIN_FUNC;

    REFCTL0 &= ~REFMSTR;

    // ADC12 Sample and Hold clock cycle is set to 384 cycles
    // ADC12 ADC12 is turned on
    ADC12CTL0 = ADC12SHT0_9 | ADC12ON;

    // ADC12SHP = 1 = SAMPCON signal sourced from sampling timer
    // Enable sample timer
    ADC12CTL1 = ADC12SHP;

    // Memory location 0 is used for input channel A0
    // Reference voltage is set to VCC = 3.3 V
    ADC12MCTL0 = ADC12SREF_0 + ADC12INCH_0;

    __delay_cycles(100);                        // delay to allow Ref to settle
    ADC12CTL0 |= ADC12ENC;                      // Enable conversion

}

// Function to sample voltage, and convert and store ADC value
void sampleVoltage() {

    ADC12CTL0 &= ~ADC12SC;  // clear the start bit
    ADC12CTL0 |= ADC12SC;   // Sampling and conversion start
                            // Single conversion (single channel)

    // Poll busy bit waiting for conversion to complete
    while (ADC12CTL1 & ADC12BUSY)
        __no_operation();

    in_volt = ADC12MEM0; // Read results from conversion

    __no_operation(); // SET BREAKPOINT HERE
}

// Returns voltage as a float, from ADC value
float readVoltage(unsigned int adc_out) {

    float voltage;

    voltage = (float)((adc_out * vref_pos) / (4095.0));

    return voltage;
}

long unsigned int TimerSetValue(long unsigned int tempTimer) {

    long unsigned int reTimer = 0;
    unsigned char w, r;

    w = (unsigned char) (tempTimer & 0xFF);
    MasterSPIWrite(w);
    r = SlaveSPIRead();
    reTimer |= (((uint32_t)r));

    tempTimer >>= 8;
    w = (unsigned char) (tempTimer & 0xFF);
    MasterSPIWrite(w);
    r = SlaveSPIRead();
    reTimer |= (((uint32_t)r) << 8);

    tempTimer >>= 16;
    w = (unsigned char) (tempTimer & 0xFF);
    MasterSPIWrite(w);
    r = SlaveSPIRead();
    reTimer |= (((uint32_t)r) << 16);

    tempTimer >>= 24;
    w = (unsigned char) (tempTimer & 0xFF);
    MasterSPIWrite(w);
    r = SlaveSPIRead();
    reTimer |= (((uint32_t)r) << 24);

    return reTimer;

}

//long unsigned int Send_4_Bytes(long unsigned int data_4_bytes) {
//
//    long unsigned int slaveRead_val = 0;
//    unsigned char w, r;
//
//    w = (unsigned char) (data_4_bytes & 0xFF);
//    MasterSPIWrite(w);
//    r = SlaveSPIRead();
//    slaveRead_val |= (((uint32_t)r));
//
//    data_4_bytes >>= 8;
//    w = (unsigned char) (data_4_bytes & 0xFF);
//    MasterSPIWrite(w);
//    r = SlaveSPIRead();
//    slaveRead_val |= (((uint32_t)r) << 8);
//
//    data_4_bytes >>= 16;
//    w = (unsigned char) (data_4_bytes & 0xFF);
//    MasterSPIWrite(w);
//    r = SlaveSPIRead();
//    slaveRead_val |= (((uint32_t)r) << 16);
//
//    data_4_bytes >>= 24;
//    w = (unsigned char) (data_4_bytes & 0xFF);
//    MasterSPIWrite(w);
//    r = SlaveSPIRead();
//    slaveRead_val |= (((uint32_t)r) << 24);
//
//    return slaveRead_val;
//
//}



// Function which takes a copy of global time count as its input argument
// Convert the time in seconds that was passed in to Hour, Minutes and Seconds
// Takes above variables and creates ASCII array that is displayed
void displayTime(long unsigned int inTime) {

    unsigned int hours, minutes, seconds;

    // Determines hours, minutes, seconds as an integer
    hours = ((long int)(inTime / (60 * 60))) % 24;
    minutes = ((long int)(inTime / 60)) % 60;
    seconds = inTime % 60;

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

//    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, timeASCII, AUTO_STRING_LENGTH, 64, 70, TRANSPARENT_TEXT);
//    Graphics_flushBuffer(&g_sContext);

}

// Function which takes a copy of voltage variable as its input argument
// Takes above voltage variable and creates ASCII array that is displayed
void displayVoltage(unsigned int inVolt) {

    // Format array for TIME
    unsigned char voltageASCII[10];

    voltageASCII[0] = ((int)(inVolt / 10)) + '0';
    voltageASCII[1] =  '.';
    voltageASCII[2] = ((int)(inVolt % 10)) + '0';
    voltageASCII[3] = ' ';
    voltageASCII[4] = 'V';
    voltageASCII[5] = 'o';
    voltageASCII[6] = 'l';
    voltageASCII[7] = 't';
    voltageASCII[8] = 's';
    voltageASCII[9] = '\0';

    Graphics_drawStringCentered(&g_sContext, voltageASCII, AUTO_STRING_LENGTH, 64, 80, TRANSPARENT_TEXT);

}
