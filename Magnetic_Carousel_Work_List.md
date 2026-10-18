# Magnetic Carousel: Working List

**Problem 16 (IYPT 2027 problem list, used for the 19th TYPT)**

> *Several neodymium magnets are attached to a horizontal disc, with adjacent magnets oriented in opposite directions. A fixed non-magnetic plate is placed horizontally above the disc. When the disc rotates about its vertical axis and a steel ball is placed on the plate, the ball may move either in the same direction as, or opposite to, the rotation of the disc. Investigate this phenomenon and determine how the motion depends on relevant parameters.*

Source: TYPT problem page, https://typt.phy.ntnu.edu.tw/home/typt%E8%BE%AF%E8%AB%96%E9%A1%8C%E7%9B%AE.html (wording checked 18 Oct 2026).

---

## 1. The physics to investigate (summary)

The rotating disc carries a pattern of alternating N/S magnets. Seen from the plate, this is a **travelling, alternating magnetic field**: at any fixed point the vertical field component flips sign every time a magnet passes, and the field *direction* rotates around a horizontal axis between neighbouring magnets. A steel ball is soft-magnetic, so the field magnetizes it, and it is pulled toward regions of strong field (the field gradient), mostly down onto the plate and toward the magnets' track. Friction between the ball and the plate lets the ball **roll**, and how it rolls decides which way it travels.

**Hypothesis A: "dragged" motion (co-rotation).** The ball sits in a potential well above a magnet, or between two, and the moving field pattern pulls it along, so it moves *with* the disc. This should win when the disc is slow, the gap is small, the field is strong or friction is low.

**Hypothesis B: "gear" motion (counter-rotation).** As alternating poles pass beneath it, the direction of the field at the ball rotates. The ball's magnetization, and because of magnetic torque from shape or residual magnetization the ball itself, tends to **spin** to follow that rotating direction. A spinning ball touching the plate then **rolls without slipping** the opposite way, like a gear meshing with a rack, so it travels *against* the disc. This should win when the disc is fast and the ball can no longer keep up with the potential wells.

**Hypothesis C: eddy currents.** If the plate is a conductor (aluminium, copper), the moving field induces eddy currents in the plate and in the ball. These cause drag (an asynchronous-motor-like effect) and damping. Comparing acrylic, glass and wood (insulators) with aluminium and copper (conductors) separates this effect from the purely magnetic ones.

Other things that might matter: rolling friction and slipping, how the ball's spin frequency compares with the pole-passing frequency (*f* = *N*·*n*/2 field cycles per second for *N* magnets at *n* rev/s), the ball's moment of inertia, the radial (centrifugal or field-gradient) position of the ball, and hysteresis or remanence in the steel. **All of A, B and C are hypotheses to test, not established results.** A key goal is to find the **transition** between co- and counter-rotation and see what controls it.

---

## 2. Materials list

