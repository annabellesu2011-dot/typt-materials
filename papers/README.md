# Literature for IYPT/TYPT Problem 16: Magnetic carousel

This annotated bibliography collects **21 verified sources** for *Magnetic carousel*, official **Problem 16 of the 40th IYPT (2027)** and of the **19th TYPT**. The problem was approved by the IYPT IOC on 14 July 2026 and is being prepared during 2026. It is not an IYPT 2026 problem.

**Official statement.** Several neodymium magnets are attached to a horizontal disc, with adjacent magnets oriented in opposite directions. A fixed non-magnetic plate is placed horizontally above the disc. When the disc rotates about its vertical axis and a steel ball is placed on the plate, the ball may move either in the same direction as, or opposite to, the rotation of the disc. Investigate this phenomenon and determine how the motion depends on relevant parameters.

**Physics suggested by the closest papers.** At low disc speed and small gap, the steel ball is trapped in a field maximum and is dragged along with the travelling magnet pattern (synchronous co-rotation, sometimes with sliding rather than rolling). Above a critical wave speed the lock is lost. The alternating magnets produce a travelling field whose direction also rotates in a vertical plane; magnetization lag (hysteresis and/or eddy currents) plus rolling friction can then torque the ball so that it rolls *against* the pattern. The same qualitative transition appears for both soft-steel and permanently magnetized beads, and the switch can show hysteresis when the disc speed is ramped up and then down. Other relevant ingredients are the exponential decay of a periodic magnet field with height, eddy-current drag if the plate is conducting, and friction that decides whether the ball rolls or slides.

Every journal item below was checked against a publisher page, an official PDF, and/or the Crossref record. No citation was invented. BibTeX for all 21 entries is in [`papers.bib`](papers.bib). Open-access versions are linked where they exist.

---

## 1. Official problem statements and IYPT/TYPT resources

1. **International Young Physicists’ Tournament**, *Problems for the 40th IYPT 2027* (approved 14 July 2026). Problem 16 is “Magnetic carousel.”
   - https://iypt.org/iypt-2027-problems/

2. **Taiwan Young Physicists’ Tournament**, *第19屆臺灣青年學生物理辯論競賽題目* (19th TYPT problem list). The 19th TYPT uses the IYPT 2027 set and includes Problem 16 in full.
   - https://typt.phy.ntnu.edu.tw/home/typt%E8%BE%AF%E8%AB%96%E9%A1%8C%E7%9B%AE.html

3. **Filip Křížek**, *Problem No. 16 “Magnetický kolotoč”*, mentor notes for the 40th IYPT, Nuclear Physics Institute of the Czech Academy of Sciences (ÚJF AV ČR). Outlines the slow synchronous and fast reverse-orbit regimes, writes the force as \(\mathbf{F}=\nabla(\mathbf{m}\cdot\mathbf{B})\) with hysteresis-dependent magnetization, and flags friction, eddy currents, steel hardness, ball size, field strength, and magnet spacing as parameters to test. Includes a demonstration video.
   - https://www.tmfcr.cz/40/pdf/16-krizek.pdf
   - Video: https://youtu.be/Yn5Iw2qkWyY

4. **German Young Physicists’ Tournament (GYPT)**, “16 Magnetic carousel.” Official problem text plus starter questions on the forces on the ball, initial position, motor speed, magnet strength and number, and magnet arrangement and spacing.
   - https://www.gypt.org/aufgaben/16-magnetic-carousel.html

---

## 2. Co-rotation versus counter-rotation of magnetic and ferromagnetic spheres

These three papers are the closest published analogues of the IYPT/TYPT setup.

5. **Christophe Gissinger**, “Wave-induced motion of magnetic spheres,” *EPL (Europhysics Letters)* **112**, 50003 (2015).
   - DOI: https://doi.org/10.1209/0295-5075/112.50003
   - arXiv: https://arxiv.org/abs/1601.01111
   - Author PDF: https://www.phys.ens.fr/~gissinger/pdf/Gissinger_epl2015.pdf
   - **Why it matters:** This is essentially the carousel experiment: a steel or NdFeB bead on a fixed plexiglass plate, driven by 16 alternating-polarity neodymium magnets on a rotating disc. At low wave speed the bead is trapped and translates in synchronism with the wave (often sliding, not rolling); above a threshold it bifurcates, subcritically, into backward rolling that can even be faster than the wave, with hysteresis and possible chaos. The model combines magnetic torque, solid friction, and air resistance.

6. **Yeung Yeung Chau, Ruo-Yang Zhang, and Weijia Wen**, “Reverse rotation of soft ferromagnetic ball in rotating magnetic field,” *Journal of Magnetism and Magnetic Materials* **476**, 376–381 (2019).
   - DOI: https://doi.org/10.1016/j.jmmm.2018.12.073
   - **Why it matters:** Maps the motion of a soft-iron ball above two rotating magnets, for both same and opposite polarity. At low speed and small height the ball is trapped and co-rotates; at high speed it enters a stable reverse-rotation phase. Phase diagrams versus rotation rate and height, plus almost-stationary transition states, are directly usable as a design and comparison template.

