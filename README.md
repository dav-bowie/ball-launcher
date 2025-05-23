# 🐾 Woofie: Assistive Ball Launcher and Treat System for Dogs  
### 🥈 Runner-Up – Most Innovative Design (UC Berkeley ME Mechatronics 2025)

**Woofie** is an interactive, multi-subsystem mechatronic device designed to empower pet owners—especially those with limited mobility—by automating a fetch-and-reward experience for dogs. Developed in UC Berkeley’s ME 102B Capstone, the system integrates ball launching, treat dispensing, and responsive feedback into a fully functional product.

---

## 🧠 Highlights
- Dual-motor launcher with **adjustable speed** control via potentiometer
- IR-triggered **dual treat dispensers** using rotary encoders and motor control
- **Interactive LCD display** for user feedback and real-time RPM display
- **Asynchronous audio playback** via piezo buzzer (Imperial March 🎵)
- **Two-state system control** (IDLE, ARMED) with auto-disarm logic

---

## 🎥 Demo
[▶️ Watch the Full Project in Action](https://drive.google.com/file/d/1Dk6wdNfHpoLyrY7RZ_D1KdSOoXZ06RKC/view)

---

## 📐 Mechanical & Electrical Design
| Subsystem      | Features |
|----------------|----------|
| **Launcher**   | Dual flywheel mechanism, potentiometer-based RPM adjustment, LCD with real-time bar graph |
| **Gate**       | Gate actuated by encoder-driven motor with timed control and debounce |
| **Treat Dispensers** | IR sensors trigger rotary dispensers for two treat types (kibble and milkbone) |
| **Housing**    | Custom CAD-designed modular frame with integrated electronics tray |

Schematic & CAD images available [here](https://app.cirkitdesigner.com/project/befecfd9-77df-4d1e-b138-91273988c8ff)  
Bill of Materials: [Google Sheets BOM](https://docs.google.com/spreadsheets/d/1K9LUF71FqHe86PqtXvVdi28Xar9T9IM3ObvhlEvTqNQ/edit)

---

## 💻 System Architecture

### ⚙️ Ball Launcher Controls (ESP32 Feather)
```cpp

IDLE:  // Shows RPM on LCD, updates speed via potentiometer
ARMED: // Launches for 30s, plays Imperial March, shows spinner
🔌 Hardware Pin Mapping
Motor Control (via BTS7960)
Feather GPIO	Motor Function	Notes
26 / 25	Motor 1 RPWM / LPWM	Forward rotation
32 / 33	Motor 1 EN pins	Enable HIGH
18 / 19	Motor 2 RPWM / LPWM	Second flywheel
4 / 14	Motor 2 EN pins	Enable HIGH

Peripherals
Feather GPIO	Function	Peripheral
15	Potentiometer (ADC)	Speed dial
12	Pushbutton (input)	Launch/arm
13	LED	Status
23 / 22	I²C SDA / SCL	LCD
27	Piezo Buzzer	Audio

⚒️ How to Run It
Wire components as shown above

Install Arduino libraries:

LiquidCrystal_I2C

Upload code to your ESP32 Feather via Arduino IDE

Power the board (5 V for logic, 12 V for motors)

System Flow:
IDLE → Adjust speed via pot, view RPM + bar graph

Button Press → ARMED: motors spin, audio plays, gate opens

Auto-disarm after 30s or second button press → return to IDLE

🛠 Engineering Takeaways
Multi-subsystem coordination through modular FSMs

Real-time control with asynchronous audio and UI

Precision motor control via rotary encoders

Integrated human-centered design (LCD/UI/audio)

Collaborative project management and subsystem integration

🧑‍💼 Why It Matters for Recruiters
This project demonstrates:

Mechanical engineering: CAD, system prototyping, tolerancing

Embedded software: FSM logic, PWM, interrupts, sensor handling

Product management: Team coordination, subsystem deadlines, user-centered design

Technical PM & SWE: Hardware-software integration, debugging, and delivery under tight constraints

🙌 Team
David Bui – Embedded software & system integration

Dylan Anacleto-Black, Eduardo Barajas, Evelyne Morisseau, Anisa Torres – Subsystem design, manufacturing, testing