| # | Item | Suggested spec | Qty | Purpose | Notes |
|---|------|----------------|-----|---------|-------|
| 1 | Neodymium disc magnets | N35 and N52 grades; e.g. Ø10×5 mm, Ø15×5 mm, Ø20×10 mm | 12–24 per size (even numbers) | Create the alternating field | Buy extra; they chip easily. You need an **even** number for proper alternation around the ring |
| 2 | Rotating disc | Acrylic or MDF, Ø200–300 mm, 6–10 mm thick, with holes or pockets for magnets | 2–3 | Holds the magnets | Laser-cut versions with 8 / 12 / 16 / 24 pockets let you change the pole spacing |
| 3 | Motor + speed control | 12 V DC gear motor (~100–600 rpm) + PWM speed controller; or an old record turntable or a drill on a stand | 1 | Spin the disc at a controlled speed | It must spin smoothly and wobble-free; add a flange coupler or hub |
| 4 | Power supply | 12 V DC adapter or bench supply | 1 | Powers the motor | A bench supply makes speed changes repeatable |
| 5 | Tachometer | Optical laser tachometer, or a phone slow-motion video + a marker on the disc | 1 | Measure disc speed | Put reflective tape on the disc for the tachometer |
| 6 | Steel balls | Chrome-steel bearing balls Ø3, 5, 8, 10, 12, 16, 20 mm | 3–5 per size | Test object | Also get stainless steel (weakly magnetic, 304 vs 420), and for comparison a glass or aluminium ball |
| 7 | Plates (top surface) | Acrylic 2, 3, 5 mm; glass 3 mm; plywood 3 mm; aluminium 1–3 mm; copper 1 mm | 1 each | Change the gap, friction and conductivity | Same size as or bigger than the disc; must be **flat and level** |
| 8 | Spacers / height adjusters | Nylon standoffs, washers, or a lab jack | 1 set | Set the plate-to-magnet gap (1–20 mm) | The plate must stay fixed; it must not touch the disc |
| 9 | Frame / stand | Aluminium profile or wooden frame, bubble level | 1 | Hold the plate rigidly above the disc | Adjustable feet help with levelling |
| 10 | Gaussmeter / magnetometer | Hall-probe gaussmeter, or a phone magnetometer app (e.g. phyphox) | 1 | Map the field vs height and position | A phone saturates near strong magnets; use it only at larger distances |
| 11 | High-speed camera | Phone at 240 fps (or more) slow-motion | 1–2 | Record the ball's path and spin | One camera from above, one from the side if possible |
| 12 | Tripod + phone mount | Overhead mount | 1 | Steady top-down video | Keep the camera at a fixed height for calibration |
| 13 | Lighting | LED panel | 1 | Bright, flicker-free light for slow-mo | Avoid mains-flicker lights at high fps |
| 14 | Tracking software | **Tracker** (free, physlets.org/tracker) and/or Python (OpenCV) | – | Extract position, angular velocity and spin | Paint a dot or line on the ball so spin is visible |
| 15 | Measuring tools | Ruler, vernier/digital calipers, protractor | 1 each | Geometry: radii, gaps, ball sizes | |
| 16 | Digital scale | 0.01 g resolution | 1 | Ball masses | |
| 17 | Adhesives | Epoxy or hot glue; double-sided tape | – | Fix the magnets in the disc | Magnets repel each other in alternating rings, so glue them firmly |
| 18 | Pole marker | Compass or pole-identifier + permanent marker | 1 | Mark N/S on every magnet before gluing | Essential for correct alternation |
| 19 | Markers for tracking | White/coloured paint pen, small stickers | – | Mark the ball (spin) and disc (speed) | |
| 20 | Inclined-plane kit (optional) | Plank + protractor | 1 | Measure rolling friction of ball on each plate | Supports the modelling |
| 21 | Laptop | Python 3 (numpy, scipy, matplotlib), Tracker, slide software | 1 | Analysis, simulation, presentation | |
| 22 | Safety gear | Safety goggles, thin gloves, non-magnetic pliers/wedge | 1 set | Protection | See Section 4 |
| 23 | Storage | Plastic box with dividers, magnet keepers | 1 | Store magnets safely apart | Keep away from phones, cards and electronics |

---

## 3. Jobs / tasks checklist