7. **Jean Farago, Thierry Charitat, Alexandre Bigot, Romain Schotter, and Igor Kulić**, “Counterrotation of magnetic beads in spinning fields,” *Physical Review E* **102**, 042201 (2020).
   - DOI: https://doi.org/10.1103/PhysRevE.102.042201
   - arXiv: https://arxiv.org/abs/2003.10782
   - **Why it matters:** Revisits Chau *et al.* with a laboratory stirrer, experiments, numerics, and theory for a ferromagnetic bead whose translation and rotation are coupled by contact with the plate. It explains the co- to counter-rotation flip above a critical frequency, the radial stabilization of the counter-rotating orbit, and why a purely paramagnetic bead (no remanence) cannot counter-rotate in their model.

---

## 3. Magnetic surface walkers, rolling microrobots, and step-out

8. **Pietro Tierno, Ramin Golestanian, Ignacio Pagonabarraga, and Francesc Sagués**, “Controlled swimming in confined fluids of magnetically actuated colloidal rotors,” *Physical Review Letters* **101**, 218304 (2008).
   - DOI: https://doi.org/10.1103/PhysRevLett.101.218304
   - **Why it matters:** A classic “surface walker”: a rotating magnetic field turns a magnetized particle near a wall, and the wall converts rotation into net translation. That torque-plus-rolling mechanism is the microscopic counterpart of the carousel ball rolling on the plate.

9. **Charles E. Sing, Lothar Schmid, Matthias F. Schneider, Thomas Franke, and Alfredo Alexander-Katz**, “Controlled surface-induced flows from the motion of self-assembled colloidal walkers,” *Proceedings of the National Academy of Sciences* **107**, 535–540 (2010).
   - DOI: https://doi.org/10.1073/pnas.0906489107
   - **Why it matters:** Self-assembled colloidal chains “walk” along a surface in a rotating magnetic field. Gissinger cites this work as the microfluidic analogue of wave-driven rolling of magnetic spheres.

10. **Arthur W. Mahoney, Nathan D. Nelson, Kathrin E. Peyer, Bradley J. Nelson, and Jake J. Abbott**, “Behavior of rotating magnetic microrobots above the step-out frequency with application to control of multi-microrobot systems,” *Applied Physics Letters* **104**, 144101 (2014).
    - DOI: https://doi.org/10.1063/1.4870768
    - **Why it matters:** Defines the *step-out frequency*: the rotation rate at which the available magnetic torque can no longer keep a magnetic or soft-ferromagnetic robot synchronous with the driving field, after which its speed drops sharply. This is the natural language for the carousel’s loss of lock and the transition out of co-rotation.

---

## 4. Eddy-current drag, magnetic braking, and conducting plates

These papers matter if the fixed plate is metal (aluminum, copper, etc.), and more generally for eddy-current forces between moving magnets and conductors. An insulating plate (acrylic, glass) largely removes this contribution.

11. **H. D. Wiederick, N. Gauthier, D. A. Campbell, and P. Rochon**, “Magnetic braking: Simple theory and experiment,” *American Journal of Physics* **55**, 500–503 (1987).
    - DOI: https://doi.org/10.1119/1.15103
    - **Why it matters:** A clear undergraduate theory and experiment for the drag on a conductor moving through a localized magnetic field, i.e. eddy-current braking. If the carousel plate is conducting, the spinning magnet array will feel (and produce) a similar drag.

12. **W. M. Saslow**, “Maxwell’s theory of eddy currents in thin conducting sheets, and applications to electromagnetic shielding and MAGLEV,” *American Journal of Physics* **60**, 693–711 (1992).
    - DOI: https://doi.org/10.1119/1.17101
    - **Why it matters:** The standard modern account of Maxwell’s receding-image theory for a magnet moving over a thin conducting sheet, including Arago’s disk (a magnet above a rotating conductor) and the lift-to-drag ratio. Directly relevant to eddy-current coupling between the rotating magnet disc and a metal plate.

13. **Manuel I. González**, “Experiments with eddy currents: the eddy current brake,” *European Journal of Physics* **25**, 463–468 (2004).
    - DOI: https://doi.org/10.1088/0143-0807/25/4/001
    - **Why it matters:** A compact student-lab treatment of an eddy-current brake, useful if you need to measure or estimate the braking torque on the disc when a conducting plate is used.

14. **Fernando G. Tomasel and Mario C. Marconi**, “Rolling magnets down a conductive hill: Revisiting a classic demonstration of the effects of eddy currents,” *American Journal of Physics* **80**, 800–803 (2012).
    - DOI: https://doi.org/10.1119/1.4730939
    - **Why it matters:** Quantitative video tracking of rare-earth magnets rolling on a conducting incline — a laboratory cousin of IYPT 2006 “Rolling magnets,” and a practical method for extracting eddy-current drag from filmed trajectories.

