# 🌱 IrrigateToNotIrritate™  
**Smart IoT Plant Care System using FreeRTOS & ESP32**
# [Project Link](https://docs.google.com/document/d/1TReUOiwNaZuH_LoJ0fddSMDoVMS7u1vT/edit)
<img width="999" height="1219" alt="image" src="https://github.com/user-attachments/assets/22832ed1-f583-421b-a21d-1f0b62b8aa1e" width="100"/>
<img width="1000" height="1002" alt="image" src="https://github.com/user-attachments/assets/18c5eaaa-494b-4dde-b0f0-2b588b6eaa40" width="100"/>


## 📌 Overview
IrrigateToNotIrritate™ is a real-time, IoT-enabled smart irrigation system designed to automate plant care. It monitors environmental conditions and dynamically adjusts watering using embedded systems and cloud integration.

The system combines **FreeRTOS (MCXC444)** for deterministic task scheduling with an **ESP32** for connectivity, enabling a scalable and intelligent plant care solution.

---

## 🚀 Features
- 🌱 Automated **soil moisture monitoring**
- 🌦️ **Weather-adaptive watering** (via OpenWeatherMap API)
- 📲 **Real-time Telegram alerts** (watering + low tank warnings)
- 🔘 **Manual override button** (interrupt-driven)
- 💧 **Water level detection**
- 💡 Smart LED feedback with **finite state machine (FSM)**
- ⚡ **Dynamic sensor power control** (extends sensor lifespan)

---

## 🏗️ System Architecture

### Hardware
- **MCXC444** – Real-time control (FreeRTOS)
- **ESP32** – IoT connectivity + API handling
- Sensors:
  - Soil moisture (FC-28)
  - Water level (HW-038)
  - Light sensor (KY-018)
- Actuators:
  - SG90 Servo motor (water valve)
  - Onboard LEDs (status indication)

### Software
- **FreeRTOS multi-tasking system (6 tasks):**
  - Soil moisture polling  
  - Alert handling  
  - LED control  
  - UART transmit  
  - UART receive  
  - Manual override  

- **Synchronization tools:**
  - Queues (data transfer)
  - Semaphores (event triggering)
  - Mutex (resource protection)

---

## 🔌 Communication Protocol

- **UART (8N1, 9600 baud)** between MCXC444 and ESP32  
- Custom packet format using `< >` delimiters  


---

## 🌐 IoT Integration
- **OpenWeatherMap API** → Adjust watering thresholds based on weather  
- **Telegram Bot API** → Sends real-time notifications with timestamps  

---

## ⚙️ Key Design Concepts
- **RTOS Scheduling** → Deterministic, concurrent task execution  
- **Hardware-Software Co-design** → Split responsibilities across MCXC444 & ESP32  
- **FSM-based LED control** → Maintains consistent visual states  
- **Interrupt-driven design** → Responsive manual override  

---

## 🧠 Challenges & Solutions
- **Heap memory exhaustion (FreeRTOS)**  
  → Increased heap allocation for stable multi-tasking  

- **UART interrupt noise**  
  → Resolved with proper common ground connection  

- **Telegram API limitations (GET requests)**  
  → Migrated to HTTP POST with JSON payload  

---

## 📊 Skills Demonstrated
- Embedded Systems  
- Real-Time Operating Systems (FreeRTOS)  
- IoT Integration (ESP32, APIs)  
- UART Communication Protocol Design  
- Hardware-Software Integration  
- Debugging & System Optimization  

---

## 👨‍💻 Contributors
- Gan Andrew Hoa Thien  
- Pang Ang Sheng Asher  
- Zheng Kaiwen  

---

## 📄 License
*(Optional — add if needed)*
