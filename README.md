## Table of Contents

- [Project Overview](#project-overview)
- [Project Structure](#project-structure)
- [Hardware Components](#hardware-components)
- [Circuit Wiring](#circuit-wiring)
- [Distance Zones](#distance-zones)
- [How It Works](#how-it-works)
- [Interrupt Architecture](#interrupt-architecture)
- [Serial Monitor Output](#serial-monitor-output)
- [Simulation (SimulIDE)](#simulation-simulide)
- [Flashing to Real Hardware](#flashing-to-real-hardware)
- [Configuration](#configuration)

---

## Project Overview

| Property         | Details                          |
|------------------|----------------------------------|
| Platform         | Arduino Uno (ATmega328P)         |
| Language         | C++ / Arduino (.ino)             |
| Simulator        | SimulIDE v1.1.0-SR1              |
| Sensor           | HC-SR04 Ultrasonic               |
| Output           | Red LED (variable blink speed)   |
| Danger Threshold | 5 cm                             |
| Serial Baud Rate | 9600                             |
| Timer Resolution | 1 µs (Timer1, Prescaler 8)       |

The system continuously measures the distance between the sensor and the nearest object. As the object gets closer, the LED blinks faster to warn the user — simulating a real-world proximity alarm (e.g., parking sensor, obstacle avoidance).

---

## Project Structure

```
empeded project/
├── project.ino       # Arduino sketch — all logic, ISRs, and zone handling
└── project.sim1      # SimulIDE circuit file — full wiring layout
```

---

## Hardware Components

| Component       | Quantity | Notes                             |
|-----------------|----------|-----------------------------------|
| Arduino Uno     | 1        | ATmega328P @ 16 MHz               |
| HC-SR04 Sensor  | 1        | Trig on D12, Echo on D8           |
| Red LED         | 1        | Connected to D7                   |
| 220 Ω Resistor  | 1        | Current-limiting resistor for LED |

---

## Circuit Wiring

| Arduino Pin | Connected To       | Purpose                       |
|-------------|--------------------|-------------------------------|
| D12         | HC-SR04 TRIG       | Sends 10 µs trigger pulse     |
| D8          | HC-SR04 ECHO       | Receives echo pulse (ISR)     |
| D7          | LED Anode (+)      | LED control (blink output)    |
| GND         | Resistor → LED (−) | Completes LED circuit         |

**LED Circuit:** D7 → LED anode → LED cathode → 220Ω resistor → GND

> The 220Ω resistor limits current to a safe ~14 mA given a ~3V LED forward voltage on a 5V line.

---

## Distance Zones

| Zone | Name       | Distance Range  | Blink Delay | Serial Message       |
|------|------------|-----------------|-------------|----------------------|
| 1    | Safe       | 35 – 50 cm      | 600 ms      | safe ZONE            |
| 2    | Take Care  | 23 – 32 cm      | 500 ms      | take care ZONE !!!   |
| 3    | Warning    | 14 – 20 cm      | 400 ms      | warning ZONE         |
| 4    | Danger     | 5 – 11 cm       | 200 ms      | DANGER ZONE          |
| 0    | Path Clear | (default/other) | LED OFF     | Path Clear           |

> **Faster blink = closer object.** In the Danger Zone, the LED blinks every 200 ms — five times per second.

---

## How It Works

### 1. Trigger Pulse

The `triggerUltrasonic()` function fires a clean 10 µs HIGH pulse on the TRIG pin, causing the HC-SR04 to emit an 8-cycle ultrasonic burst at 40 kHz.

```cpp
void triggerUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
}
```

### 2. Echo Measurement

When the burst reflects off an object and returns, the ECHO pin goes HIGH for a duration proportional to distance. This is captured via hardware interrupts.

```
distance (cm) = pulse_duration (µs) / 58
```

*(Speed of sound ~343 m/s; round trip halved = 58 µs/cm)*

### 3. Simulated Distance Loop

In the current build, `loop()` runs a **simulated sequence** from 50 cm down to 5 cm (steps of 3 cm) to demonstrate all four zones in SimulIDE without live sensor input. The real ISR-based measurement code is present but commented out.

### 4. Zone Classification & LED Response

Each distance is mapped to a zone via `switch`, and `blinkLED(delayTime)` is called with the corresponding interval:

```cpp
void blinkLED(int delayTime) {
  digitalWrite(LED_PIN, HIGH);
  delay(delayTime);
  digitalWrite(LED_PIN, LOW);
  delay(delayTime);
}
```

---

## Interrupt Architecture

### Pin Change Interrupt (PCINT0 — D8 / ECHO pin)

Triggers on any logic change on D8. The ISR captures rising and falling edges:

- **Rising edge** → record `TCNT1` as `startTime`, reset `overflowCount`
- **Falling edge** → record `TCNT1` as `endTime`, compute total duration

```cpp
ISR(PCINT0_vect) {
  if (digitalRead(ECHO_PIN)) {
    startTime = TCNT1;
    overflowCount = 0;
    waitingForRising = false;
  } else {
    endTime = TCNT1;
    pulseDuration = ((uint32_t)overflowCount << 16) + endTime - startTime;
    measurementReady = true;
    waitingForRising = true;
  }
}
```

### Timer1 Overflow Interrupt (TIMER1_OVF_vect)

Timer1 is a 16-bit timer. With prescaler 8 @ 16 MHz it ticks every 0.5 µs and overflows every 32.768 ms. The overflow ISR extends the measurable range beyond 16 bits:

```cpp
ISR(TIMER1_OVF_vect) {
  overflowCount++;
}
```

Final duration: `pulseDuration = (overflowCount × 65536) + endTime − startTime`

### Timer1 Setup

```cpp
TCCR1B |= (1 << CS11);    // Prescaler = 8
TIMSK1 |= (1 << TOIE1);   // Overflow interrupt
PCICR  |= (1 << PCIE0);   // Pin change interrupt group 0
PCMSK0 |= (1 << PCINT0);  // Watch D8
sei();
```

---

## Serial Monitor Output

Open Serial Monitor at **9600 baud**:

```
Distance: 50 cm
safe ZONE
--------------------
Distance: 32 cm
take care ZONE !!!
--------------------
Distance: 20 cm
warning ZONE
--------------------
Distance: 8 cm
DANGER ZONE
--------------------
```

---

## Simulation (SimulIDE)

### Components

| Component   | SimulIDE ID  | Details                           |
|-------------|--------------|-----------------------------------|
| Arduino Uno | Uno-1        | ATmega328P @ 16 MHz, loads .hex   |
| Red LED     | Led-2        | 1.8V threshold, 30 mA max         |
| Resistor    | Resistor-3   | 220 Ω                             |
| HC-SR04     | SR04-4       | TRIG on D12, ECHO on D8           |

### Steps

1. Install [SimulIDE](https://simulide.com/) v1.1.0 or later.
2. Open `project.sim1`.
3. Compile `project.ino` in Arduino IDE — note the `.hex` output path.
4. In SimulIDE, right-click the Uno → set **Program** path to your `.hex`.
5. Press **Play** — watch the LED blink and check the serial panel.

> The `.sim1` references a hardcoded path (`C:/Users/Andalus/...`). Update it after recompiling on your machine.

---

## Flashing to Real Hardware

1. Open `project.ino` in Arduino IDE (v1.8+ or v2.x).
2. Select **Board:** Arduino Uno and the correct **Port**.
3. Uncomment the real measurement block in `loop()`:

```cpp
if (measurementReady) {
  noInterrupts();
  uint32_t duration = pulseDuration;
  measurementReady = false;
  interrupts();
  float distance = duration / 58.0;
  // Add zone logic here using the real distance value
}
```

4. Comment out the simulated distance `for` loop.
5. Click **Upload** and open Serial Monitor at 9600 baud.

---

## Configuration

| Constant          | Default | Description                               |
|-------------------|---------|-------------------------------------------|
| `TRIG_PIN`        | `12`    | Pin connected to HC-SR04 TRIG             |
| `ECHO_PIN`        | `8`     | Pin connected to HC-SR04 ECHO             |
| `LED_PIN`         | `7`     | Pin connected to LED anode                |
| `DANGER_DISTANCE` | `5`     | Minimum distance (cm) — defined, unused   |

---




author
ahmad sobeh
ahmadaymansobeh@gmail.com