15. **Mark A. Nurge, Robert C. Youngquist, and Stanley O. Starr**, “Drag and lift forces between a rotating conductive sphere and a cylindrical magnet,” *American Journal of Physics* **86**, 443–452 (2018).
    - DOI: https://doi.org/10.1119/1.5024220
    - **Why it matters:** Calculates and measures the eddy-current drag and repulsive lift between a rotating conducting sphere and a nearby cylindrical magnet. That is the local force law for a steel (conducting) ball moving relative to the carousel magnets, even when the plate itself is insulating.

---

## 5. Eddy-current and hysteresis torque on steel spheres in rotating fields

16. **B. R. Teare**, “Theory of Hysteresis-Motor Torque,” *Transactions of the American Institute of Electrical Engineers* **59**, 907–912 (1940).
    - DOI: https://doi.org/10.1109/T-AIEE.1940.5058066
    - **Why it matters:** The classic quantitative theory of hysteresis-motor torque: magnetization lags the rotating field by a hysteresis angle, producing a torque set by the \(B\)–\(H\) loop and the geometry. This is the lag-torque mechanism often invoked for a steel ball in a turning carousel field.

17. **T. Reichert, T. Nussbaumer, and J. W. Kolar**, “Complete analytical solution of electromagnetic field problem of high-speed spinning ball,” *Journal of Applied Physics* **112**, 104901 (2012).
    - DOI: https://doi.org/10.1063/1.4765676
    - **Why it matters:** Exact fields, eddy-current density, motor torque, and losses for a conducting sphere in a rotating magnetic field, checked against 3D FEM. It is a usable formula set for the eddy-current part of the torque on the steel ball as a function of slip frequency.

18. **Marcel Schuck, Daniel Steinert, Thomas Nussbaumer, and Johann W. Kolar**, “Ultrafast rotation of magnetically levitated macroscopic steel spheres,” *Science Advances* **4**, e1701519 (2018).
    - DOI: https://doi.org/10.1126/sciadv.1701519
    - Open access: https://www.science.org/doi/10.1126/sciadv.1701519
    - PMC: https://pmc.ncbi.nlm.nih.gov/articles/PMC5756664/
    - **Why it matters:** Experiments and a torque model for millimeter-scale steel spheres driven by a rotating field. The torque is shown to arise from eddy currents induced by the slip between the field and the sphere’s rotation — the same lag-torque idea, measured on real steel balls.

---

## 6. Magnetic gears and Halbach / periodic magnet arrays

19. **K. Atallah and D. Howe**, “A novel high-performance magnetic gear,” *IEEE Transactions on Magnetics* **37**, 2844–2846 (2001).
    - DOI: https://doi.org/10.1109/20.951324
    - **Why it matters:** The seminal coaxial magnetic gear: torque is transmitted synchronously by a periodic permanent-magnet field, with a definite pull-out torque beyond which synchronism is lost. That is the engineering analogue of the ball locking to, and then slipping out of, the moving magnet pattern.

20. **K. Halbach**, “Design of permanent multipole magnets with oriented rare earth cobalt material,” *Nuclear Instruments and Methods* **169**, 1–10 (1980).
    - DOI: https://doi.org/10.1016/0029-554X(80)90094-4
    - **Why it matters:** The founding Halbach-array paper. A periodic permanent-magnet arrangement produces a travelling or rotating field whose strength falls off approximately as \(e^{-kz}\) with \(k\) set by the spatial period. That decay law is why gap and magnet spacing are expected to control the locking speed so strongly.

---

## 7. Earlier IYPT problem on rolling magnets

21. **International Young Physicists’ Tournament**, *Problems for the 19th IYPT 2006*, Problem 12: “Rolling magnets” — “Investigate the motion of a magnet as it rolls down an inclined plane.”
    - https://iypt.org/problems/iypt-2006-problems/
    - **Why it matters:** The closest earlier IYPT problem. It is not the same geometry, but student reports and the Tomasel–Marconi paper above treat rolling of strong magnets, including eddy-current damping on conducting surfaces. The IYPT solutions archive is at https://archive.iypt.org/solutions/.

---

## Suggested reading order

1. Official IYPT/TYPT statement (items 1–2), then Křížek’s mentor notes (item 3).
2. **Gissinger (2015)** — almost the same apparatus; start here.
3. **Chau *et al.* (2019)** and **Farago *et al.* (2020)** — phase diagrams and a dynamical-systems theory of the direction reversal.
4. **Mahoney *et al.* (2014)** for step-out / loss of synchronism; **Tierno (2008)** or **Sing *et al.* (2010)** for rolling on a surface.
5. **Halbach (1980)** for the \(e^{-kz}\) field; **Teare (1940)** and **Reichert *et al.* (2012)** or **Schuck *et al.* (2018)** for lag torque; **Saslow (1992)** or **Wiederick *et al.* (1987)** if the plate conducts.
