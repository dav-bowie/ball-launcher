/* TWO-MOTOR POT-CONTROLLED HW-039 (BTS7960) + I²C LCD + Piezo
 * ESP32 Feather (Adafruit HUZZAH32)
 * Goal is to do the following: launch a ball at selectable speed, have potentiometer set the duty-cycle/RPM, Button toggles IDLE ← → ARMED, and song or tone plays while ARMED
*/
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


/* ──────────── Pin Definitions ──────────────────────────────────── */
#define BTN_PIN      12
#define LED_PIN      13
#define POT_PIN      15


// Motor 1 (BTS7960 #1)
#define RPWM1_PIN    26
#define LPWM1_PIN    25
#define REN1_PIN     32
#define LEN1_PIN     33


// Motor 2 (BTS7960 #2)
#define RPWM2_PIN    18
#define LPWM2_PIN    19
#define REN2_PIN     4
#define LEN2_PIN     14


#define SPEAKER_PIN  27   // Piezo


/* ──────────── Global Constants ─────────────────────────────────── */
const uint32_t PWM_FREQ = 20'000;     // 20 kHz
const uint8_t  PWM_RES  = 8;          // 8-bit (0-255)
const uint16_t MAX_RPM  = 25'000;     // scale factor


/* ──────────── State Machine ────────────────────────────── */
enum State { IDLE, ARMED };
static State    curState    = IDLE;
static uint8_t  desiredDuty = 0;
static uint32_t armedStart  = 0;
static const uint32_t ARMED_MS = 30'000;   // 30 s run


/* ──────────── LCD & UI assets ───────────────── */
LiquidCrystal_I2C lcd(0x27, 16, 2);
byte fullBlock[8] = { 0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F };
const char spinner[4] = {'|','/','-','\\'};


/* ──────────── EVENT CHECKER #1 : BUTTON ISR ───────────────────────
 *  Sets a flag whenever the user presses BTN_PIN (rising edge).    */
volatile bool buttonFlag = false;
void IRAM_ATTR onButton() { buttonFlag = true; }


/* ──────────── SERVICE FUNCTIONS ────────────────────────────────── */
/* Service S1: driveMotor1() – forward rotation */
inline void driveMotor1(uint8_t duty) {
  ledcWrite(RPWM1_PIN, duty);
  ledcWrite(LPWM1_PIN, 0);
}


/* Service S2: driveMotor2() – reverse rotation (opposite pin) */
inline void driveMotor2(uint8_t duty) {
  ledcWrite(RPWM2_PIN, duty);   // opposite direction of Motor 1
  ledcWrite(LPWM2_PIN, 0);
}


/* Service S3: lcdIdle() – draw RPM and bar graph while IDLE */
void lcdIdle() {
  uint16_t rpm = (uint32_t)desiredDuty * MAX_RPM / 255;
  lcd.setCursor(0, 0);
  lcd.printf("IDLE RPM:%5u", rpm);
  int barLen = map(rpm, 0, MAX_RPM, 0, 16);
  lcd.setCursor(0, 1);
  for (int i = 0; i < 16; ++i)
    (i < barLen) ? lcd.write(byte(0)) : lcd.print(' ');
}


/* Service S4: lcdArmed() – “Woofie launching” splash + spinner */
void lcdArmed() {
  uint8_t idx = (millis() / 300) % 4;
  lcd.setCursor(0, 0);  lcd.print("Woofie launching");
  lcd.setCursor(0, 1);  lcd.print("in progress "); lcd.print(spinner[idx]); lcd.print("   ");
}


