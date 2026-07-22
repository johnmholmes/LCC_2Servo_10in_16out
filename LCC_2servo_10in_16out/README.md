# This is tested with the ESP32 Devkit 1 only.

This is the first node to be released in the beyond the basic series. They offer more inputs and outputs via extra i2c pheripherals. This node offers the following. The sketch is configured to use the SN65HVD230 transciever module for Can bus comminication.

You will need to have the OpenLCB_Single_Thread library installed for use with the Arduino IDE. This is available in the Arduino library manager.

- 2 servos on pins D32, and D33.
- 2 Frog relay pins D25 for servo 1 and D26 for servo 2. It has been programmed for Active HIGH relays.
- 10 inputs that can be used for sensors or using a push button to control toggled logic states.
  - Righthand side of the shield.
  - D4.
  - D16.
  - D17.
  - D5.
  - D18.
  - D19.
  - Lefthand side of the shield.
  - D13.
  - D12.
  - D14.
  - D27.

- This is a node that uses an i2c 0x20 addressed MCP23017 to give you 16 outputs with either HIGH or LOW state for LED indication for various layout items.
  - A0 to A7
  - B0 to B7
  - These pins can source a maximium of about 20 milliamps so current limiting resistors are required to protect the module.
 
- Board Manager used for testing ESP32 by Espressif version 3.3.10
- Third party Libraries required to be installed.
- You need to have the OpenLCB_Single_Thread Library Version 0.1.19 
- ESP32Servo.h needs to be installed from the Arduino Library Manager version 3.2.1
- ACAN_ESP32 3.0.3
- MCP23017 by Bertrand version 2.0.0




