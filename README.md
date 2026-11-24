# RC-Car — Wireless Bluetooth/Wi-Fi Controlled Robot Car

A bare-metal embedded project built on the **TI Tiva TM4C123GH6PM** (ARM Cortex-M4).
A four-wheel skid-steer robot car whose motors are driven by register-level peripheral
drivers written from the microcontroller datasheet, with movement commands delivered
wirelessly from a phone/PC over a networked link.

> **Status:** Work in progress. On the Tiva, the motor, timer, UART, and RGB LED drivers and
> the UART command parser are implemented, and the motors have been verified on the car
> (`main.c` currently runs a motor bring-up test). The ESP32 bridge firmware is under way:
> it connects to Wi-Fi and gets an IP address; the UDP receiver and the UART link to the
> Tiva are next. The controller app is still to come. See [Roadmap](#roadmap).

---

## Overview

The system is a **distributed embedded design** with a clear separation of concerns:

- The **Tiva TM4C123** handles deterministic, real-time **motor control** — bare-metal,
  no HAL or driver library, everything configured directly at the register level.
- An **ESP32-S3** (in progress) handles **networking**, receiving commands over Wi-Fi/UDP
  and bridging them to the Tiva over a UART serial link.
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
| Wireless | **Arduino Nano ESP32** (u-blox NORA-W106 module, ESP32-S3, 16 MB flash, 8 MB PSRAM) — Wi-Fi/UDP bridge, *in progress* |
| Chassis | 4WD robot car chassis |
| Power | Battery pack (motor supply) + regulated 3.3 V for logic |

---

## Tiva Firmware Design

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

`Motor_Init()` waits on the PWM0 peripheral-ready bit (`PRPWM`) after enabling the module
clock, so the first PWM register access cannot fault on a cold start.

**Timer driver (`Timer_Interrupt.c` / `.h`)** — Timer0A configured for a **1 ms periodic
interrupt** (50 MHz clock, prescaler, NVIC priority), running a user callback from the ISR.
Provides the millisecond time base used for delays and the planned command-safety timeout.

**UART driver (`UART1.c` / `.h`)** — UART1 (PB0/PB1) at 115200 8N1, polled TX/RX, used as
the serial link to the ESP32. Includes character and string I/O helpers.

**Command parser (`Command_Parser.c` / `.h`)** — Reads one line from UART1, parses it into
two signed side speeds, and passes them to `drive()`. See
[Command protocol](#command-protocol).

**RGB LED driver (`GPIO.c` / `GPIO.h`)** — Drives the LaunchPad's on-board RGB LED
(PF1–PF3), used as a start-up status indicator.

### Pin assignments

| Function | Pin(s) | Peripheral |
|----------|--------|------------|
| Left speed (ENA) | PB4 | M0PWM2 |
| Right speed (ENB) | PB5 | M0PWM3 |
| Left direction (IN1/IN2) | PD3 / PD2 | GPIO |
| Right direction (IN3/IN4) | PD1 / PD0 | GPIO |
| UART RX (from ESP32) | PB0 | U1RX |
| UART TX (to ESP32) | PB1 | U1TX |
| Status LED (red / blue / green) | PF1 / PF2 / PF3 | GPIO |

### Command protocol

Each command is one line of text: two signed decimal side speeds separated by a comma and
terminated by a carriage return.

```
<left>,<right>\r
```

The sign of each value sets that side's direction and the magnitude sets its speed as a
PWM duty in counts, out of the PWM period (16000 as configured in `main.c`). Values at or
above the period are clamped just below it.

| Message | Action | Call |
|---------|--------|------|
| `8000,8000` | Forward at half speed | `drive(8000, 8000)` |
| `-8000,-8000` | Reverse at half speed | `drive(-8000, -8000)` |
| `-8000,8000` | Pivot left | `drive(-8000, 8000)` |
| `8000,-8000` | Pivot right | `drive(8000, -8000)` |
| `0,0` | Stop | `drive(0, 0)` |

### Bring-up test and status LED

`main.c` currently runs a motor bring-up test in place of the command parser (the
`parser()` call is commented out). The on-board LED shows how far start-up got:

| LED | Meaning |
|-----|---------|
| Red | `main()` entered; motor driver initializing |
| Blue | Motor driver ready; UART and timer initializing |
| Green | All initialization complete; main loop running |

An LED that stays red or blue means that stage did not complete. Once green, the loop
drives forward, reverse, pivot left, pivot right, then stop, holding each for one second.

---

## ESP32 Bridge Firmware (`ESP-Bridge/`)

The ESP32 side is a separate **ESP-IDF** project (v6.0.2, target `esp32s3`) in the
`ESP-Bridge/` folder, built with the ESP-IDF extension for VS Code. Its job is to receive
movement commands over Wi-Fi/UDP and forward them unchanged to the Tiva over UART, in the
same `<left>,<right>\r` format described in [Command protocol](#command-protocol).

| File | Purpose | Status |
|------|---------|--------|
| `main/main.c` | Initializes NVS flash, then brings up Wi-Fi | Working |
| `main/wifi_ap.c` / `.h` | Connects to a Wi-Fi network in **station mode** (joins a router), retrying up to 5 times | Verified on hardware |
| `main/uart_link.c` / `.h` | UART1 at 115200 8N1 to the Tiva; `uart_link_send(left, right)` formats and sends one command | Written, not yet called |
| `main/udp_server.c` / `.h` | Receives UDP command packets | Placeholder |
| `main/wifi_password.h` | Wi-Fi network name and password | **Git-ignored**, create locally |
| `sdkconfig.defaults` | Routes the console to USB Serial/JTAG so logs reach the PC | — |

> Despite its name, `wifi_ap` currently uses station mode, not access-point mode.

### ESP32 ↔ Tiva wiring

| Nano ESP32 pin | GPIO | Tiva pin | Signal |
|----------------|------|----------|--------|
| TX1 | GPIO43 | PB0 (U1RX) | Commands to the Tiva |
| RX0 | GPIO44 | PB1 (U1TX) | Tiva to ESP32 (unused for now) |
| GND | — | GND | Common ground (required) |

Both boards use 3.3 V logic, so no level shifting is needed. On the car, the Nano can be
powered from the battery through **VIN** (6–21 V); **VBUS** only supplies 5 V while USB is
connected.

### Wi-Fi credentials

`main/wifi_password.h` is listed in `ESP-Bridge/.gitignore` so credentials are never
committed. After cloning, create it with two definitions:

```c
#define wifi_id     "your-network-name"
#define wifi_pass   "your-password"
```

The ESP32-S3 supports **2.4 GHz** networks only.

---

## Build & Flash

### Tiva (Keil)

1. Open `RC-Car.uvprojx` in **Keil µVision**.
2. Build (F7).
3. Connect the TM4C123 LaunchPad via the **DEBUG** USB port (power switch on DEBUG),
   with the Stellaris ICDI driver installed.
4. Download/flash (F8).

### ESP32 (ESP-IDF)

1. Open the `ESP-Bridge/` folder on its own in VS Code (not the repo root), with the
   ESP-IDF extension installed and ESP-IDF v6.0.2 selected.
2. Create `main/wifi_password.h` (see [Wi-Fi credentials](#wi-fi-credentials)).
3. Build with **ESP-IDF: Build Your Project**.
4. For the first flash, or whenever the board is not running ESP-IDF firmware, enter
   download mode: connect **B1** to **GND**, press **RESET**, then select the COM port that
   appears.
5. Flash with **ESP-IDF: Flash Your Project** using the **UART** flash method.
6. Remove the B1 jumper, press **RESET**, and open **ESP-IDF: Monitor Device**. A
   successful start prints `wifi station: got ip:<address>`; that address is where UDP
   commands are sent.

> Flashing ESP-IDF firmware replaces the Arduino bootloader on the Nano ESP32. Arduino
> documents how to restore it if the board is needed for Arduino sketches again.

---

## Roadmap

- [x] Motor driver — skid-steer differential drive (PWM Module 0)
- [x] Timer0A 1 ms interrupt tick
- [x] UART1 driver (serial link to ESP32)
- [x] Command parser (`left,right` text line → `drive()`)
- [x] RGB LED start-up status indicator
- [x] Motor bring-up test on the car
- [ ] Re-enable the command parser in `main.c` after bring-up
- [ ] Command safety timeout / failsafe (auto-stop on comms loss)
- [ ] Interrupt-driven UART RX with ring buffer
- [x] ESP32 bridge project set up (ESP-IDF, Nano ESP32 / ESP32-S3)
- [x] ESP32 Wi-Fi connection (station mode)
- [ ] ESP32 UART link to the Tiva tested end to end
- [ ] ESP32 UDP command receiver
- [ ] Forward UDP commands to the Tiva, with an ESP32-side failsafe (send `0,0` on silence)
- [ ] Disable Wi-Fi power saving for lower command latency
- [ ] Java controller application (UDP)
- [ ] Ultrasonic obstacle avoidance (timer input capture)

---

## Demo
