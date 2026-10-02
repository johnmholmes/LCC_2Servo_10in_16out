#ifndef CONFIG_H
#define CONFIG_H

// To set a new nodeid based on your MERG membership number, edit the next two lines only
#define MERG_NUMBER 25345 // substitute this example membership number with your own number(in decimal)
#define NODE_INDEX 1     // Change this to a unique number for each node. (0-255)

#define NODE_ADDRESS 0x03, 0x04, (MERG_NUMBER >> 16), (MERG_NUMBER >> 8), (MERG_NUMBER & 0xFF), NODE_INDEX // Donot change this

// To set a new nodeid edit the next line
//#define NODE_ADDRESS  0x05,0x01,0x01,0x01,0x8E,0x04  // must be unique from an address space owned by you for DIY

// To Force Reset EEPROM to Factory Defaults set this value to 1, else 0 to go into operation mode.
#define RESET_TO_FACTORY_DEFAULTS 1

/*
  ======================================================================================
    End of end user configurations Changing anything below this will break the sketch.
  ======================================================================================
*/

// Choose a board, uncomment one line, see boards.h
#define ESP32_BOARD

/* Debugging -- uncomment to activate debugging statements: */
//#define DEBUG Serial

/*
  Un comment out if you wish to use the node as a standalone node.
*/
//#define USEGCSERIAL
//#define NOCAN

#ifdef USEGCSERIAL
  #include "GCSerial.h"
  #undef DEBUG           // Cannot use DEBUG when using GCSerial
#endif

/*
  Altering the number of servos require changes made to the Boards.h for pin allocations.
*/
#define NUM_SERVOS 2
#define NUM_POS    3  

// Define Frog Relay Output Pins
#define FROG_PIN_0  25  // Frog relay for Servo 0 (Servo Pin 32)
#define FROG_PIN_1  26  // Frog relay for Servo 1 (Servo Pin 33)

// Define Discrete Pull-up Input Pins (10 inputs)
#define NUM_INPUTS 10
const uint8_t inputPins[NUM_INPUTS] = { 4, 16, 17, 5, 18, 19, 13, 12, 14, 27 }; 
#define INPUT_TICK_MS        10    // sampling period
#define INPUT_MIN_DEBOUNCE   2     // consecutive differing samples needed (~20 ms)
#define INPUT_TICKS_PER_UNIT 10    // CDI delay unit = 100 ms = 10 ticks

#define NUM_OUTPUTS 16

#define NUM_EVENT ((NUM_SERVOS * 7) + (NUM_INPUTS * 2) + (NUM_OUTPUTS * 2))

// Board definitions
#define MANU " OpenLCB "                    // The manufacturer of node
#define MODEL BOARD " 2Servo10in16out "     // The default model of the board
#define HWVERSION " ESP 1 Beyond "          // Hardware version
#define SWVERSION " 1.0.7 "                 // Software version

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#endif // CONFIG_H
