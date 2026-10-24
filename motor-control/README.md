# Motor control — Magnetic Carousel (TYPT/IYPT 2026 Problem 16)

Spins the magnet disc at a steady, known speed (0–600 RPM, fine control at 50–200 RPM), supports slow ramps for hysteresis runs, and logs set vs. measured speed (RPM and rad/s) as CSV.

Motor: 17HS4401 NEMA 17 stepper (1.8°, 1.7 A, 0.45 N·m, 5 mm shaft) with a TMC2209 driver at 16× microstepping (MS1 = MS2 = HIGH), StealthChop. Real disc speed is measured by an IR sensor reading one mark per revolution.

Two versions:
- `arduino_carousel/` — Arduino Nano/Uno makes the step pulses with Timer1 (pin 9). Control it over USB serial at 115200 baud from a PC or the Pi.
- `pi_carousel/` — Raspberry Pi 4 only, using pigpio hardware PWM on GPIO 18 for jitter-free step pulses.

## Commands (both versions)
- `S <rpm>` — go to rpm at 100 RPM/s
- `R <rpm> <sec>` — ramp linearly to rpm over sec seconds
- `X` — stop now
- `Q` — quit (Pi version only)

Negative rpm reverses direction.

## Wiring
| Signal | Arduino | Pi 4 (BCM) |
|---|---|---|
| TMC2209 STEP | D9 | GPIO 18 |
| TMC2209 DIR | D8 | GPIO 23 |
| TMC2209 EN | D7 | GPIO 24 |
| IR sensor out | D2 | GPIO 17 |

Common ground between controller, driver and sensor. On the Pi, power the driver logic (VIO) and the IR sensor from 3.3 V (or use a level shifter) — 5 V on a Pi GPIO can damage it.

## Build and run (Pi version)
```
sudo apt install pigpio
g++ carousel.cpp -o carousel -lpigpio -lpthread -lrt
sudo systemctl stop pigpiod   # only if it is running
sudo ./carousel run1.csv
```
pigpio supports the Pi 4, not the Pi 5.

## Essentials
- TMC2209 module with heatsink
- Separate 12–24 V, 3 A motor supply (24 V gives more torque at top speed)
- 100 µF+ electrolytic capacitor across driver VM/GND
- Pi: official 5 V 3 A USB-C supply
- 3.3 V IR sensor (slot type or TCRT5000) and a contrasting mark on the disc
- 5 mm shaft coupler/hub, rigid motor mount; direct drive or toothed belt only
- Multimeter (coil pairs ≈ 1.5 Ω each; set Vref)

## Safety and setup
- Set motor current to ~1.2–1.4 A with the Vref pot, motor unplugged, power on.
- Never plug or unplug the motor while the driver is powered.
- Keep motor and any encoder ≥ 5 cm below the magnet disc (non-magnetic shaft extension).
- Start slow (`S 30`) and check measured RPM matches before going faster.
- Rough estimate (not measured): eddy-current drag from a 2 mm aluminium plate ≈ 0.1 N·m at 60 rad/s, roughly linear in speed; the 17HS4401 gives ~0.2 N·m at 600 RPM on 24 V.
