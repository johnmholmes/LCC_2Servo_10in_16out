#include "Config.h"   // Contains configuration, see "Config.h"
#include "Boards.h"   // Contains Board definitions, see "Boards.h"
#include "mdebugging.h"           // debugging
#include "OpenLCBHeader.h"        // System house-keeping.
#include <ESP32Servo.h>
#include <Wire.h>
#include <MCP23017.h>

#define OLCB_NO_BLUE_GOLD // Do not delete

// Define Frog Relay Output Pins
#define FROG_PIN_0  25  // Frog relay for Servo 0 (Servo Pin 32)
#define FROG_PIN_1  26  // Frog relay for Servo 1 (Servo Pin 33)

// Define Discrete Pull-up Input Pins (10 inputs)
#define NUM_INPUTS 10
const uint8_t inputPins[NUM_INPUTS] = { 4, 16, 17, 5, 18, 19, 13, 12, 14, 27 }; 

// Define MCP23017 Output Configuration (Bertrand Lemasle library)
#define NUM_OUTPUTS 16
#define MCP23017_ADDRESS 0x20
MCP23017 mcp = MCP23017(MCP23017_ADDRESS);

// Track target states for the 16 outputs to safely pass from OpenLCB callbacks to the background task
volatile bool targetOutputState[NUM_OUTPUTS];
bool currentOutputState[NUM_OUTPUTS];

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
          <name>Speed 5-50 (delay between steps)</name>
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
        <repname>D4 </repname>
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
          <description>Value multiplied by 100ms. Time signal must stay LOW before event transmits.</description>
          <min>0</min><max>255</max>
          <hints><slider tickSpacing='65' immediate='yes' showValue='yes'> </slider></hints>
        </int>

        <int size='1'>
          <name>Off-Delay / Transit HIGH (0 to 25.5 seconds)</name>
          <description>Value multiplied by 100ms. Time signal must stay HIGH before event transmits.</description>
          <min>0</min><max>255</max>
          <hints><slider tickSpacing='65' immediate='yes' showValue='yes'> </slider></hints>
        </int>

        <eventid><name>Input Transited HIGH Event </name></eventid>
        <eventid><name>Input Transited LOW Event </name></eventid>
    </group>
    <group replication=')" N(NUM_OUTPUTS) R"('>
        <name>MCP23017 Outputs (Address 0x20)</name>
        <repname>A0 </repname>
        <repname>A1 </repname>
        <repname>A2 </repname>
        <repname>A3 </repname>
        <repname>A4 </repname>
        <repname>A5 </repname>
        <repname>A6 </repname>
        <repname>A7 </repname>
        <repname>B0 </repname>
        <repname>B1 </repname>
        <repname>B2 </repname>
        <repname>B3 </repname>
        <repname>B4 </repname>
        <repname>B5 </repname>
        <repname>B6 </repname>
        <repname>B7 </repname>
        <string size='24'><name>Output Description / Label</name></string>
        <eventid><name>Output HIGH Event</name></eventid>
        <eventid><name>Output LOW Event</name></eventid>
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
      } outputs[NUM_OUTPUTS];

  uint8_t curpos[NUM_SERVOS]; 
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
    // Total registered Node events: 14 (Servos) + 20 (Inputs) + 32 (Outputs) = 66
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

        // ================= OUTPUTS 0-15 (32 Events) =================
        CEID(outputs[0].setHighEid), CEID(outputs[0].setLowEid),
        CEID(outputs[1].setHighEid), CEID(outputs[1].setLowEid),
        CEID(outputs[2].setHighEid), CEID(outputs[2].setLowEid),
        CEID(outputs[3].setHighEid), CEID(outputs[3].setLowEid),
        CEID(outputs[4].setHighEid), CEID(outputs[4].setLowEid),
        CEID(outputs[5].setHighEid), CEID(outputs[5].setLowEid),
        CEID(outputs[6].setHighEid), CEID(outputs[6].setLowEid),
        CEID(outputs[7].setHighEid), CEID(outputs[7].setLowEid),
        CEID(outputs[8].setHighEid), CEID(outputs[8].setLowEid),
        CEID(outputs[9].setHighEid), CEID(outputs[9].setLowEid),
        CEID(outputs[10].setHighEid), CEID(outputs[10].setLowEid),
        CEID(outputs[11].setHighEid), CEID(outputs[11].setLowEid),
        CEID(outputs[12].setHighEid), CEID(outputs[12].setLowEid),
        CEID(outputs[13].setHighEid), CEID(outputs[13].setLowEid),
        CEID(outputs[14].setHighEid), CEID(outputs[14].setLowEid),
        CEID(outputs[15].setHighEid), CEID(outputs[15].setLowEid)
    };

    extern const char SNII_const_data[] PROGMEM = 
    "\001" MANU "\000" MODEL "\000" HWVERSION "\000" SWVERSION " " OlcbCommonVersion;
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
bool posdirty = false;

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
    else if (index < (NUM_SERVOS * 7) + (NUM_INPUTS * 2) + (NUM_OUTPUTS * 2)) {
        int outIdx = (index - (NUM_SERVOS * 7) - (NUM_INPUTS * 2)) / 2;
        int stateType = (index - (NUM_SERVOS * 7) - (NUM_INPUTS * 2)) % 2;

        bool trackingState = currentOutputState[outIdx];
        if (stateType == 0 && trackingState == HIGH) return VALID;
        if (stateType == 1 && trackingState == LOW) return VALID;
        return INVALID;
    }
    
    return UNKNOWN;
}  

