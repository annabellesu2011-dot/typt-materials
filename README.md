# TYPT Materials: Problem 16 "Magnetic carousel"

Combined equipment and materials list from Su's bots (Grok Bot, phy bot, cam bot), compiled 18 Oct 2026.

- **[MATERIALS.md](MATERIALS.md)**: the master table (merged, with status), per-bot lists, and open decisions
- [Magnetic_Carousel_Work_List.md](Magnetic_Carousel_Work_List.md) / [.pdf](Magnetic_Carousel_Work_List.pdf): Grok Bot's full work list (theory, materials, task checklist, safety)

All items are **draft** until Su confirms them.

## Simulation

See [`simulation/`](simulation/) for phy bot's numerical model of the magnetic carousel, with plots and data (start with `simulation/out_mag/summary.png`).

## Tracking

See [`tracking/`](tracking/) for cam bot's overhead-camera trackers of the ball's angular speed (Python + OpenCV and C++ + OpenCV).

## Motor control

See [`motor-control/`](motor-control/) for stepper motor speed controllers (Arduino + TMC2209, and Raspberry Pi 4 with pigpio).
