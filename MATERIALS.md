# TYPT Materials Summary: Problem 16 "Magnetic carousel"

**Compiled:** 18 Oct 2026 (Asia/Taipei)
**Compiled by:** Grok Bot, from the lists kept by Su's bots (Grok Bot, phy bot, cam bot)

This file combines every bot's equipment notes into one list. **Nothing here has been confirmed by Su yet.** Specs and quantities are suggestions or simulation defaults unless a row says otherwise. Nothing is invented: if a source gave no value, the cell says *not specified* or *TBD*.

**Status key**
- **draft**: proposed by a bot, not confirmed by Su
- **TBD**: still undecided or unspecified
- **confirmed**: approved by Su (no rows have this status yet)

> Problem statement: *Several neodymium magnets are attached to a horizontal disc, with adjacent magnets oriented in opposite directions. A fixed non-magnetic plate is placed horizontally above the disc. When the disc rotates about its vertical axis and a steel ball is placed on the plate, the ball may move either in the same direction as, or opposite to, the rotation of the disc. Investigate this phenomenon and determine how the motion depends on relevant parameters.*
> Source: [TYPT problem page](https://typt.phy.ntnu.edu.tw/home/typt%E8%BE%AF%E8%AB%96%E9%A1%8C%E7%9B%AE.html). The page lists it as an IYPT 2027 problem for the 19th TYPT. See *Conflicts & open decisions* for the year labelling.

---

## 1. Master materials table (merged, deduplicated)

Source abbreviations: **GB** = Grok Bot work list, **PHY** = phy bot, **CAM** = cam bot.

| # | Item | Spec | Qty | Purpose | Source bot(s) | Status |
|---|------|------|-----|---------|---------------|--------|
| 1 | Neodymium magnets | PHY: N42 cylinders Ø10 × 5 mm (simulation default). GB: N35 and N52 grades, e.g. Ø10×5, Ø15×5, Ø20×10 mm. Mounted with alternating poles up | PHY: 8 (also testing 4–16). GB: 12–24 per size, even numbers, plus spares | Create the alternating field | GB, PHY | draft (grade and size TBD) |
| 2 | Rotating disc | PHY: magnets on a 5 cm radius. GB: acrylic or MDF, Ø200–300 mm, 6–10 mm thick, pockets for 8/12/16/24 magnets | PHY: 1. GB: 2–3, plus 1 control disc with all magnets the same way up | Carries the magnets | GB, PHY | draft |
| 3 | Motor + speed controller | PHY: DC motor, steady ~0–60 rad/s, direct drive or gear/toothed belt (avoid rubber belts, they slip). GB: 12 V DC gear motor (~100–600 rpm) + PWM controller, or a turntable or drill on a stand; smooth, wobble-free, with a flange coupler or hub | 1 | Spin the disc at a steady, controlled speed | GB, PHY | draft |
| 4 | Power supply | GB: 12 V DC adapter or bench supply | 1 | Power the motor | GB | draft |
| 5 | Fixed non-magnetic plate(s) | PHY: aluminium (conducting) and acrylic (non-conducting). GB: acrylic 2/3/5 mm, glass 3 mm, plywood 3 mm, aluminium 1–3 mm, copper 1 mm; flat and level, at least as large as the disc | PHY: at least 1 aluminium + 1 acrylic. GB: 1 of each | Surface the ball rolls on; varies gap, friction and conductivity | GB, PHY | draft |
| 6 | Spacers / height adjusters | PHY: spacers for a ~1–15 mm gap. GB: nylon standoffs, washers or a lab jack, 1–20 mm gap | 1 set | Set the plate-to-magnet gap | GB, PHY | draft (gap range TBD) |
| 7 | Frame / stand | GB: aluminium profile or wooden frame, bubble level, adjustable feet | 1 | Hold the plate rigidly above the disc | GB | draft |
| 8 | Steel balls | PHY: radius ~2–10 mm (5 mm default), hard vs soft steel. GB: chrome-steel bearing balls Ø3, 5, 8, 10, 12, 16, 20 mm; stainless 304/420; glass or aluminium ball as a non-magnetic control | GB: 3–5 per size. PHY: not specified | Test objects | GB, PHY | draft (size set TBD) |
| 9 | Target sphere (for camera tracking) | CAM: silver/metallic, size not specified | 1 (assumed) | Object tracked by the camera; appears to be the same as the steel ball in row 8 | CAM | TBD |
| 10 | Disc-speed measurement | PHY: phone tachometer app, slow-motion video, or a magnet + Hall sensor counting turns. GB: optical laser tachometer with reflective tape, or phone slow-mo + disc marker | 1 | Measure the actual disc speed | GB, PHY | draft (method TBD) |
| 11 | Overhead camera | CAM: webcam E-books W15, model E-PCC077, USB 2.0, adjustable focus, LED fill lights kept **off** (glare). GB: phone at ≥240 fps slow-mo, 1 above + optionally 1 at the side. PHY: camera above the setup (no model given) | CAM: 1 webcam. GB: 1–2 phones | Track the ball's path, angular speed and spin | GB, PHY, CAM | draft (camera choice TBD) |
| 12 | Camera mount | GB: tripod + phone mount, overhead, fixed height. CAM: webcam mounted directly above the plate looking down | 1 | Steady, calibrated top-down video | GB, CAM | draft |
| 13 | Lighting | CAM: soft, even, diffuse room light, not a single bright lamp overhead. GB: flicker-free LED panel (avoid mains flicker at high fps) | 1 | Even lighting for tracking / slow-mo | GB, CAM | draft |
| 14 | Tracking background plate | CAM: 3D-printed matte black PLA. Print top side down on a textured bed, 0.12–0.16 mm layers, concentric/monotonic top pattern, no raised features, thick or 100% infill so it stays flat; optional sanding + matte black spray paint. Diameter and thickness not decided | 1 (assumed) | Matte, high-contrast background for tracking. Could double as the fixed plate (row 5); see open decisions | CAM | TBD |
| 15 | Hall probe / gaussmeter | PHY: Hall probe / gaussmeter. GB: Hall-probe gaussmeter, or a phone magnetometer (e.g. phyphox) only at larger distances (saturates near strong magnets) | 1 | Map the field and compare it with the model | GB, PHY | draft |
| 16 | Tracking / analysis software | CAM: Python and C++ OpenCV trackers (on the box). GB: Tracker (physlets.org/tracker) and/or Python OpenCV | – | Extract position, angular velocity and spin | GB, CAM | draft (tools exist; choice TBD) |
| 17 | Simulation software | PHY: C++ simulation + Python plotting (on the box) | – | Model the field and the ball's motion | PHY | draft |
| 18 | Computer / laptop | CAM: computer running the Python or C++ OpenCV tracker. GB: laptop with Python 3 (numpy, scipy, matplotlib), Tracker, slide software | 1 | Tracking, analysis, simulation, presentation | GB, CAM | draft |
| 19 | Measuring tools | GB: ruler, vernier/digital calipers, protractor | 1 each | Radii, gaps, ball sizes | GB | draft |
| 20 | Digital scale | GB: 0.01 g resolution | 1 | Ball masses | GB | draft |
| 21 | Adhesives | GB: epoxy or hot glue; double-sided tape | – | Fix the magnets in the disc | GB | draft |
| 22 | Pole marker | GB: compass or pole identifier + permanent marker | 1 | Mark N/S on every magnet before gluing | GB | draft |
| 23 | Tracking markers | GB: white/coloured paint pen, small stickers | – | Mark the ball (spin) and the disc (speed) | GB | draft |
| 24 | Inclined-plane kit (optional) | GB: plank + protractor | 1 | Rolling friction of each ball–plate pair | GB | draft |
| 25 | Safety gear | GB: safety goggles, thin gloves, non-magnetic pliers/wedge; guard or shield around the spinning disc | 1 set | Protection | GB | draft |
| 26 | Magnet storage | GB: plastic box with dividers, magnet keepers | 1 | Store magnets safely apart | GB | draft |

### Software and files on the shared box

| Path | What it is | Source |
|------|------------|--------|
| `/workspace/carousel/magnetic_carousel.cpp` | Magnetic carousel simulation (C++) | phy bot |
| `/workspace/carousel/plot_magnetic.py` | Plotting script for the simulation | phy bot |
| `/workspace/carousel/out_mag/` | Simulation output plots and CSVs | phy bot |
| `/workspace/sphere-tracker/` (zip: `/workspace/sphere-tracker.zip`) | Python OpenCV sphere tracker | cam bot |
| `/workspace/sphere-tracker-cpp/` (zip: `/workspace/sphere-tracker-cpp.zip`) | C++ OpenCV sphere tracker | cam bot |
| `Magnetic_Carousel_Work_List.md` / `.pdf` (this folder) | Full work list: theory, materials, 8-phase task checklist, safety | Grok Bot |

These files are **not** copied into this folder. They live elsewhere on the box and could become their own repos, or be added later.

---

## 2. Per-bot sections

### 2.1 Grok Bot: Magnetic Carousel Work List
Full list in [`Magnetic_Carousel_Work_List.md`](Magnetic_Carousel_Work_List.md) (PDF: [`Magnetic_Carousel_Work_List.pdf`](Magnetic_Carousel_Work_List.pdf)). All specs are *suggested*, so they count as draft.

| # | Item | Suggested spec | Qty |
|---|------|----------------|-----|
| 1 | Neodymium disc magnets | N35 and N52; e.g. Ø10×5, Ø15×5, Ø20×10 mm | 12–24 per size (even) |
| 2 | Rotating disc | Acrylic or MDF, Ø200–300 mm, 6–10 mm thick, pockets | 2–3 |
| 3 | Motor + speed control | 12 V DC gear motor (~100–600 rpm) + PWM; or turntable or drill on a stand | 1 |
| 4 | Power supply | 12 V DC adapter or bench supply | 1 |
| 5 | Tachometer | Optical laser tachometer, or phone slow-mo + disc marker | 1 |
| 6 | Steel balls | Chrome steel Ø3–20 mm; stainless 304/420; glass or Al control | 3–5 per size |
| 7 | Plates | Acrylic 2/3/5 mm, glass 3 mm, plywood 3 mm, Al 1–3 mm, Cu 1 mm | 1 each |
| 8 | Spacers / height adjusters | Nylon standoffs, washers, lab jack (1–20 mm gap) | 1 set |
| 9 | Frame / stand | Al profile or wood, bubble level | 1 |
| 10 | Gaussmeter / magnetometer | Hall-probe gaussmeter or phone app (phyphox) | 1 |
| 11 | High-speed camera | Phone at ≥240 fps | 1–2 |
| 12 | Tripod + phone mount | Overhead | 1 |
| 13 | Lighting | LED panel, flicker-free | 1 |
| 14 | Tracking software | Tracker and/or Python (OpenCV) | – |
| 15 | Measuring tools | Ruler, calipers, protractor | 1 each |
| 16 | Digital scale | 0.01 g | 1 |
| 17 | Adhesives | Epoxy or hot glue; double-sided tape | – |
| 18 | Pole marker | Compass or pole identifier + marker | 1 |
| 19 | Tracking markers | Paint pen, stickers | – |
| 20 | Inclined-plane kit (optional) | Plank + protractor | 1 |
| 21 | Laptop | Python 3 (numpy, scipy, matplotlib), Tracker, slides | 1 |
| 22 | Safety gear | Goggles, thin gloves, non-magnetic pliers/wedge | 1 set |
| 23 | Storage | Box with dividers, magnet keepers | 1 |

### 2.2 phy bot (theory and simulation)
Problem: 16, Magnetic carousel (phy bot calls it TYPT/IYPT **2026** Problem 16). phy bot says this is a **draft**: Su hasn't confirmed any specs or quantities. No materials file was saved; this list comes from phy bot's discussion with Su plus the simulation defaults.

| Item | Spec | Qty | Purpose |
|------|------|-----|---------|
| Neodymium magnets | N42 cylinders, Ø10 × 5 mm (sim default), alternating poles up | 8 (also testing 4–16) | Alternating field |
| Horizontal disc | Magnets on a 5 cm radius | 1 | Carries the magnets |
| Motor with speed controller | DC, steady ~0–60 rad/s; direct drive or gear/toothed belt (no rubber belts) | 1 | Constant disc speed |
| Fixed non-magnetic plate | Aluminium (conducting) and acrylic (non-conducting); spacers for ~1–15 mm gap | not specified | Ball rolls on it |
| Steel balls | Radius ~2–10 mm (5 mm default); hard vs soft steel (magnetization lag) | not specified | Test objects |
| Disc-speed measurement | Phone tachometer app, slow-mo video, or magnet + Hall sensor | not specified | Actual disc speed |
| Camera above setup | not specified | not specified | Ball angular speed |
| Hall probe / gaussmeter | not specified | not specified | Field vs model |

Related files: `/workspace/carousel/magnetic_carousel.cpp`, `/workspace/carousel/plot_magnetic.py`, `/workspace/carousel/out_mag/`.

### 2.3 cam bot (camera tracking)
Su never told cam bot which TYPT/IYPT problem this is for. The project measures a silver sphere's angular speed as it orbits the centre of a plate, using an overhead camera. It **appears related** to the Magnetic carousel ball tracking, but that isn't confirmed. No quantities were set, so each item is assumed to be 1. No materials file was saved.

| Item | Spec | Qty | Purpose |
|------|------|-----|---------|
| Webcam | E-books W15, model E-PCC077; USB 2.0; LED fill lights kept off (glare); adjustable focus; mounted directly above the plate looking down | 1 | Films the sphere |
| Target sphere | Silver/metallic; size not specified | 1 | Tracked object |
| Plate/disk | 3D-printed matte black PLA; print top side down on a textured bed; 0.12–0.16 mm layers; concentric/monotonic top pattern; no raised features; thick or 100% infill; optional sanding + matte black spray paint; diameter and thickness TBD | 1 | Matte, high-contrast background |
| Lighting | Soft, even, diffuse room light, not a single bright overhead lamp | – | Even illumination |
| Computer | Runs the Python or C++ OpenCV tracker | 1 | Tracking |

Software: `/workspace/sphere-tracker` (Python), `/workspace/sphere-tracker-cpp` (C++), plus zip copies in `/workspace/`.

### 2.4 motor bot
No materials list yet. Su only just set up motor bot, and no problem or equipment has been picked, so it's left out of this summary for now.

---

## 3. Conflicts & open decisions

1. **Magnet grade and size:** phy bot uses N42, Ø10×5 mm, ×8 (sim default, testing 4–16). Grok Bot's list suggests N35 and N52 in several sizes, 12–24 per size. Decide the grade(s), size(s) and count, and keep the count even.
2. **Disc size:** phy bot uses a 5 cm magnet-ring radius. Grok Bot suggests a Ø200–300 mm disc with 8/12/16/24 pockets. Choose the disc diameter, ring radius and number of discs.
3. **Motor speed range:** phy bot gives ~0–60 rad/s (≈ 0–570 rpm). Grok Bot suggests a ~100–600 rpm gear motor. The ranges mostly overlap, but check the low-speed end and the drive type (phy bot: avoid rubber belts).
4. **Camera:** cam bot uses the E-books W15 webcam (USB 2.0, frame rate not specified). Grok Bot suggests a phone at ≥240 fps. phy bot just says "camera above". Decide whether the webcam's frame rate is enough for spin tracking, or whether to use the phone for slow-mo (or both).
5. **Plate material:** phy bot plans aluminium + acrylic. Grok Bot adds glass, plywood and copper. cam bot's plate is 3D-printed matte black PLA. Is the PLA plate the fixed plate the ball rolls on, a background under a transparent plate, or a separate tracking-only setup? PLA thickness also affects the gap.
6. **Ball size and type:** phy bot uses radius 2–10 mm (= Ø4–20 mm, 5 mm radius default). Grok Bot lists Ø3–20 mm and stainless/non-magnetic controls. cam bot's "silver sphere" has no size given. Note radius vs diameter when buying.
7. **Gap range:** phy bot says ~1–15 mm; Grok Bot says 1–20 mm.
8. **Disc-speed method:** phone tachometer vs laser tachometer vs Hall sensor vs slow-mo video.
9. **Tracking software:** cam bot's OpenCV trackers (Python/C++) vs Tracker (physlets).
10. **Lighting:** cam bot wants diffuse room light; Grok Bot suggests an LED panel. Pick one that is flicker-free and glare-free on a shiny ball.
11. **Problem year labelling:** phy bot calls it "TYPT/IYPT 2026 Problem 16". The TYPT page (and Grok Bot's list) labels it an IYPT **2027** problem used for the 19th TYPT. Check and use one label everywhere.
12. **cam bot's problem:** Su hasn't confirmed that cam bot's sphere-tracking project is for Problem 16.
13. **Quantities:** most quantities are suggestions or assumed 1. Su needs to confirm them before buying.
