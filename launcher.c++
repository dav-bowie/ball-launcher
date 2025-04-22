/********************************************************************
 This code is what I used in order to fulfill the following actions: 
 (1) Launch a ball at different speeds
 (2) Adjust speeds via potentiometer
 (3) Sound played when button is pressed 
 (4) Button to change states from idle to armed state
 (5) Button to turn off and on the motor and sound 
 ********************************************************************/


#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


// ─── Pin Definitions ───────────────────────────────────────────────
#define BTN_PIN      12
#define LED_PIN      13
#define POT_PIN      15


// Motor 1
#define RPWM1_PIN    26
#define LPWM1_PIN    25
#define REN1_PIN     32
#define LEN1_PIN     33


// Motor 2
#define RPWM2_PIN    18
#define LPWM2_PIN    19
#define REN2_PIN     4
#define LEN2_PIN     14


// Piezo speaker
#define SPEAKER_PIN  27


// ─── PWM Parameters ────────────────────────────────────────────────
const uint32_t PWM_FREQ = 20000;   // 20 kHz carrier
const uint8_t  PWM_RES  = 8;       // 0–255 duty
const uint16_t MAX_RPM  = 25000;   // full‑scale at PWM=255


// ─── State Machine ─────────────────────────────────────────────────
enum State { IDLE, ARMED };
static State    curState    = IDLE;
static uint8_t  desiredDuty = 0;
static uint32_t armedStart  = 0;
static const uint32_t ARMED_MS = 30'000;  // 30 s run


volatile bool buttonFlag = false;
void IRAM_ATTR onButton() { buttonFlag = true; }


LiquidCrystal_I2C lcd(0x27, 16, 2);


// ─── Custom Char: full‑block for bar graph ─────────────────────────
byte fullBlock[8] = {
  0x1F,0x1F,0x1F,0x1F,
  0x1F,0x1F,0x1F,0x1F
};


// ─── Spinner chars for ARMED mode ─────────────────────────────────
const char spinner[4] = {'|','/','-','\\'};


inline void driveMotor1(uint8_t duty) {
  ledcWrite(RPWM1_PIN, duty);
  ledcWrite(LPWM1_PIN, 0);
}
inline void driveMotor2(uint8_t duty) {
  ledcWrite(RPWM2_PIN, 0);
  ledcWrite(LPWM2_PIN, duty);
}


// ─── LCD: IDLE State with RPM + dynamic bar graph ─────────────────
void lcdIdle() {
  // compute RPM
  uint16_t rpm = (uint32_t)desiredDuty * MAX_RPM / 255;


  // Line 1: show state + numeric RPM
  lcd.setCursor(0, 0);
  lcd.printf("IDLE RPM:%5u", rpm);


  // Line 2: bar graph across 16 columns
  int barLen = map(rpm, 0, MAX_RPM, 0, 16);
  lcd.setCursor(0, 1);
  for (int i = 0; i < 16; i++) {
    if (i < barLen) lcd.write(byte(0));
    else            lcd.print(' ');
  }
}


// ─── LCD: ARMED State with “Woofie launching” + loading spinner ────
void lcdArmed() {
  uint8_t idx = (millis() / 300) % 4;
  lcd.setCursor(0, 0);
  lcd.print("Woofie launching");
  lcd.setCursor(0, 1);
  lcd.print("in progress ");
  lcd.print(spinner[idx]);
  lcd.print("   ");
}


// ─── Imperial March melody ─────────────────────────────────────────
// Note definitions
#define NOTE_C5  523
#define NOTE_A4  440
#define NOTE_F4  349
#define NOTE_E4  330
#define REST     0


// Melody and rhythm arrays
const int impMelody[] = {
  NOTE_A4, NOTE_A4, NOTE_A4,
  NOTE_F4, NOTE_C5, NOTE_A4,
  NOTE_F4, NOTE_C5, NOTE_A4,
  REST,    NOTE_E4, NOTE_E4,
  NOTE_E4, NOTE_F4, NOTE_C5,
  NOTE_A4, REST
};
const int impBeats[] = {
  4,8,8,
  4,8,4,
  4,8,4,
  4,8,8,
  4,8,4,
  4,8
};
const int IMP_LENGTH = sizeof(impMelody) / sizeof(impMelody[0]);
const int TEMPO = 90;  // bpm


static int   noteIndex     = 0;
static long  nextNoteTime  = 0;


// play next note if it’s time (non‑blocking)
void updateMelody() {
  if (millis() < nextNoteTime) return;
  int note = impMelody[noteIndex];
  int dur  = 60000 / TEMPO / impBeats[noteIndex];
  if (note == REST) noTone(SPEAKER_PIN);
  else              tone(SPEAKER_PIN, note, dur);
  nextNoteTime = millis() + dur * 1.3;
  noteIndex = (noteIndex + 1) % IMP_LENGTH;
}


void setup() {
  Serial.begin(115200);


  // LCD init + custom char
  Wire.begin(23, 22);
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, fullBlock);
  lcdIdle();


  // button + LED
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLDOWN);
  attachInterrupt(BTN_PIN, onButton, RISING);


  // Motor 1 HW‑039
  pinMode(REN1_PIN, OUTPUT); digitalWrite(REN1_PIN, HIGH);
  pinMode(LEN1_PIN, OUTPUT); digitalWrite(LEN1_PIN, HIGH);
  pinMode(LPWM1_PIN, OUTPUT); digitalWrite(LPWM1_PIN, LOW);
  ledcAttach(RPWM1_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(LPWM1_PIN, PWM_FREQ, PWM_RES);
  driveMotor1(0);


  // Motor 2 HW‑039
  pinMode(REN2_PIN, OUTPUT); digitalWrite(REN2_PIN, HIGH);
  pinMode(LEN2_PIN, OUTPUT); digitalWrite(LEN2_PIN, HIGH);
  pinMode(LPWM2_PIN, OUTPUT); digitalWrite(LPWM2_PIN, LOW);
  ledcAttach(RPWM2_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(LPWM2_PIN, PWM_FREQ, PWM_RES);
  driveMotor2(0);


  // ensure speaker silent
  noTone(SPEAKER_PIN);
}


void loop() {
  // handle button press → toggle IDLE/ARMED
  if (buttonFlag) {
    buttonFlag = false;
    if (curState == IDLE) {
      // ARM
      curState    = ARMED;
      armedStart  = millis();
      noteIndex   = 0;
      nextNoteTime= 0;
      digitalWrite(LED_PIN, HIGH);
      driveMotor1(desiredDuty);
      driveMotor2(desiredDuty);
      lcdArmed();
    } else {
      // DISARM
      curState = IDLE;
      digitalWrite(LED_PIN, LOW);
      driveMotor1(0);
      driveMotor2(0);
      noTone(SPEAKER_PIN);
      lcdIdle();
    }
  }


  // auto‑disarm on timeout
  if (curState == ARMED && millis() - armedStart >= ARMED_MS) {
    curState = IDLE;
    digitalWrite(LED_PIN, LOW);
    driveMotor1(0);
    driveMotor2(0);
    noTone(SPEAKER_PIN);
    lcdIdle();
  }


  // IDLE: adjust pot → update display
  if (curState == IDLE) {
    uint8_t duty = map(analogRead(POT_PIN), 0, 4095, 0, 255);
    if (duty != desiredDuty) {
      desiredDuty = duty;
      lcdIdle();
    }
  }


  // ARMED: refresh LCD + play melody
  if (curState == ARMED) {
    lcdArmed();
    updateMelody();
  }
}




