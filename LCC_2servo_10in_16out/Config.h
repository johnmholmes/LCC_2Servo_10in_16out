#ifndef CONFIG_H
#define CONFIG_H

//----------------------Either Use your MERG number or your own range If using your own comment out the lines 6 & 7 and uncomment line 10-------
// To set a new nodeid based on your MERG membership number, edit the next line
#define MERG_NUMBER 8528 // Add MERG membership number (in decimal)
#define NODE_ADDRESS 0x03, 0x04, (MERG_NUMBER >> 16), (MERG_NUMBER >> 8), (MERG_NUMBER & 0xFF), 1 // The last digit needs to be unique to your nodes in this case its is node 4

// To set a new nodeid edit the next line
#define NODE_ADDRESS  0x05,0x01,0x01,0x01,0x8E,0x04  // must be unique from an address space owned by you for DIY

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
#define NUM_EVENT  66

// Board definitions
#define MANU " OpenLCB "                    // The manufacturer of node
#define MODEL BOARD " 2Servo10in16out "     // The default model of the board
#define HWVERSION " ESP 1 Beyond "          // Hardware version
#define SWVERSION " 1.0.4 "                 // Software version

// Global defs
const bool USE_90_ON_STARTUP = true;  

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#endif // CONFIG_H
