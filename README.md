# JamGuard 🛡️⚙️
### Smart Motor Monitoring and Automatic Protection System

---

**JamGuard** is an educational and functional prototype designed to monitor DC motor rotation in real-time, detect sustained stalls or mechanical jams, and trigger immediate hardware and software-level protections[cite: 2, 3]. Developed as a first-year B.Tech engineering project at VIT Vellore[cite: 2], it bridges embedded control, magnetic sensing, motor driver safety, and local IoT web dashboards without relying on external cloud infrastructure.

---

## 👥 Project Team
* **Patnala Subhaom Kumar Patro** (`26BEC0268`)
* **Ruchir Nilesh Patil** (`26BEC0187`)
* **Aarav Ghiya** (`26BEC0169`)
* **Atharv Saxena** (`26BEC0387`)

**Guided by:** Dr. KVN Kavitha (School of Electronics Engineering - SENSE)

---

## 📌 Abstract & Problem Statement
In automated systems like conveyor belts, packaging machinery, robotics, and automated gates, a motor can remain electrically energized while its mechanical shaft stalls or jams. Continuous stalling without rotation causes electrical power to convert into destructive heat within the windings, reducing efficiency to zero. 

JamGuard solves this by continuously observing rotation via magnetic pulses, computing RPM, applying intelligent timeout logic, disabling the motor during faults, and alerting operators locally.

---

## 🛠️ System Architecture & Working Principle
1. **Sensing:** A 5 mm magnet attached to the DC motor passes a Hall-effect sensor, generating pulse signals on each revolution.
2. **Processing & Control:** An **Arduino Uno R3** reads the pulse frequency, calculates RPM, and runs safety timeout logic:
   * **3-second Startup Grace Period:** Ignores initial spin-up transients.
   * **1.5-second No-Pulse Timeout:** Detects sustained loss of rotation and trips the protection mechanism.
3. **Protection & Actuation:** Upon detecting a jam, the system disables the motor via an **L298N driver**, activates an onboard LED and buzzer, and waits for a manual reset push-button.
4. **Local Dashboard:** The Arduino transmits status over UART (using proper 5V-to-3.3V level shifting/voltage dividers) to an **ESP32 Dev Module**, which hosts a local Wi-Fi web dashboard displaying live RPM, motor status, and fault alerts.

---

## 🧰 Hardware & Software Stack

| Component / Tool | Role / Function |
| :--- | :--- |
| **Arduino Uno R3** | Sensor signal acquisition, RPM calculation, and local protection logic |
| **ESP32 Dev Module** | UART serial data receiver and local Wi-Fi web server dashboard |
| **Hall-effect Sensor + 5mm Magnet** | Non-contact magnetic pulse detection per revolution |
| **L298N Motor Driver** | Isolates and switches high-current DC motor power |
| **Peripherals** | LED, Piezo Buzzer, Reset Push-Button, Current-limiting resistors (5% tolerance) |
| **Software IDE** | Arduino IDE (C/C++ firmware development) |

---

## 🔌 Connection Guidelines
* **Common Ground:** Ensure all modules (Arduino, ESP32, L298N, and sensors) share a common ground (`GND`).
* **Level Shifting:** Always use a voltage divider or 5V-to-3.3V level shifter on the UART TX line from the Arduino Uno to the ESP32 input to protect the ESP32 pins.

---

## 🚀 Future Scope
* Implementing digital filtering or debouncing for enhanced pulse signal stability.
* Adding adaptive monitoring intervals and multi-motor status aggregation.
* Upgrading to industrial-grade sensors and isolated interfaces for rugged deployment.

---

## 📜 License
This project is developed for academic purposes under the Department of SENSE, Vellore Institute of Technology (VIT), Vellore[cite: 2]. Feel free to reference or build upon this work with appropriate attribution.
