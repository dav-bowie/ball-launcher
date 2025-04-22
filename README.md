# Ball Launcher

A two‑motor ball‑launcher powered by an ESP32 Feather, featuring an interactive I²C LCD display and a piezo speaker that plays the iconic _Imperial March_ during launch. Ideal for robotics demos, lab assignments, or hobby projects that require a fun and visual demonstration of motor control.

What you will need to make this: 

- Dual‑motor control using two BTS7960 (HW‑039) drivers connected to separate motors.
- Potentiometer‑adjustable speed in the IDLE state, with real‑time RPM calculation and a dynamic bar graph on a 16×2 I²C LCD.
- Pushbutton‑triggered launch: pressing the button arms the motors for a fixed 30 s interval.
- Interactive LCD feedback:
  - IDLE State: shows "IDLE RPM: XXXXX" and a 16‑segment bar graph matching the current speed.
  - **Armed State: displays "Woofie launching" with a loading spinner while motors run.
- **Piezo speaker on GPIO 27 that loops the _Imperial March_ theme asynchronously during launch.
- **Automatic disarm after 30 s or by re‑pressing the button, with all systems returning to IDLE.

---

## 📦 Hardware Requirements

ESP32 Feather (HUZZAH32)    
BTS7960  Driver (HW‑039)|   
DC Motors                   
16×2 I²C LCD                
10 kΩ Potentiometer         
Pushbutton                  
Piezo Buzzer / Speaker      
Wires, breadboard, power    



| Feather GPIO | Function               | BTS7960 Pin | Notes                    |
|:------------:|------------------------|:-----------:|--------------------------|
| 26           | Motor 1 RPWM (PWM+)    | RPWM1       | 8‑bit PWM                |
| 25           | Motor 1 LPWM (PWM–)    | LPWM1       | Held LOW for forward     |
| 32           | Motor 1 R_EN           | R_EN1       | Enable HIGH              |
| 33           | Motor 1 L_EN           | L_EN1       | Enable HIGH              |
| 18           | Motor 2 RPWM (PWM–)    | LPWM2       | Reverse spin direction   |
| 19           | Motor 2 LPWM (PWM+)    | RPWM2       | PWM for motor 2          |
| 4            | Motor 2 R_EN           | R_EN2       | Enable HIGH              |
| 14           | Motor 2 L_EN           | L_EN2       | Enable HIGH              |

### ESP32 Feather → Peripherals

| Feather GPIO | Function             | Peripheral      |
|:------------:|----------------------|-----------------|
| 15           | Potentiometer (ADC)  | Wiper → ADC1_CH5|
| 12           | Pushbutton (INPUT_PULLDOWN) | BTN       |
| 13           | Onboard LED          | OLED indicator  |
| 23           | I²C SDA              | LCD SDA         |
| 22           | I²C SCL              | LCD SCL         |
| 27           | Piezo Buzzer (+)     | Speaker +       |
| GND          | Piezo Buzzer (–)     | Speaker –       |
| 5 V / GND    | Logic power          | LCD, Feather    |
| 12 V / GND   | Motor supply         | BTS7960 B+ / B– |

---

2. **Install Arduino libraries**
   - [LiquidCrystal_I2C](https://github.com/johnrickman/LiquidCrystal_I2C)

3. **Open in Arduino IDE**
   - Load `ball_launcher.ino` (or your chosen sketch name).
   - Select **Adafruit HUZZAH32** board under **Tools → Board**.
   - Choose the correct **COM port**.

4. **Upload** to your ESP32 Feather.

1. **Power up** your system: 5 V logic and 12 V motor supply.
2. **IDLE State** (default):
   - Turn the potentiometer to adjust speed.
   - Observe the **RPM** value and **bar graph** updating in real time.
3. **Arm / Launch**:
   - Press the **pushbutton** once.
   - Motors spin at the set speed.
   - The LCD displays **"Woofie launching"** with a loading spinner.
   - The piezo buzzer plays the looped **Imperial March** theme for 30 s.
4. **Disarm**:
   - Press the pushbutton again, or wait 30 s to auto‑disarm.
   - Motors stop, the buzzer silences, and the LCD reverts to IDLE.

