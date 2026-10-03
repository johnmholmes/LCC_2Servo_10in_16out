#include "Config.h"   // Contains configuration, see "Config.h"
#include "Boards.h"   // Contains Board definitions, see "Boards.h"
#include "mdebugging.h"           // debugging
#include "OpenLCBHeader.h"        // System house-keeping.
#include <ESP32Servo.h>
#include <Wire.h>
#include <MCP23017.h>

// Define MCP23017 Output Configuration (Bertrand Lemasle library)

#define MCP23017_ADDRESS 0x27
MCP23017 mcp = MCP23017(MCP23017_ADDRESS);

// Track target states for the 16 outputs to safely pass from OpenLCB callbacks to the background task
volatile bool targetOutputState[NUM_OUTPUTS];
bool currentOutputState[NUM_OUTPUTS];

// Output modes: steady uses targetOutputState; the flash modes are driven from a shared
// millis() phase so every flashing output blinks in sync.
enum OutputMode : uint8_t { OUT_STEADY = 0, OUT_FLASH_1S = 1, OUT_FLASH_250MS = 2 };
volatile uint8_t targetOutputMode[NUM_OUTPUTS];   // zero-initialised = OUT_STEADY
volatile uint8_t startupServo = 0;   // servos with a higher index wait their turn at boot

extern "C" {
    #define N(x) xN(x)     
    #define xN(x) #x       
const char configDefInfo[] PROGMEM =
    CDIheader R"(
    <name>Application Configuration</name>
    <hints><visibility hideable='yes' hidden='yes' ></visibility></hints>
    <group>
        <name>Turnout Servo Speed Configuration</name>
         <description>Ensure Servos are powered from a separate 5 volt power supply. Not from the shield</description>
        <int size='1'>
          <name>Speed 5-50 (5 slowest 50 Fastest)</name>
          <min>5</min><max>50</max>
          <hints><slider tickSpacing='15' immediate='yes' showValue='yes'> </slider></hints>
        </int>
    </group>
    <group replication=')" N(NUM_SERVOS) R"('>
        <name>Servos</name>
        <repname>Servo Pin 32</repname>
        <repname>Servo Pin 33</repname>
        <string size='24'><name>Servo Location On Layout.</name></string>

        <group replication=')" N(NUM_POS) R"('>
        <name>  Closed     Midpoint     Thrown</name>
            <repname>Position</repname>
            <eventid><name>EventID</name></eventid>
            <int size='1'>
                <name>Servo Position in approximate degrees range 0 to 180. Take care when using the slider small changes are best done using the text box</name>
                <min>0</min><max>180</max>
                <hints><slider tickSpacing='45' immediate='yes' showValue='yes'> </slider></hints>
            </int>
        </group>
        
        <eventid><name>Servo Reached Closed (Pos 1) Event</name></eventid>
        <eventid><name>Servo Reached Thrown (Pos 3) Event</name></eventid>
        <eventid><name>Servo Passed Midpoint Moving to Thrown (Frog Relay)</name></eventid>
        <eventid><name>Servo Passed Midpoint Moving to Closed (Frog Relay)</name></eventid>
    </group>
    <group replication=')" N(NUM_INPUTS) R"('>
        <name>Inputs Using INPUT_PULLUP To Hold The Pin HIGH 3.3 Volts.</name>
        <repname>D4</repname>
        <repname>D16</repname>
        <repname>D17</repname>
        <repname>D5</repname>
        <repname>D18</repname>
        <repname>D19</repname>
        <repname>D13</repname>
        <repname>D12</repname>
        <repname>D14</repname>
        <repname>D27</repname>
        <string size='24'><name>Input Description / Location</name></string>
        
        <int size='1'>
            <name>Input Operating Mode</name>
            <description>Select how physical pin changes trigger the Event IDs.</description>
            <min>0</min><max>1</max>
            <map>
                <relation><property>0</property><value>Direct State Tracking (Sensor / Switch)</value></relation>
                <relation><property>1</property><value>Pushbutton Toggle State Change</value></relation>
            </map>
        </int>

        <int size='1'>
          <name>On-Delay / Transit LOW (0 to 25.5 seconds)</name>
          <description>Value x 100ms. The pin must stay LOW this long before the event is sent (minimum about 20ms debounce even at 0). In pushbutton mode this is how long the button must be held to count as a press.</description>
          <min>0</min><max>255</max>
          <hints><slider tickSpacing='65' immediate='yes' showValue='yes'> </slider></hints>
        </int>

        <int size='1'>
          <name>Off-Delay / Transit HIGH (0 to 25.5 seconds)</name>
          <description>Value x 100ms. The pin must stay HIGH this long before the event is sent (minimum about 20ms debounce even at 0). In pushbutton mode no event is sent on release, but the button must stay released this long before the next press is accepted.</description>
          <min>0</min><max>255</max>
          <hints><slider tickSpacing='65' immediate='yes' showValue='yes'> </slider></hints>
        </int>

        <eventid>
          <name>Input Transited HIGH Event</name>
          <description>Direct mode: sent when the input goes HIGH. Pushbutton mode: sent on every 2nd press (2nd, 4th, ...).</description>
        </eventid>
        <eventid>
          <name>Input Transited LOW Event</name>
          <description>Direct mode: sent when the input goes LOW. Pushbutton mode: sent on every 1st press (1st, 3rd, ...).</description>
        </eventid>
    </group>
    <group replication=')" N(NUM_OUTPUTS) R"('>
        <name>MCP23017 Outputs (Address )" N(MCP23017_ADDRESS) R"()</name>
        <repname>A0</repname>
        <repname>A1</repname>
        <repname>A2</repname>
        <repname>A3</repname>
        <repname>A4</repname>
        <repname>A5</repname>
        <repname>A6</repname>
        <repname>A7</repname>
        <repname>B0</repname>
        <repname>B1</repname>
        <repname>B2</repname>
        <repname>B3</repname>
        <repname>B4</repname>
        <repname>B5</repname>
        <repname>B6</repname>
        <repname>B7</repname>
        <string size='24'><name>Output Description / Label</name></string>
        <eventid>
          <name>Output HIGH Event</name>
          <description>Sets the output steadily HIGH.</description>
        </eventid>
        <eventid>
          <name>Output LOW Event</name>
          <description>Sets the output steadily LOW.</description>
        </eventid>
        <eventid>
          <name>Output Flash 1 Second Event</name>
          <description>Flashes the output: 1 second on, 1 second off, until another output event is received.</description>
        </eventid>
        <eventid>
          <name>Output Flash 250ms Event</name>
          <description>Flashes the output: 250ms on, 250ms off, until another output event is received.</description>
        </eventid>
    </group>
    )" CDIfooter;
} 