/* ──────────── Service #5: Setting the sound when button is pressed ────────────────────────────────────────────── */
#define NOTE_C5 523
#define NOTE_A4 440
#define NOTE_F4 349
#define NOTE_E4 330
#define REST      0
const int impMelody[] = {
  NOTE_A4, NOTE_A4, NOTE_A4,
  NOTE_F4, NOTE_C5, NOTE_A4,
  NOTE_F4, NOTE_C5, NOTE_A4,
  REST,    NOTE_E4, NOTE_E4,
  NOTE_E4, NOTE_F4, NOTE_C5,
  NOTE_A4, REST
};
const int impBeats[] = {
  4,8,8,  4,8,4,  4,8,4,  4,8,8,  4,8,4,  4,8
};
const int IMP_LEN = sizeof(impMelody)/sizeof(impMelody[0]);
const int TEMPO   = 90;
static int  noteIndex    = 0;
static long nextNoteTime = 0;
void updateMelody() {
  if (millis() < nextNoteTime) return;
  int note = impMelody[noteIndex];
  int dur  = 60000 / TEMPO / impBeats[noteIndex];
  (note == REST) ? noTone(SPEAKER_PIN) : tone(SPEAKER_PIN, note, dur);
  nextNoteTime = millis() + dur * 1.3;
  noteIndex = (noteIndex + 1) % IMP_LEN;
}


/* ──────────── This is for setting up the motors, button, speaker, LCD────────────────────────── */
void setup() {
  Serial.begin(115200);


  Wire.begin(23, 22);
  lcd.init(); lcd.backlight(); lcd.createChar(0, fullBlock); lcdIdle();


  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLDOWN);
  attachInterrupt(BTN_PIN, onButton, RISING);     // <-- EVENT CHECKER #1


  pinMode(REN1_PIN, OUTPUT); digitalWrite(REN1_PIN, HIGH);
  pinMode(LEN1_PIN, OUTPUT); digitalWrite(LEN1_PIN, HIGH);
  pinMode(LPWM1_PIN, OUTPUT); digitalWrite(LPWM1_PIN, LOW);
  ledcAttach(RPWM1_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(LPWM1_PIN, PWM_FREQ, PWM_RES);
  driveMotor1(0);                                 // S1


  pinMode(REN2_PIN, OUTPUT); digitalWrite(REN2_PIN, HIGH);
  pinMode(LEN2_PIN, OUTPUT); digitalWrite(LEN2_PIN, HIGH);
  pinMode(LPWM2_PIN, OUTPUT); digitalWrite(LPWM2_PIN, LOW);
  ledcAttach(RPWM2_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(LPWM2_PIN, PWM_FREQ, PWM_RES);
  driveMotor2(0);                                 // S2


  noTone(SPEAKER_PIN);
}


/* ──────────── LOOP (state machine) ─────────────────────────────── */
void loop() {


  /* EVENT CHECKER #2 : Potentiometer change (polled) ---------------*/
  if (curState == IDLE) {
    uint8_t duty = map(analogRead(POT_PIN), 0, 4095, 0, 255);
    if (duty != desiredDuty) { desiredDuty = duty; lcdIdle(); }      // S3
  }


  /* ----- Handle button edge --------------------------------------*/
  if (buttonFlag) {                        // flag set by EVENT CHECKER #1
    buttonFlag = false;
    if (curState == IDLE) {                // transition:  IDLE → ARMED
      curState   = ARMED;
      armedStart = millis();
      noteIndex  = 0; nextNoteTime = 0;
      digitalWrite(LED_PIN, HIGH);
      driveMotor1(desiredDuty);            // S1
      driveMotor2(desiredDuty);            // S2
      lcdArmed();                          // S4
    } else {                               // transition:  ARMED → IDLE
      curState = IDLE;
      digitalWrite(LED_PIN, LOW);
      driveMotor1(0); driveMotor2(0);      // S1 & S2
      noTone(SPEAKER_PIN);
      lcdIdle();                           // S3
    }
  }


  /* Auto-disarm after timeout -------------------------------------*/
  if (curState == ARMED && millis() - armedStart >= ARMED_MS) {
    curState = IDLE;
    digitalWrite(LED_PIN, LOW);
    driveMotor1(0); driveMotor2(0);         // S1 & S2
    noTone(SPEAKER_PIN);
    lcdIdle();                              // S3
  }


  /* Services active while ARMED -----------------------------------*/
  if (curState == ARMED) {
    lcdArmed();   // S4
    updateMelody(); // S5
  }
}