### Phase 1: Research & theory (week 1–2)
- [ ] Read the problem carefully and list every "relevant parameter"
- [ ] Review basics: magnetization of soft iron, force on a magnetic dipole in a field gradient, magnetic torque, rolling without slipping, rolling friction, eddy currents
- [ ] Look up similar known demos (rotating-magnet "magnetic gear" or "ball races", Arago's disc / eddy current drag, magnetic stirrers) and note real sources only
- [ ] Write down Hypotheses A, B, C (Section 1) and the prediction each one makes
- [ ] Make simple order-of-magnitude estimates (field at gap *h*, magnetic force vs ball weight, pole-passing frequency)

### Phase 2: Building the apparatus (week 2–3)
- [ ] Design the disc(s): radius of the magnet ring, number of pockets (8/12/16/24)
- [ ] Mark the N/S pole of every magnet
- [ ] Glue the magnets in **alternating** orientation; let the epoxy cure fully
- [ ] Build a **control disc** with all magnets in the **same** orientation
- [ ] Mount the motor and check that the disc runs true (no wobble or contact)
- [ ] Build the frame with an adjustable, levelled plate holder
- [ ] Add a speed marker / reflective tape to the disc
- [ ] Set up the overhead camera, lighting and a scale reference (ruler in frame)

### Phase 3: Calibration (week 3)
- [ ] Calibrate disc speed vs controller setting (rpm table) with a tachometer or video
- [ ] Map *B* (vertical and horizontal parts) vs height above the magnets (0–20 mm)
- [ ] Map *B* along the ring path (one full pole period) at the plate height
- [ ] Measure rolling-friction / slip properties of each ball–plate pair (inclined plane)
- [ ] Measure every ball's diameter and mass
- [ ] Check repeatability: run the same setting 5 times

### Phase 4: Experiments (week 4–6)
For each run, record: disc rpm, ball's angular velocity around the axis, ball's spin direction and rate, direction (co or counter), whether it is steady, and the ball's radius of motion.

| Parameter | Suggested range | Hold constant | What to look for |
|---|---|---|---|
| Disc angular speed | 0 → max, small steps (e.g. every 20 rpm) | everything else | Co/counter transition speed; ball speed vs disc speed |
| Number of magnets / pole spacing | 8, 12, 16, 24 | rpm, gap, ball | Does the pole-passing frequency control the direction? |
| Magnet strength / size | N35 vs N52; different sizes | spacing, gap | Force vs drag |
| Plate-to-magnet gap | 1–20 mm | rpm, ball | Weaker field means less dragging |
| Plate material (friction) | acrylic, glass, wood | gap, rpm | Role of friction and slipping |
| Plate conductivity | acrylic vs aluminium vs copper (same thickness) | gap, rpm | Eddy-current effect |
| Ball diameter / mass | Ø3–20 mm | rpm, gap | Inertia vs magnetic torque |
| Ball material | chrome steel, stainless 420/304, non-magnetic | – | Is the effect magnetic? Does remanence matter? |
| Ball starting position | inner / on / outer side of the magnet ring | rpm, gap | Stable orbits, radial drift |
| Magnet arrangement (control) | alternating vs all the same | everything | Is alternation necessary for counter-rotation? |

- [ ] Do a quick qualitative survey first (find where co and counter rotation happen)
- [ ] Then do careful sweeps, one parameter at a time
- [ ] Repeat every point at least 3–5 times for error bars
- [ ] Keep a lab notebook: date, setup photo, settings, video file names

### Phase 5: Measurement method
- [ ] Film from above at 240 fps; keep the camera fixed and calibrated
- [ ] Track the ball centre in Tracker, then convert to polar coordinates (r, θ) about the disc axis
- [ ] Get the ball's orbital angular velocity Ω_ball from θ(t)
- [ ] Track the painted mark on the ball to get its spin rate ω_spin and direction
- [ ] Get the disc's angular velocity Ω_disc from the disc marker
- [ ] Check the rolling-without-slipping condition: compare the ball's surface speed with its travel speed

### Phase 6: Data analysis
- [ ] Plot Ω_ball vs Ω_disc (sign shows co- or counter-rotation)
- [ ] Plot the ratio Ω_ball / Ω_disc vs each parameter
- [ ] Plot ω_spin vs the pole-passing frequency *f* = *N*·Ω_disc/(4π) (in rev/s: *N*·*n*/2)
- [ ] Find the **transition** speed or condition and plot it vs gap, ball size and *N*
- [ ] Look for dimensionless groups (e.g. magnetic force/weight, pole spacing/ball diameter)
- [ ] Estimate uncertainties and show error bars on every plot

### Phase 7: Modelling / simulation
- [ ] Build a simple field model (magnets as dipoles or cylinders); compute *B* at the plate and compare with the field map
- [ ] Model the force on the ball (induced dipole, ∝ ∇B²) and the torque on it
- [ ] Write equations of motion for a rolling ball (translation + rotation + friction)
- [ ] Simulate in Python (numpy/scipy `solve_ivp`); predict the co/counter regimes
- [ ] Compare the model with the experiment and discuss the differences honestly
- [ ] Optional: COMSOL / FEMM for eddy currents in conductive plates

### Phase 8: Presentation & debate prep
- [ ] Make a 12-minute report: phenomenon video → theory → setup → results → model vs data → conclusion
- [ ] Include clear slow-motion clips of both co- and counter-rotation
- [ ] Prepare backup slides (field maps, error analysis, extra parameters)
- [ ] List likely opponent/reviewer questions and answers, for example:
  - [ ] "How do you know the ball rolls without slipping?"
  - [ ] "What is the role of eddy currents? Did you test a conductive plate?"
  - [ ] "Why does the transition happen at that speed? Is it predicted by your model?"
  - [ ] "What about the ball's remanent magnetization / hysteresis?"
  - [ ] "Is your result valid for other magnet numbers or sizes?"
  - [ ] "What are your main sources of error?"
- [ ] Rehearse with teammates; time it; practise opponent and reviewer roles too

---

## 4. Safety notes

- **Pinching:** Strong neodymium magnets snap together with great force and can pinch skin badly. Handle one at a time, slide them apart (don't pull), and keep them separated with spacers.
- **Fragile magnets:** NdFeB magnets are brittle and can **shatter** into sharp pieces if they collide. Always wear **safety goggles**.
- **Rotating parts:** Make sure the magnets are glued firmly before spinning; a loose magnet can fly off. Start at low speed, use a guard or shield, keep hair, sleeves and fingers away, and never reach under the plate while it is spinning.
- **Steel balls:** Small balls can shoot off the plate; keep the area clear and wear goggles.
- **Electronics & medical:** Keep magnets away from pacemakers and medical implants, phones, credit cards and hard drives. Use a phone magnetometer only at a safe distance.
- **Electrical:** Use a proper low-voltage supply; insulate the motor wiring.
- **Tools & glue:** Use epoxy in a ventilated area; wear gloves.
- **Storage:** Store magnets in a labelled box with keepers, away from children.

---

*Prepared as a planning aid. Mechanisms in Section 1 are hypotheses to be tested experimentally.*
