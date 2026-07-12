# LCC_2Servo_10in_16out

This is the first node to be released in the beyond the basic series. They offer more inputs and outputs via extra i2c pheripherals. This node offers the following.

- 2 servos on pins D32, and D33.
- 2 Frog realy pins D25 for servo 1 and D26 for servo 2. It has been programmed for Active HIGH relays.
- 10 inputs that can be used for sensors or using a push button to control toggled logic states.
  - D4,D16,17,5,18,19,
- This is a node that uses an i2c 0x20 addressed MCP23017 to give you 16 outputs

