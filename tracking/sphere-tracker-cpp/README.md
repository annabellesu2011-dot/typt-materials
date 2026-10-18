# sphere-tracker-cpp

A C++17 / OpenCV 4 port of the Python `sphere-tracker`. It measures the **angular speed**
of a specific sphere (silver/metallic by default) orbiting the centre of a plate, filmed by
a camera, and tracks its path.

## Layout

```
CMakeLists.txt
src/tracker.hpp     Options, Detector, geometry (circle fit, unwrap), orbit model, gating
src/tracker.cpp     silver / color / hough detection, candidate scoring, selection
src/analysis.cpp    centre, unwrapped theta, Savitzky-Golay smoothing, omega, CSV, OpenCV plot
src/main.cpp        CLI, capture loop, Kalman / CSRT / orbit prediction, overlay, video output
run_test.sh         headless check on the synthetic silver video (Linux/macOS)
test_output/        results of the bundled test run
```

## Build

### Linux (Debian/Ubuntu)
```bash
sudo apt install build-essential cmake libopencv-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/sphere_tracker --help
```
On Debian/Ubuntu, `libopencv-dev` includes the contrib `tracking` module, so `--tracker csrt` is
enabled automatically. CMake prints whether it found it.

### Windows: vcpkg (recommended)
```powershell
git clone https://github.com/microsoft/vcpkg C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
C:\vcpkg\vcpkg install opencv4[ffmpeg]:x64-windows
#   (optional CSRT tracker: opencv4[contrib,ffmpeg]:x64-windows)
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
build\Release\sphere_tracker.exe --help
```
Run this from a "Developer PowerShell for VS" (Visual Studio 2019/2022 with the C++ workload).
vcpkg copies the OpenCV DLLs next to the exe.

### Windows: prebuilt OpenCV
1. Download the Windows release from https://opencv.org/releases/ and extract it to, for example,
   `C:\opencv`.
2. Configure with OpenCV_DIR pointing at the folder that contains `OpenCVConfig.cmake`:
   ```powershell
   cmake -S . -B build -DOpenCV_DIR=C:/opencv/build
   cmake --build build --config Release
   ```
3. Add `C:\opencv\build\x64\vc16\bin` to `PATH` (or copy `opencv_world4xx.dll` and
   `opencv_videoio_ffmpeg4xx_64.dll` next to the exe).
   The prebuilt package has no contrib modules, so `--tracker csrt` is disabled. Everything else
   works.

## Usage

```bash
# Video: click the target sphere and the plate centre in the first frame
./build/sphere_tracker video.mp4 --pick --click-center --output annotated.mp4

# Live camera 0: click the sphere, centre auto-estimated by a least-squares circle fit
./build/sphere_tracker 0 --pick

# Headless, fully specified
./build/sphere_tracker video.mp4 --no-display --init 593 237 --center 407 295 \
    --min-radius 12 --max-radius 26 --csv track.csv --plot track.png --output annotated.mp4

# Coloured ball: click to auto-pick the HSV range, or give it explicitly
./build/sphere_tracker video.mp4 --mode color --pick --hough-fallback
./build/sphere_tracker video.mp4 --mode color --hsv-lower 0 120 80 --hsv-upper 10 255 255

# Self-test on the synthetic video
./run_test.sh
```
`./build/sphere_tracker --help` lists all options. They match the Python CLI: `--mode`,
`--min/max-radius`, `--hough-*`, `--sat-max`, `--value-min`, `--spec-value`, `--min-score`,
`--bg-sub`, `--pick`, `--init`, `--click-center`, `--center`, `--gate`, `--orbit-tol`,
`--no-orbit-gate`, `--no-kalman`, `--tracker`, `--max-missed`, `--fps`, `--smooth`, `--trail`,
`--output`, `--csv`, `--plot`, `--no-display`, `--max-frames`.
Keys: `q`/`Esc` quits. In the click windows, click and then press any key (`Esc` skips).

### Outputs
* **CSV**: `frame,t,x,y,theta_unwrapped,omega_rad_s,rpm,detected`. On missed frames x/y are
  empty and theta is interpolated.
* **Plot PNG** (drawn with OpenCV, no matplotlib): on the left, the path coloured by time with
  the fitted orbit and centre; on the right, RPM vs time with the mean line and missed frames
  marked. `--plot ""` disables it.
* **Annotated video**: trail (broken at missed frames), centre, radius line, candidates, the
  selected sphere, and live omega.
* **Console**: the mean angular speed from a linear fit of the unwrapped theta(t). This is the
  most accurate single number.
  Sign: **+ = counter-clockwise as seen in the image.**

## How it works (same algorithm as the Python version)
1. **Silver detection**: HoughCircles candidates (within the radius range) plus round blobs from
   a low-saturation, high-brightness mask. Each candidate is scored on low saturation, the
   fraction of specular-highlight pixels, and shading contrast. MOG2 foreground is optional
   (`--bg-sub`).
2. **Lock on one sphere**: `--pick`/`--init`. Prediction comes from the last position, then a
   Kalman filter (constant velocity), then CSRT if enabled, and finally the **orbit model**
   (centre + radius + current omega), which predicts along the circle. That keeps the lock
   through occlusions.
   Gating: candidates must be within `--gate` px of the prediction and within `--orbit-tol` px
   of the target's orbit radius. The second rule rejects decoy spheres on other orbits.
3. **Missed detections** are flagged, and the gate widens slowly. After `--max-missed` frames
   the tracker re-acquires anywhere on the same orbit. Theta is interpolated across the gaps.
4. **Centre**: clicked, given, or found with a least-squares (Kasa) circle fit.
5. **Omega**: `atan2` angle, then unwrap, then Savitzky-Golay quadratic smoothing (`--smooth`),
   then a derivative matching numpy.gradient. Timestamps are frame/FPS for files and a
   steady clock for live cameras.

## Verified result
On `sphere-tracker/test_output/synthetic_silver.mp4`: a silver sphere at **30 RPM** on a
195 px orbit, a silver decoy at -18 RPM on a 110 px orbit, noise, flicker, and an 8-frame
occlusion.

| run | measured | error |
|---|---|---|
| auto centre (circle fit) | 30.005 RPM | 0.017 % |
| given centre | 30.006 RPM | 0.02 % |
| hough mode | 30.002 RPM | 0.007 % |
| color mode + hough fallback | 30.000 RPM | <0.01 % |
| CSRT + bg-sub | 30.003 RPM | 0.01 % |

The target was detected in 292 of 300 frames. The only misses are the 8 occluded frames. Every
detection lies 188–202 px from the centre, so the tracker never switched to the decoy at 110 px.

## Tips
Mount the camera fixed and directly above the plate. Use a dark, matte plate for a silver ball
and diffuse light with a small highlight. Set `--min/--max-radius` close to the real pixel
radius. With an angled camera the orbit looks like an ellipse: the mean RPM stays correct, but
instantaneous omega oscillates. Rectify the plate with a perspective transform for exact
per-frame values.

## Differences from the Python version
* The plot is drawn with OpenCV instead of matplotlib. It is simpler and has no colour bar or
  second rad/s axis.
* There is no synthetic video generator in C++. Use `../sphere-tracker/make_synthetic_video.py`.
* With the prebuilt Windows OpenCV (no contrib), `--tracker csrt` is unavailable.
