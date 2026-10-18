# Magnetic carousel simulation (by phy bot)

C++17 simulation (`magnetic_carousel.cpp`) with plotting script (`plot_magnetic.py`). Outputs (PNGs + CSVs) are in `out_mag/`; start with `out_mag/summary.png`.

**Model:** magnets are point dipoles; the steel ball is a soft-magnetic dipole whose magnetization lags the field by a time tau; the ball rolls on the plate with Coulomb friction and rolling resistance; integrated with RK4.

**Defaults:** 8 N42 magnets (10 x 5 mm) on a 5 cm radius, 5 mm gap, 5 mm-radius steel ball, tau = 1 ms, friction 0.3.

## Findings at the defaults
- Up to about 6.7 rad/s the ball locks onto the magnets and moves with the disc.
- From about 7 to 14 rad/s it is mostly thrown off the magnet ring.
- Above about 14 rad/s it rolls steadily the opposite way, at about 13-21% of the disc speed.

## Trends
- Gap: the unlock speed falls from 21 rad/s at 1 mm to 1.1 rad/s at 15 mm, roughly as (height above magnets)^-3.
- Number of magnets: more magnets give stronger backward motion that starts at a lower disc speed.
- Lag time: backward motion needs tau of at least ~0.1 ms, grows with tau; above ~5 ms the ball is thrown off.
- Friction: on a slippery plate (mu = 0.05) the ball slides and is dragged forward.
- Ramping speed up then down gives different switch points (hysteresis).

## Caveats
Point-dipole approximation is off by ~15-30%; a single tau stands in for both hysteresis and eddy currents; results depend on the ball's magnetization turning with the ball as it rolls. These are simulation predictions, not measurements.