typedef struct {
      EVENT_SPACE_HEADER eventSpaceHeader; 
      char nodeName[20];  
      char nodeDesc[24];  
      uint8_t servodelay; 
      
      struct {
        char desc[24];        
        struct {
          EventID eid;       
          uint8_t angle;     
        } pos[NUM_POS];
        EventID reachedClosedEid; 
        EventID reachedThrownEid; 
        EventID passedMidThrownEid; 
        EventID passedMidClosedEid; 
      } servos[NUM_SERVOS];

      struct {
        char desc[24]; 
        uint8_t mode;     
        uint8_t onDelay;  
        uint8_t offDelay; 
        EventID highStateEid;
        EventID lowStateEid;
      } inputs[NUM_INPUTS];

      struct {
        char desc[24];
        EventID setHighEid;
        EventID setLowEid;
        EventID flash1sEid;
        EventID flash250Eid;
      } outputs[NUM_OUTPUTS];

  } MemStruct;                

uint8_t curpos[NUM_SERVOS]; 

// Dynamic tracker configurations
bool servoMoving[NUM_SERVOS] = {false, false};
bool midCrossed[NUM_SERVOS] = {false, false}; 

// Trackers sized dynamically via NUM_INPUTS macro
bool lastInputState[NUM_INPUTS] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
bool stableInputState[NUM_INPUTS] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH}; 
bool virtualToggleState[NUM_INPUTS] = {false, false, false, false, false, false, false, false, false, false}; 
uint32_t inputTimer[NUM_INPUTS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const uint8_t frogPins[NUM_SERVOS] = { FROG_PIN_0, FROG_PIN_1 };

extern "C" {
    // Total registered Node events: 14 (Servos) + 20 (Inputs) + 64 (Outputs) = 98
    const EIDTab eidtab[NUM_EVENT] PROGMEM = {
        // ================= SERVO 0 (7 Events) =================
        CEID(servos[0].pos[0].eid), CEID(servos[0].pos[1].eid), CEID(servos[0].pos[2].eid),          
        PEID(servos[0].reachedClosedEid), PEID(servos[0].reachedThrownEid),    
        PEID(servos[0].passedMidThrownEid), PEID(servos[0].passedMidClosedEid),  

        // ================= SERVO 1 (7 Events) =================
        CEID(servos[1].pos[0].eid), CEID(servos[1].pos[1].eid), CEID(servos[1].pos[2].eid),          
        PEID(servos[1].reachedClosedEid), PEID(servos[1].reachedThrownEid),    
        PEID(servos[1].passedMidThrownEid), PEID(servos[1].passedMidClosedEid),

        // ================= INPUTS 0-9 (20 Events) =================
        PEID(inputs[0].highStateEid), PEID(inputs[0].lowStateEid),
        PEID(inputs[1].highStateEid), PEID(inputs[1].lowStateEid),
        PEID(inputs[2].highStateEid), PEID(inputs[2].lowStateEid),
        PEID(inputs[3].highStateEid), PEID(inputs[3].lowStateEid),
        PEID(inputs[4].highStateEid), PEID(inputs[4].lowStateEid),
        PEID(inputs[5].highStateEid), PEID(inputs[5].lowStateEid),
        PEID(inputs[6].highStateEid), PEID(inputs[6].lowStateEid),
        PEID(inputs[7].highStateEid), PEID(inputs[7].lowStateEid),
        PEID(inputs[8].highStateEid), PEID(inputs[8].lowStateEid),
        PEID(inputs[9].highStateEid), PEID(inputs[9].lowStateEid),

        // ================= OUTPUTS 0-15 (64 Events) =================
        CEID(outputs[0].setHighEid), CEID(outputs[0].setLowEid), CEID(outputs[0].flash1sEid), CEID(outputs[0].flash250Eid),
        CEID(outputs[1].setHighEid), CEID(outputs[1].setLowEid), CEID(outputs[1].flash1sEid), CEID(outputs[1].flash250Eid),
        CEID(outputs[2].setHighEid), CEID(outputs[2].setLowEid), CEID(outputs[2].flash1sEid), CEID(outputs[2].flash250Eid),
        CEID(outputs[3].setHighEid), CEID(outputs[3].setLowEid), CEID(outputs[3].flash1sEid), CEID(outputs[3].flash250Eid),
        CEID(outputs[4].setHighEid), CEID(outputs[4].setLowEid), CEID(outputs[4].flash1sEid), CEID(outputs[4].flash250Eid),
        CEID(outputs[5].setHighEid), CEID(outputs[5].setLowEid), CEID(outputs[5].flash1sEid), CEID(outputs[5].flash250Eid),
        CEID(outputs[6].setHighEid), CEID(outputs[6].setLowEid), CEID(outputs[6].flash1sEid), CEID(outputs[6].flash250Eid),
        CEID(outputs[7].setHighEid), CEID(outputs[7].setLowEid), CEID(outputs[7].flash1sEid), CEID(outputs[7].flash250Eid),
        CEID(outputs[8].setHighEid), CEID(outputs[8].setLowEid), CEID(outputs[8].flash1sEid), CEID(outputs[8].flash250Eid),
        CEID(outputs[9].setHighEid), CEID(outputs[9].setLowEid), CEID(outputs[9].flash1sEid), CEID(outputs[9].flash250Eid),
        CEID(outputs[10].setHighEid), CEID(outputs[10].setLowEid), CEID(outputs[10].flash1sEid), CEID(outputs[10].flash250Eid),
        CEID(outputs[11].setHighEid), CEID(outputs[11].setLowEid), CEID(outputs[11].flash1sEid), CEID(outputs[11].flash250Eid),
        CEID(outputs[12].setHighEid), CEID(outputs[12].setLowEid), CEID(outputs[12].flash1sEid), CEID(outputs[12].flash250Eid),
        CEID(outputs[13].setHighEid), CEID(outputs[13].setLowEid), CEID(outputs[13].flash1sEid), CEID(outputs[13].flash250Eid),
        CEID(outputs[14].setHighEid), CEID(outputs[14].setLowEid), CEID(outputs[14].flash1sEid), CEID(outputs[14].flash250Eid),
        CEID(outputs[15].setHighEid), CEID(outputs[15].setLowEid), CEID(outputs[15].flash1sEid), CEID(outputs[15].flash250Eid)
    };

    extern const char SNII_const_data[] PROGMEM = 
    "\001" MANU "\000" MODEL "\000" HWVERSION "\000" SWVERSION "Lib " OlcbCommonVersion;
}

uint8_t protocolIdentValue[6] = {   
        pSimple | pDatagram | pMemConfig | pPCEvents | !pIdent    | pTeach     | !pStream   | !pReservation, 
        pACDI   | pSNIP     | pCDI       | !pRemote  | !pDisplay  | !pTraction | !pFunction | !pDCC        , 
        0, 0, 0, 0                                                                                         
};

Servo servo[NUM_SERVOS];
uint8_t servoActual[NUM_SERVOS];
uint8_t servoTarget[NUM_SERVOS];
uint8_t servopin[]  = { SERVOPINS };

#define SERVO_DELAY_OFFSET  EEADDR(servodelay)
//bool posdirty = false;

void servoSet(); 

void reportConfig() {
  dP("\n 2Servos, 10 Inputs, and 16 MCP23017 Outputs Loaded (Bertrand Library).");
  dP("\nNode ID="); dP(TOSTRING((NODE_ADDRESS)));
}

void userInitAll()
{ 
  NODECONFIG.put(EEADDR(nodeName), ESTRING("Esp32"));
  NODECONFIG.put(EEADDR(nodeDesc), ESTRING("2Servo10in16out"));
  NODECONFIG.update(SERVO_DELAY_OFFSET, 20);
  
  for(uint8_t i = 0; i < NUM_SERVOS; i++) {
    NODECONFIG.put(EEADDR(servos[i].desc), ESTRING(""));
    for(int p=0; p<NUM_POS; p++) {
      NODECONFIG.update(EEADDR(servos[i].pos[p].angle), 90);
    }
  }

  for(uint8_t i = 0; i < NUM_INPUTS; i++) {
    NODECONFIG.put(EEADDR(inputs[i].desc), ESTRING(""));
    NODECONFIG.update(EEADDR(inputs[i].mode), 0);     
    NODECONFIG.update(EEADDR(inputs[i].onDelay), 0);  
    NODECONFIG.update(EEADDR(inputs[i].offDelay), 0); 
  }

  for(uint8_t i = 0; i < NUM_OUTPUTS; i++) {
    NODECONFIG.put(EEADDR(outputs[i].desc), ESTRING(""));
  }
  
  EEPROMcommit;
}

enum evStates { VALID=4, INVALID=5, UNKNOWN=7 };

uint8_t userState(uint16_t index) {
    if (index < (NUM_SERVOS * 7)) {
        int ch = index / 7; 
        int localIndex = index % 7;
        if (localIndex < 3) {
            if (curpos[ch] == localIndex) return VALID;
            else return INVALID;
        }
        if (!servoMoving[ch] && servoActual[ch] == servoTarget[ch]) {
            if (localIndex == 3 && curpos[ch] == 0) return VALID; 
            if (localIndex == 4 && curpos[ch] == 2) return VALID; 
        }
        return INVALID;
    } 
    else if (index < (NUM_SERVOS * 7) + (NUM_INPUTS * 2)) {
        int inputIdx = (index - (NUM_SERVOS * 7)) / 2;
        int stateType = (index - (NUM_SERVOS * 7)) % 2; 
        
        uint8_t operationalMode = NODECONFIG.read(EEADDR(inputs[inputIdx].mode));
        if (operationalMode == 0) {
            bool trackingState = stableInputState[inputIdx];
            if (stateType == 0 && trackingState == HIGH) return VALID;
            if (stateType == 1 && trackingState == LOW) return VALID;
        } else {
            bool trackingState = virtualToggleState[inputIdx];
            if (stateType == 0 && trackingState == false) return VALID; 
            if (stateType == 1 && trackingState == true) return VALID;  
        }
        return INVALID;
    }
    else if (index < (NUM_SERVOS * 7) + (NUM_INPUTS * 2) + (NUM_OUTPUTS * 4)) {
        int outIdx    = (index - (NUM_SERVOS * 7) - (NUM_INPUTS * 2)) / 4;
        int stateType = (index - (NUM_SERVOS * 7) - (NUM_INPUTS * 2)) % 4;
        uint8_t mode  = targetOutputMode[outIdx];

        if (stateType == 0) return (mode == OUT_STEADY && currentOutputState[outIdx] == HIGH) ? VALID : INVALID;
        if (stateType == 1) return (mode == OUT_STEADY && currentOutputState[outIdx] == LOW)  ? VALID : INVALID;
        if (stateType == 2) return (mode == OUT_FLASH_1S)    ? VALID : INVALID;
        return                     (mode == OUT_FLASH_250MS) ? VALID : INVALID;
    }
    
    return UNKNOWN;
}  

// Only change: servo command fields are written in a safe order so the servo task
// never sees a half-written command (midCrossed reset BEFORE the target, moving flag LAST).

void pceCallback(uint16_t index) {
    dP("\npceCallback, index="); dP((uint16_t)index);

    // ---- Servo position events (consumed: +0..+2 per servo) ----
    if (index < (NUM_SERVOS * 7)) {
        int ch = index / 7;
        int localIndex = index % 7;
        if (ch < NUM_SERVOS && localIndex < 3) {
            curpos[ch]     = localIndex;
            midCrossed[ch] = false;     // reset before the new target is visible
            servoTarget[ch] = NODECONFIG.read( EEADDR(servos[ch].pos[localIndex].angle) );
            servoMoving[ch] = true;     // last: the task now sees a complete command
        }
        return;
    }

    // ---- Output events (consumed): +0 HIGH, +1 LOW, +2 flash 1 s, +3 flash 250 ms ----
    uint16_t outputStartOffset = (NUM_SERVOS * 7) + (NUM_INPUTS * 2);
    if (index >= outputStartOffset && index < (outputStartOffset + (NUM_OUTPUTS * 4))) {
        int outIdx    = (index - outputStartOffset) / 4;
        int stateType = (index - outputStartOffset) % 4;

        switch (stateType) {
          case 0: targetOutputState[outIdx] = HIGH; targetOutputMode[outIdx] = OUT_STEADY;      break;
          case 1: targetOutputState[outIdx] = LOW;  targetOutputMode[outIdx] = OUT_STEADY;      break;
          case 2:                                   targetOutputMode[outIdx] = OUT_FLASH_1S;    break;
          case 3:                                   targetOutputMode[outIdx] = OUT_FLASH_250MS; break;
        }
        return;
    }
}

void userSoftReset() {}
void userHardReset() {}

NodeID nodeid(NODE_ADDRESS);  
#include "OpenLCBMid.h"    

void userConfigWritten(uint32_t address, uint16_t length, uint16_t func)
{
  EEPROMcommit;
  servoSet();
}

// Drop-in replacement for servoBackgroundTask() in the 2 servo / 10 in / 16 out sketch.
// Event index layout per servo (unchanged): base = i * 7
//   +0..+2 consumed position events, +3 reached Closed, +4 reached Thrown,
//   +5 passed midpoint moving to Thrown, +6 passed midpoint moving to Closed.
// Frog relay: LOW = Closed side, HIGH = Thrown side (as in servoStartUp()).

void servoBackgroundTask(void * parameter) {
  static uint32_t lastmove = 0;

  for(;;) {
    // Speed: higher slider value = bigger step per 20 ms tick = faster.
    uint8_t sliderVal = NODECONFIG.read( SERVO_DELAY_OFFSET );
    if (sliderVal < 1) sliderVal = 1;
    uint8_t stepSize = sliderVal / 5;
    if (stepSize < 1) stepSize = 1;

    vTaskDelay(pdMS_TO_TICKS(20));

    for (int i = 0; i < NUM_SERVOS; i++) {
      // Startup gate: servos with a higher index wait their turn at boot.
      // Once every servo has settled once, startupServo == NUM_SERVOS and this never triggers.
      if (i > startupServo) continue;

      const uint16_t base = i * 7;
      const uint8_t midAngle  = NODECONFIG.read( EEADDR(servos[i].pos[1].angle) );
      const uint8_t oldActual = servoActual[i];
      const uint8_t target    = servoTarget[i];  // snapshot: another task may change it mid-iteration

      // ---- Arrived ----
      if (target == oldActual) {
        if (servoMoving[i]) {
          servoMoving[i] = false;
          // Force the frog relay to the correct side on arrival, even if the
          // midpoint crossing was never detected (odd midpoint angle, config edit...).
          if (curpos[i] == 0) {
            digitalWrite(frogPins[i], LOW);
            OpenLcb.produce(base + 3);   // reached Closed
          } else if (curpos[i] == 2) {
            digitalWrite(frogPins[i], HIGH);
            OpenLcb.produce(base + 4);   // reached Thrown
          }
          // curpos == 1 (midpoint position): no event, frog left as is.
        }
        // Release the next servo at startup. Deliberately outside the servoMoving check so a
        // servo that is already at its target counts as settled and cannot stall the sequence.
        if (i == startupServo) startupServo = startupServo + 1;
        continue;
      }

      // ---- Step towards the target ----
      if (target > oldActual) {
        servoActual[i] = ((target - oldActual) > stepSize) ? (uint8_t)(oldActual + stepSize) : target;
      } else {
        servoActual[i] = ((oldActual - target) > stepSize) ? (uint8_t)(oldActual - stepSize) : target;
      }
      const uint8_t newActual = servoActual[i];

      // ---- Midpoint crossing (frog relay + event) ----
      // Inclusive comparisons so a move that STARTS exactly on midAngle still counts.
      // The servo has moved (newActual != oldActual), so this cannot fire while idle.
      if (servoMoving[i] && !midCrossed[i] && (curpos[i] == 0 || curpos[i] == 2)) {
        const bool crossed = (oldActual <= midAngle && newActual >= midAngle) ||
                             (oldActual >= midAngle && newActual <= midAngle);
        if (crossed) {
          midCrossed[i] = true;
          if (curpos[i] == 2) {
            digitalWrite(frogPins[i], HIGH);
            OpenLcb.produce(base + 5);   // passed midpoint moving to Thrown
          } else {
            digitalWrite(frogPins[i], LOW);
            OpenLcb.produce(base + 6);   // passed midpoint moving to Closed
          }
        }
      }

      // ---- Drive the servo ----
      if (!servo[i].attached()) {
        servo[i].attach(servopin[i]);
        vTaskDelay(pdMS_TO_TICKS(50));
      }
      servo[i].write(newActual);
      lastmove = millis();
    }

    // Release the servos 1 s after the last movement (stops hum/jitter and saves power).
    if (lastmove && (millis() - lastmove) > 1000) {
      for (int i = 0; i < NUM_SERVOS; i++) servo[i].detach();
      lastmove = 0;
    }
  }
}


void inputBackgroundTask(void * parameter) {
  for(;;) {
    vTaskDelay(pdMS_TO_TICKS(INPUT_TICK_MS));

    for (int i = 0; i < NUM_INPUTS; i++) {
      const bool reading = digitalRead(inputPins[i]);

      // Back at the accepted level: cancel any pending change.
      if (reading == stableInputState[i]) {
        inputTimer[i] = 0;
        continue;
      }

      // Reading differs from the accepted level: count consecutive samples.
      inputTimer[i]++;

      // On-Delay when going LOW, Off-Delay when going HIGH (EEPROM read only while a change is pending).
      uint8_t units;
      if (reading == LOW) units = NODECONFIG.read(EEADDR(inputs[i].onDelay));
      else                units = NODECONFIG.read(EEADDR(inputs[i].offDelay));

      uint32_t needed = (uint32_t)units * INPUT_TICKS_PER_UNIT;
      if (needed < INPUT_MIN_DEBOUNCE) needed = INPUT_MIN_DEBOUNCE;
      if (inputTimer[i] < needed) continue;

      // ---- Change accepted ----
      stableInputState[i] = reading;
      inputTimer[i] = 0;

      const uint16_t base = (NUM_SERVOS * 7) + (i * 2);   // +0 = HIGH event, +1 = LOW event
      const uint8_t  mode = NODECONFIG.read(EEADDR(inputs[i].mode));

      if (mode == 0) {
        // Follow the input.
        OpenLcb.produce(reading == HIGH ? base : base + 1);
      } else if (reading == LOW) {
        // Pushbutton toggle: only the press counts, the release just re-arms the input.
        virtualToggleState[i] = !virtualToggleState[i];
        OpenLcb.produce(virtualToggleState[i] ? base + 1 : base);
      }
    }
  }
}

void outputBackgroundTask(void * parameter) {
  for(;;) {
    vTaskDelay(pdMS_TO_TICKS(25));          // fine enough for 250 ms flashing
    const uint32_t now = millis();

    for (int i = 0; i < NUM_OUTPUTS; i++) {
      bool desired;
      switch (targetOutputMode[i]) {
        case OUT_FLASH_1S:    desired = (((now / 1000) & 1) == 0); break;  // 1 s on, 1 s off
        case OUT_FLASH_250MS: desired = (((now / 250)  & 1) == 0); break;  // 250 ms on, 250 ms off
        default:              desired = targetOutputState[i];      break;  // steady
      }

      // Only touch the I2C bus when the pin actually has to change.
      // Bertrand's library handles 0-15 sequentially using standard HIGH/LOW macros.
      if (desired != currentOutputState[i]) {
        currentOutputState[i] = desired;
        mcp.digitalWrite(i, desired ? HIGH : LOW);
      }
    }
  }
}

void servoStartUp() {
  for(int i=0; i<NUM_SERVOS; i++) {
    curpos[i] = 0; // Target is position 0 (Closed)
    digitalWrite(frogPins[i], LOW); // Setup default frog relay orientation
    
    // 1. Force the physical starting position tracking to 90 degrees
    servoActual[i] = 90;
    delay(500);
    
    // 2. Read what the actual intended Target angle is for position 0 (Closed)
    servoTarget[i] = NODECONFIG.read( EEADDR( servos[i].pos[curpos[i]].angle ) );
    
    // 3. Attach the pin and command an immediate sweep to the 90-degree reference point
    servo[i].attach(servopin[i]);
    servo[i].write(servoActual[i]);
    
    // 4. Prime the background task flags to sweep from 90 to target using the slider's speed rate
    if (servoTarget[i] != servoActual[i]) {
      servoMoving[i] = true;
    } else {
      servoMoving[i] = false; 
    }
    midCrossed[i] = false;
    
    delay(100); // Small pause for electrical stability during power-up sequence
  }
  // Synchronizes targets cleanly across execution memory blocks
  servoSet();
}

void servoSet() {
  for(int i=0; i<NUM_SERVOS; i++) {
    servoTarget[i] = NODECONFIG.read( EEADDR( servos[i].pos[curpos[i]].angle ) );
  }
}

void setup()
{
  Serial.begin(115200); while(!Serial);
  delay(2000);
  dP("\n Setup Initialized");

  pinMode(FROG_PIN_0, OUTPUT);
  pinMode(FROG_PIN_1, OUTPUT);

  for(int i = 0; i < NUM_INPUTS; i++) {
    pinMode(inputPins[i], INPUT_PULLUP);
    lastInputState[i] = digitalRead(inputPins[i]); 
    stableInputState[i] = lastInputState[i];
  }

  // Initialize Wire and Bertrand's MCP23017 Configuration
  Wire.begin(); 
  mcp.init();

  for(int i = 0; i < NUM_OUTPUTS; i++) {
    mcp.pinMode(i, OUTPUT);
    mcp.digitalWrite(i, LOW); 
    targetOutputState[i] = LOW;
    targetOutputMode[i]  = OUT_STEADY;
    currentOutputState[i] = LOW;
  }

  EEPROMbegin;
  Olcb_init(nodeid, RESET_TO_FACTORY_DEFAULTS);
  reportConfig();

  servoStartUp();

  xTaskCreatePinnedToCore(servoBackgroundTask, "ServoTask", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(inputBackgroundTask, "InputTask", 3072, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(outputBackgroundTask, "OutputTask", 3072, NULL, 1, NULL, 0);
}

void loop() {
  Olcb_process();        
}
