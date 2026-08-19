# RC-Car — Wireless Bluetooth/Wi-Fi Controlled Robot Car

A bare-metal embedded project built on the **TI Tiva TM4C123GH6PM** (ARM Cortex-M4).
A four-wheel skid-steer robot car whose motors are driven by register-level peripheral
drivers written from the microcontroller datasheet, with movement commands delivered
wirelessly from a phone/PC over a networked link.

> **Status:** Work in progress. Core motor, timer, and UART drivers are implemented;
> the command parser, wireless bridge, and controller app are in progress. See
> [Roadmap](#roadmap).

---

## Overview

The system is a **distributed embedded design** with a clear separation of concerns:

- The **Tiva TM4C123** handles deterministic, real-time **motor control** — bare-metal,
  no HAL or driver library, everything configured directly at the register level.
- An **ESP32** (planned) handles **networking**, receiving commands over Wi-Fi/UDP and
  bridging them to the Tiva over a UART serial link.
- A **controller application** (planned, Java) sends movement commands over UDP.

```
[ Controller app ] --UDP/Wi-Fi--> [ ESP32 ] --UART--> [ Tiva TM4C123 ] --PWM--> [ Motors ]
    (phone / PC)                  (network             (real-time             (H-bridge
                                   coprocessor)         motor control)         driver)
```

Splitting networking from motion means the Tiva can guarantee timely, deterministic
motor response while the ESP32 deals with the non-deterministic network.

---

## Hardware

| Component | Notes |
|-----------|-------|
| MCU | TI Tiva C Series **TM4C123GXL** LaunchPad (TM4C123GH6PM, Cortex-M4, 50 MHz) |
| Motors | 4 × TT gear DC motors (3–6 V), driven as two sides (skid-steer) |
| Motor driver | L298N-based dual H-bridge (OSOYOO Model-X) |
| Wireless | ESP32 (Wi-Fi/UDP bridge) — *planned* |
| Chassis | 4WD robot car chassis |
| Power | Battery pack (motor supply) + regulated 3.3 V for logic |

---

## Firmware Design

All drivers are written **directly against the TM4C123 registers from the datasheet**
(no TivaWare/driverlib), targeting Keil µVision.

**Motor driver (`Motor.c` / `Motor.h`)** — Differential (skid-steer) drive using
**PWM Module 0**. The car is controlled as two logical sides via a single entry point:

```c
void drive(int16_t left, int16_t right);
```

The **sign** of each value sets direction (positive = forward, negative = reverse,
zero = stop) and the **magnitude** sets speed. All motion — forward, reverse, and
in-place pivots — falls out of this one function, so turning is simply giving the two
sides different values. A lower-level `set_side()` owns all register access for one side.

Driving the L298N splits speed from direction: **speed** is a PWM duty on the enable
pins (PB4/PB5, from PWM generator 1), and **direction** is set by GPIO on the IN pins
(PD0–PD3) — the two inputs of a side driven to opposite levels for forward/reverse, or
both low to stop.

**Timer driver (`Timer_Interrupt.c` / `.h`)** — Timer0A configured for a **1 ms periodic
interrupt** (50 MHz clock, prescaler, NVIC priority), running a user callback from the ISR.
Provides the millisecond time base used for delays and the planned command-safety timeout.

**UART driver (`UART1.c` / `.h`)** — UART1 (PB0/PB1) at 115200 8N1, polled TX/RX, used as
the serial link to the ESP32. Includes character and string I/O helpers.

### Pin assignments

| Function | Pin(s) | Peripheral |
|----------|--------|------------|
| Left speed (ENA) | PB4 | M0PWM2 |
| Right speed (ENB) | PB5 | M0PWM3 |
| Left direction (IN1/IN2) | PD3 / PD2 | GPIO |
| Right direction (IN3/IN4) | PD1 / PD0 | GPIO |
| UART RX (from ESP32) | PB0 | U1RX |
| UART TX (to ESP32) | PB1 | U1TX |

### Command protocol (planned)

Single-character commands mapped to `drive()` calls:

| Char | Action | Call |
|------|--------|------|
| `F` | Forward | `drive(+s, +s)` |
| `B` | Reverse | `drive(-s, -s)` |
| `L` | Pivot left | `drive(-s, +s)` |
| `R` | Pivot right | `drive(+s, -s)` |
| `S` | Stop | `drive(0, 0)` |

---

## Build & Flash

1. Open `RC-Car.uvprojx` in **Keil µVision**.
2. Build (F7).
3. Connect the TM4C123 LaunchPad via the **DEBUG** USB port (power switch on DEBUG),
   with the Stellaris ICDI driver installed.
4. Download/flash (F8).

---

## Roadmap

- [x] Motor driver — skid-steer differential drive (PWM Module 0)
- [x] Timer0A 1 ms interrupt tick
- [x] UART1 driver (serial link to ESP32)
- [ ] Command parser (characters → `drive()`)
- [ ] Command safety timeout / failsafe (auto-stop on comms loss)
- [ ] Interrupt-driven UART RX with ring buffer
- [ ] ESP32 Wi-Fi/UDP → UART bridge
- [ ] Java controller application (UDP)
- [ ] Ultrasonic obstacle avoidance (timer input capture)

---

## Demo
