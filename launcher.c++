/*
 * Combined Launcher + Gate Controller
 *
 * This sketch runs a launcher and a gate motor using two simple states: IDLE and ARMED.
 *
 * IDLE:
 *   - Motors are off so the user can safely set the speed with the potentiometer.
 *   - The LCD shows the target RPM and a progress bar.
 *
 * ARMED (button press from IDLE):
 *   1. The gate swings open and holds.
 *   2. Both launcher motors spin up to the selected speed.
 *   3. You hear a brief dog bark sound for fun.
 *   4. After 10 seconds (or another button press), everything stops and we return to IDLE.
 */


#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Encoder.h>


// DEFINE THE PINOUTS


// Shared controls: button, LED indicator, and speed pot
#define BTN_PIN      12   // Button input (goes HIGH when pressed)
#define LED_PIN      13   // On-board LED to show ARMED state
#define POT_PIN      15   // Potentiometer for adjusting launcher speed


// Launcher Motor #1 (First driver)
#define RPWM1_PIN    26   // PWM pin for forward rotation
#define LPWM1_PIN    25   // PWM pin for reverse rotation
#define REN1_PIN     32   // Enable pin for forward drive
#define LEN1_PIN     33   // Enable pin for reverse drive


// Launcher Motor #2 (second driver)
#define RPWM2_PIN    18
#define LPWM2_PIN    19
#define REN2_PIN     4
#define LEN2_PIN     14


// Gate motor connections (DRV BIN1/BIN2) and encoder pins
#define BIN_1        5    // Gate motor input A
#define BIN_2        27   // Gate motor input B
#define ENC_A_PIN    16   // Encoder channel A
#define ENC_B_PIN    17   // Encoder channel B


//  Parameters
// PWM setup for launcher motors




// I needed to control the motor speed depending on the potentiometer input (analog readout).


// PWM lets me vary the average voltage applied to the motor by adjusting the duty cycle (0–255), without changing the hardware voltage itself.


// This allows smooth, adjustable control over how fast the launcher wheels spin,
// which is critical for tuning the launch force based on user input.


const uint32_t PWM_FREQ   = 20000; // 20 kHz PWM frequency
const uint8_t  PWM_RES    = 8;     // 8-bit resolution (0-255)
const uint16_t MAX_RPM    = 5300;  // Corresponding no-load speed at duty=255


// Gate motion settings
const int      FORWARD_SPEED      =  70;    // Opening speed (0–255)
const int      BACKWARD_SPEED     =  30;    // Closing speed (0–255)
const int      QUARTER_REV_COUNTS = 103;    // around 90° rotation in encoder counts
const uint32_t GATE_HOLD_MS       = 5000;  // How long to keep gate open (1000ms = 1 second)


// ARMED state auto-disarm timeout
const uint32_t ARMED_MS = 10000;  // Auto-disarm after 10 seconds


// Button debounce timing
const uint32_t DEBOUNCE_MS =   50; // Debounce interval in ms


//  State Machine
enum State { IDLE, ARMED };
static State    curState     = IDLE;       // Current system state
static uint8_t  desiredDuty  = 0;          // Target duty cycle for motors
static uint32_t armedStart   = 0;          // Timestamp when ARMED began
static bool     lastBtnState = LOW;        // Previous raw button reading
static uint32_t lastDebounce = 0;          // Last time the button input changed


// Getting LCD to Display
LiquidCrystal_I2C lcd(0x27,16,2);  // I²C LCD at address 0x27, 16×2 chars
ESP32Encoder     gateEncoder;      // Encoder for gate position


// Making LCD Bar Display a Loading Screen & Bar
byte fullBlock[8] = {  // 8 Solid blocks for progress bar
  0x1F,0x1F,0x1F,0x1F,
  0x1F,0x1F,0x1F,0x1F
};
const char spinner[4] = {'|','/','-','\\'}; // Simple spinner frames


//  Motor helpers
// Spin Motor 1 forward at given duty and brake reverse
inline void driveMotor1(uint8_t duty) {
  ledcWrite(RPWM1_PIN, duty);
  ledcWrite(LPWM1_PIN, 0);
}
// Spin Motor 2 forward at given duty and brake reverse
inline void driveMotor2(uint8_t duty) {
  ledcWrite(RPWM2_PIN, duty);
  ledcWrite(LPWM2_PIN, 0);
}


