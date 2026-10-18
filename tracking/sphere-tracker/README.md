# sphere-tracker

Measure the **angular speed** of a specific sphere orbiting the centre of a
plate, filmed by a camera, and **track its path**. Python + OpenCV.

The default detection mode is built for a **shiny silver / metallic ball**. Plain
colour thresholds don't work well on metal because it reflects whatever is around it.

## Install

```bash
pip install opencv-python numpy scipy matplotlib
# optional, enables the CSRT tracker (--tracker csrt):
pip install opencv-contrib-python
```

(On a headless server `opencv-python-headless` works as well. Use `--no-display` with it.)

## Files

| file | purpose |
|---|---|
| `sphere_tracker.py` | the tracker (CLI) |
| `make_synthetic_video.py` | generates a test video: a silver sphere at a known RPM, a silver distractor sphere, noise, flicker, and an occlusion |
| `test_synthetic.py` | end-to-end headless test that checks the recovered speed is within 2% |
| `test_output/` | results of the bundled test run (CSV, plots, annotated video, results.json) |

## Usage

```bash
# Video file. Click the target sphere and the plate centre in the first frame:
python sphere_tracker.py video.mp4 --pick --click-center --output annotated.mp4

# Live camera 0. Click the sphere; the centre is auto-estimated from the path:
python sphere_tracker.py 0 --pick

# Headless and fully specified (no windows):
python sphere_tracker.py video.mp4 --no-display --init 593 237 --center 407 295 \
    --min-radius 12 --max-radius 26 --csv track.csv --plot track.png --output annotated.mp4

# Coloured (non-metallic) ball. Click it to auto-pick its HSV range:
python sphere_tracker.py video.mp4 --mode color --pick --hough-fallback
# ...or give the HSV range directly (OpenCV H range is 0-180):
python sphere_tracker.py video.mp4 --mode color --hsv-lower 0 120 80 --hsv-upper 10 255 255

# Run the synthetic self-test:
python test_synthetic.py
```

Keys: `q` / `Esc` quits the live window. In the click windows: click, then press any key (`Esc` skips).

### Outputs
* **CSV** columns: `frame, t, x, y, theta_unwrapped, omega_rad_s, rpm, detected`.
  On missed frames `x`/`y` are empty and theta is interpolated.
* **Plot PNG**: the tracked path with the fitted orbit and centre, plus RPM (and rad/s) over time
  with the mean.
* **Annotated video** (`--output`): the path trail, plate centre, radius line, detected sphere,
  and live omega.
* **Console**: the mean angular speed from a linear fit of the unwrapped theta(t). This is the
  most accurate single number.

Sign convention: **+ = counter-clockwise as seen in the image.**

## How it works

1. **Detection** (`--mode silver`, the default)
   * Candidates come from `HoughCircles` on the grey image (set the radius range with
     `--min-radius/--max-radius`). Blobs from a low-saturation, high-brightness mask are added.
   * Each candidate gets a score for low saturation, specular highlight pixels (`--spec-value`),
     and the internal shading contrast that a metal ball shows.
   * `--bg-sub` adds MOG2 background subtraction, which favours moving objects.
2. **Locking onto one specific sphere** (works with several spheres in view)
   * `--pick` (click it) or `--init X Y` sets the target. Without either, the
     best-scoring candidate in the first frame is chosen.
   * A constant-velocity Kalman filter predicts the next position. Once enough of the path is
     known, an **orbit model** takes over and predicts along the circle using the current omega.
   * Only candidates within `--gate` px of the prediction are accepted. Candidates whose distance
     from the centre differs from the target's orbit radius by more than `--orbit-tol` px are
     rejected. This is what stops the tracker from jumping to another sphere on a different
     orbit during an occlusion.
   * `--tracker csrt` adds an OpenCV CSRT tracker, initialised on the clicked sphere. It needs
     opencv-contrib.
3. **Missed detections**: the frame is flagged `detected=0`, and the gate widens slowly. After
   `--max-missed` frames the tracker re-acquires anywhere on the same orbit. Theta is
   interpolated across the gaps.
4. **Centre**: you can click it (`--click-center`) or give it (`--center X Y`). Otherwise a
   least-squares circle fit of the tracked path estimates it.
5. **Angular speed**: `theta = atan2(-(y-cy), x-cx)`, then `np.unwrap`, then Savitzky-Golay
   smoothing (`--smooth` window), then the derivative against the timestamps. Video timestamps
   are frame/FPS (override with `--fps`). Live camera timestamps come from `time.time()`.

## Tips for good measurements
* **Fix the camera directly above the plate**, looking straight down, on a tripod or mount.
  Any camera motion shows up as fake angular motion.
* **Perspective**: an angled camera sees the circular orbit as an ellipse. The mean RPM is still
  right (one turn is still 2*pi), but the instantaneous omega will oscillate twice per turn. For
  best results mount the camera overhead. Otherwise you can undistort/rectify the plate first
  (for example with `cv2.getPerspectiveTransform` from 4 points on the plate rim).
* **Contrast**: a silver ball shows up best on a **dark, matte, non-reflective plate**. Avoid a
  white glossy plate. For a coloured ball, pick a colour that doesn't occur in the background.
* **Lighting**: use diffuse, steady light. One small light gives a clear specular highlight,
  which helps silver mode. Avoid flickering mains light at high shutter speeds.
* **Motion blur**: use a short exposure. At high RPM, use a higher FPS so the ball moves
  less than about 1 radius per frame (or raise `--gate`).
* Set `--min-radius/--max-radius` close to the ball's real pixel radius. It is the biggest
  single robustness knob.
* Shiny spindles, screws or other balls can look like the target. Use `--pick`/`--init` so the
  tracker locks onto the right one, and tune `--orbit-tol`.

## Synthetic test result (bundled run)
The test video has a silver sphere at **30 RPM** counter-clockwise on a 195 px orbit, a silver
distractor at -18 RPM on a 110 px orbit, noise, lighting flicker, and an 8-frame occlusion.

| case | measured | true | error |
|---|---|---|---|
| auto centre (circle fit) | 30.003 RPM | 30.000 RPM | 0.01 % |
| given centre | 30.006 RPM | 30.000 RPM | 0.02 % |

The target was detected in 292 of 300 frames. The 8 misses are exactly the occluded frames, and the tracker never switched to the distractor.