void pceCallback(uint16_t index) {
    dP("\npceCallback, index="); dP((uint16_t)index);
    
    if (index < (NUM_SERVOS * 7)) {
        int ch = index / 7;
        int localIndex = index % 7;
        if (ch < NUM_SERVOS && localIndex < 3) {
            curpos[ch] = localIndex;
            servoTarget[ch] = NODECONFIG.read( EEADDR(servos[ch].pos[localIndex].angle) );
            servoMoving[ch] = true; 
            midCrossed[ch] = false; 
        }
        return;
    }

    uint16_t outputStartOffset = (NUM_SERVOS * 7) + (NUM_INPUTS * 2);
    if (index >= outputStartOffset && index < (outputStartOffset + (NUM_OUTPUTS * 2))) {
        int outIdx = (index - outputStartOffset) / 2;
        int stateType = (index - outputStartOffset) % 2;

        if (stateType == 0) {
            targetOutputState[outIdx] = HIGH;
        } else {
            targetOutputState[outIdx] = LOW;
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

void servoBackgroundTask(void * parameter) {
  for(;;) {
    uint8_t sliderVal = NODECONFIG.read( SERVO_DELAY_OFFSET );
    if (sliderVal < 1) sliderVal = 1; 
    uint8_t stepSize = sliderVal / 5; 
    if (stepSize < 1) stepSize = 1;

    vTaskDelay(pdMS_TO_TICKS(20));
    static long lastmove = 0;
    
    for(int i=0; i<NUM_SERVOS; i++) {
      uint8_t midAngle = NODECONFIG.read( EEADDR(servos[i].pos[1].angle) );
      uint8_t oldActual = servoActual[i]; 

      if(servoTarget[i] == servoActual[i] ) {
        if (servoMoving[i]) {
          servoMoving[i] = false; 
          uint16_t servoBaseIndex = i * 7; 
          if (curpos[i] == 0) OpenLcb.produce(servoBaseIndex + 3); 
          else if (curpos[i] == 2) OpenLcb.produce(servoBaseIndex + 4); 
        }
        continue;
      }
      
      if(servoTarget[i] > servoActual[i]) {
        if ((servoTarget[i] - servoActual[i]) > stepSize) servoActual[i] += stepSize;
        else servoActual[i] = servoTarget[i]; 
      }
      else if(servoTarget[i] < servoActual[i]) {
        if ((servoActual[i] - servoTarget[i]) > stepSize) servoActual[i] -= stepSize;
        else servoActual[i] = servoTarget[i]; 
      }

      if (servoMoving[i] && !midCrossed[i]) {
        if (curpos[i] == 2) { 
          if ((oldActual < midAngle && servoActual[i] >= midAngle) || (oldActual > midAngle && servoActual[i] <= midAngle)) {
             midCrossed[i] = true;
             digitalWrite(frogPins[i], HIGH); 
             OpenLcb.produce((i * 7) + 5);
          }
        }
        else if (curpos[i] == 0) {
          if ((oldActual > midAngle && servoActual[i] <= midAngle) || (oldActual < midAngle && servoActual[i] >= midAngle)) {
             midCrossed[i] = true;
             digitalWrite(frogPins[i], LOW);  
             OpenLcb.produce((i * 7) + 6);
          }
        }
      }
      
      if(!servo[i].attached()) { 
        servo[i].attach(servopin[i]);
        vTaskDelay(pdMS_TO_TICKS(50));
      }
      servo[i].write(servoActual[i]);
      lastmove = millis();
      posdirty = true;
    }

    if( lastmove && (millis()-lastmove)>1000) {
      for(int i=0; i<NUM_SERVOS; i++) servo[i].detach();
      lastmove = 0;
    }
  }
}

void inputBackgroundTask(void * parameter) {
  for(;;) {
    vTaskDelay(pdMS_TO_TICKS(100)); 
    
    for(int i = 0; i < NUM_INPUTS; i++) {
      bool currentReading = digitalRead(inputPins[i]);
      uint8_t operationalMode = NODECONFIG.read(EEADDR(inputs[i].mode));
      uint16_t inputBaseIndex = (NUM_SERVOS * 7) + (i * 2);

      if (operationalMode == 0) {
        // ==================== DIRECT STATE TRACKING ====================
        if (currentReading != lastInputState[i]) {
          // Input just changed - start new debounce timer
          lastInputState[i] = currentReading;
          uint8_t configurationDelay = (currentReading == LOW) 
              ? NODECONFIG.read(EEADDR(inputs[i].onDelay)) 
              : NODECONFIG.read(EEADDR(inputs[i].offDelay));
          
          inputTimer[i] = configurationDelay;
          
          // Immediate action when delay = 0
          if (inputTimer[i] == 0) {
            stableInputState[i] = currentReading;
            if (stableInputState[i] == HIGH) {
              OpenLcb.produce(inputBaseIndex);      // HIGH event
            } else {
              OpenLcb.produce(inputBaseIndex + 1);  // LOW event
            }
          }
        } 
        else if (currentReading != stableInputState[i]) {
          // Still waiting for stable state
          if (inputTimer[i] > 0) {
            inputTimer[i]--;
          }
          if (inputTimer[i] == 0) {
            stableInputState[i] = currentReading;
            if (stableInputState[i] == HIGH) {
              OpenLcb.produce(inputBaseIndex);
            } else {
              OpenLcb.produce(inputBaseIndex + 1);
            }
          }
        }
      } 
      else {
        // ==================== PUSHBUTTON TOGGLE MODE ====================
        if (currentReading != lastInputState[i]) {
          lastInputState[i] = currentReading;

          if (currentReading == LOW) {
            // Button pressed - start onDelay
            uint8_t valDelay = NODECONFIG.read(EEADDR(inputs[i].onDelay));
            inputTimer[i] = valDelay;

            if (valDelay == 0 && stableInputState[i] == HIGH) {
              // Immediate toggle on press when delay=0
              stableInputState[i] = LOW;
              virtualToggleState[i] = !virtualToggleState[i];
              
              if (virtualToggleState[i]) {
                OpenLcb.produce(inputBaseIndex + 1);  // LOW / "on" event
              } else {
                OpenLcb.produce(inputBaseIndex);      // HIGH / "off" event
              }
              inputTimer[i] = NODECONFIG.read(EEADDR(inputs[i].offDelay));
            }
          } 
          else {
            // Button released
            if (stableInputState[i] == LOW) {
              stableInputState[i] = HIGH;
            } else {
              inputTimer[i] = 0;
            }
          }
        } 
        else if (currentReading == LOW && stableInputState[i] == HIGH) {
          // Button is being held down - countdown for debounce
          if (inputTimer[i] > 0) {
            inputTimer[i]--;
          }
          if (inputTimer[i] == 0) {
            stableInputState[i] = LOW;
            virtualToggleState[i] = !virtualToggleState[i];
            
            if (virtualToggleState[i]) {
              OpenLcb.produce(inputBaseIndex + 1);
            } else {
              OpenLcb.produce(inputBaseIndex);
            }
            inputTimer[i] = NODECONFIG.read(EEADDR(inputs[i].offDelay));
          }
        } 
        else {
          // No interesting activity - just decrement any remaining timer
          if (inputTimer[i] > 0) {
            inputTimer[i]--;
          }
        }
      }
    }
  }
}

void outputBackgroundTask(void * parameter) {
  for(;;) {
    vTaskDelay(pdMS_TO_TICKS(50)); 
    
    for(int i = 0; i < NUM_OUTPUTS; i++) {
      if (targetOutputState[i] != currentOutputState[i]) {
        currentOutputState[i] = targetOutputState[i];
        // Bertrand's library handles 0-15 sequentially using standard HIGH/LOW macros
        mcp.digitalWrite(i, currentOutputState[i] ? HIGH : LOW);
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
    currentOutputState[i] = LOW;
  }

  EEPROMbegin;
  NodeID nodeid(NODE_ADDRESS);      
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