// Gate cycle: open → hold → close
void runGateCycle() {
  Serial.println("Gate: opening...");
  gateEncoder.setCount(0);


  // Start opening gate
  analogWrite(BIN_1, 0);
  analogWrite(BIN_2, FORWARD_SPEED);
  uint32_t openStart = millis();
  while (gateEncoder.getCount() < QUARTER_REV_COUNTS && millis() - openStart < 2000) {
    delay(1);
  }
  analogWrite(BIN_2, 0);
  Serial.println("Gate open (or timeout)");


  delay(GATE_HOLD_MS); // Keep gate open for a moment


  // Start closing gate
  Serial.println("Gate: closing...");
  analogWrite(BIN_1, BACKWARD_SPEED);
  analogWrite(BIN_2, 0);
  uint32_t closeStart = millis();
  while (gateEncoder.getCount() > 0 && millis() - closeStart < 2000) {
    delay(1);
  }
  analogWrite(BIN_1, 0);
  Serial.println("Gate closed (or timeout)");
}


//  LCD for IDLE state: show RPM and a bar graph
void lcdIdle() {
  uint16_t rpm = (uint32_t)desiredDuty * MAX_RPM / 255;
  lcd.setCursor(0,0);
  lcd.printf("IDLE RPM:%5u", rpm);
  int bar = map(rpm, 0, MAX_RPM, 0, 16);
  lcd.setCursor(0,1);
  for(int i=0; i<16; i++) {
    lcd.write(i < bar ? byte(0) : ' ');
  }
}


//  LCD for ARMED state: show a spinner animation
void lcdArmed() {
  uint8_t idx = (millis()/300) % 4;
  lcd.setCursor(0,0);
  lcd.print("Woofie launching");
  lcd.setCursor(0,1);
  lcd.printf("in progress %c  ", spinner[idx]);
}


//  Setup: initialize everything
void setup() {
  Serial.begin(115200);


  // Initialize LCD over I²C
  Wire.begin(23,22);
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, fullBlock);
  lcdIdle(); // Shows IDLE screen


  // Configure button and LED
  pinMode(BTN_PIN, INPUT_PULLDOWN);
  pinMode(LED_PIN, OUTPUT);


  // Prepare launcher motors (enable and PWM)
  pinMode(REN1_PIN, OUTPUT); digitalWrite(REN1_PIN, HIGH);
  pinMode(LEN1_PIN, OUTPUT); digitalWrite(LEN1_PIN, HIGH);
  pinMode(RPWM1_PIN, OUTPUT);
  pinMode(LPWM1_PIN, OUTPUT);
  ledcAttach(RPWM1_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(LPWM1_PIN, PWM_FREQ, PWM_RES);
  driveMotor1(0);


  pinMode(REN2_PIN, OUTPUT); digitalWrite(REN2_PIN, HIGH);
  pinMode(LEN2_PIN, OUTPUT); digitalWrite(LEN2_PIN, HIGH);
  pinMode(RPWM2_PIN, OUTPUT);
  pinMode(LPWM2_PIN, OUTPUT);
  ledcAttach(RPWM2_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(LPWM2_PIN, PWM_FREQ, PWM_RES);
  driveMotor2(0);


  // Setup gate motor pins
  pinMode(BIN_1, OUTPUT);
  pinMode(BIN_2, OUTPUT);


  // Setup gate encoder with pull-ups
  ESP32Encoder::useInternalWeakPullResistors = puType::up;
  gateEncoder.attachHalfQuad(ENC_A_PIN, ENC_B_PIN);
  gateEncoder.setCount(0);


  Serial.println("System ready.");
}


// Main loop: handle button, timeouts, updates
void loop() {
  // Button handling: debounce and detect rising edge
  bool reading = digitalRead(BTN_PIN);
  if (reading != lastBtnState) lastDebounce = millis();
  if (millis() - lastDebounce > DEBOUNCE_MS) {
    static bool handled = false;
    if (reading == HIGH && !handled) {
      handled = true;
      if (curState == IDLE) {
        // >>> Transition to ARMED <<<
        curState   = ARMED;
        armedStart = millis();
        digitalWrite(LED_PIN, HIGH);
        driveMotor1(desiredDuty);
        driveMotor2(desiredDuty);
        runGateCycle();
      } else {
        // >>> Transition back to IDLE <<<
        curState = IDLE;
        digitalWrite(LED_PIN, LOW);
        driveMotor1(0);
        driveMotor2(0);
        lcdIdle();
      }
    }
    if (reading == LOW) handled = false;
  }
  lastBtnState = reading;


  // 2) Auto-disarm if we've been ARMED too long
  if (curState == ARMED && millis() - armedStart >= ARMED_MS) {
    curState = IDLE;
    digitalWrite(LED_PIN, LOW);
    driveMotor1(0);
    driveMotor2(0);
    lcdIdle();
  }


  // 3) If we're in IDLE, read the pot and update LCD if duty changed
  if (curState == IDLE) {
    uint8_t d = map(analogRead(POT_PIN), 0, 4095, 0, 255);
    if (d != desiredDuty) {
      desiredDuty = d;
      lcdIdle();
    }
  }


  // 4) If ARMED, refresh the spinner animation
  if (curState == ARMED) {
    lcdArmed();
  }
}
