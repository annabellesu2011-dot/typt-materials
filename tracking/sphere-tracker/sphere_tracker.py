#!/usr/bin/env python3
"""
sphere_tracker.py - measure the angular speed of a sphere orbiting the centre
of a plate, filmed by a (preferably fixed, overhead) camera, and track its path.

Default detection mode ("silver") is designed for shiny metallic balls, where
plain colour thresholds fail:
  * HoughCircles candidates (configurable radius range) on the grey image
  * + blob candidates from a low-saturation / high-value mask
  * each candidate scored on low saturation, specular highlight, and internal
    shading contrast (a metal ball shows a gradient + a bright spot)
  * optional background subtraction (MOG2) to favour moving objects
  * nearest-to-prediction gating with a constant-velocity Kalman filter, so the
    tracker stays locked on ONE specific sphere even when several are visible
  * optional CSRT tracker (needs opencv-contrib-python) initialised by a click

Other modes: "color" (HSV range, optionally picked by clicking) and "hough".

Outputs: CSV (frame,t,x,y,theta_unwrapped,omega_rad_s,rpm), a plot PNG
(path + omega vs time), and optionally an annotated video.
"""
import argparse
import csv
import math
import sys
import time
from collections import deque

import cv2
import numpy as np

try:
    from scipy.signal import savgol_filter
except Exception:  # scipy optional, fall back to moving average
    savgol_filter = None


# ----------------------------------------------------------------------------
# Geometry helpers
# ----------------------------------------------------------------------------
def fit_circle(xs, ys):
    """Algebraic least-squares (Kasa) circle fit. Returns (cx, cy, r)."""
    xs = np.asarray(xs, float)
    ys = np.asarray(ys, float)
    A = np.column_stack([xs, ys, np.ones_like(xs)])
    b = xs ** 2 + ys ** 2
    sol, *_ = np.linalg.lstsq(A, b, rcond=None)
    cx, cy = sol[0] / 2.0, sol[1] / 2.0
    r = math.sqrt(max(sol[2] + cx ** 2 + cy ** 2, 0.0))
    return cx, cy, r


def angles_about(xs, ys, cx, cy):
    """Angle in a math convention (counter-clockwise positive as seen on
    screen; image y-axis points down so it is flipped)."""
    return np.arctan2(-(np.asarray(ys) - cy), np.asarray(xs) - cx)


def smooth(arr, window):
    window = int(window)
    if window < 3 or len(arr) < 5:
        return np.asarray(arr, float)
    if window % 2 == 0:
        window += 1
    window = min(window, len(arr) - (1 - len(arr) % 2))
    if window < 5:
        return np.asarray(arr, float)
    if savgol_filter is not None:
        return savgol_filter(arr, window, 2, mode="interp")
    k = np.ones(window) / window
    pad = window // 2
    a = np.pad(arr, pad, mode="edge")
    return np.convolve(a, k, mode="valid")


# ----------------------------------------------------------------------------
# Detection
# ----------------------------------------------------------------------------
class Detector:
    def __init__(self, args):
        self.a = args
        self.bg = cv2.createBackgroundSubtractorMOG2(history=300, varThreshold=25,
                                                     detectShadows=False) if args.bg_sub else None
        self.lower = np.array(args.hsv_lower, np.uint8)
        self.upper = np.array(args.hsv_upper, np.uint8)

    # -- candidate generators -------------------------------------------------
    def hough_candidates(self, gray):
        a = self.a
        g = cv2.medianBlur(gray, 5)
        circles = cv2.HoughCircles(g, cv2.HOUGH_GRADIENT, dp=a.hough_dp,
                                   minDist=max(a.min_radius * 1.5, 5),
                                   param1=a.hough_param1, param2=a.hough_param2,
                                   minRadius=a.min_radius, maxRadius=a.max_radius)
        if circles is None:
            return []
        return [(float(x), float(y), float(r)) for x, y, r in circles[0]]

    def blob_candidates(self, mask):
        a = self.a
        mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, np.ones((3, 3), np.uint8))
        mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, np.ones((5, 5), np.uint8))
        cnts, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        out = []
        for c in cnts:
            area = cv2.contourArea(c)
            if area < 10:
                continue
            (x, y), r = cv2.minEnclosingCircle(c)
            if not (a.min_radius * 0.6 <= r <= a.max_radius * 1.3):
                continue
            circ = area / (math.pi * r * r + 1e-9)
            if circ < 0.45:
                continue
            m = cv2.moments(c)
            if m["m00"] > 0:
                x, y = m["m10"] / m["m00"], m["m01"] / m["m00"]
            out.append((float(x), float(y), float(r)))
        return out

    # -- scoring ---------------------------------------------------------------
    def silver_score(self, hsv, fg, x, y, r):
        h, w = hsv.shape[:2]
        r = max(r, 2.0)
        x0, x1 = int(max(x - r, 0)), int(min(x + r + 1, w))
        y0, y1 = int(max(y - r, 0)), int(min(y + r + 1, h))
        if x1 - x0 < 2 or y1 - y0 < 2:
            return 0.0
        patch = hsv[y0:y1, x0:x1]
        yy, xx = np.mgrid[y0:y1, x0:x1]
        inside = (xx - x) ** 2 + (yy - y) ** 2 <= (r * 0.9) ** 2
        if inside.sum() < 5:
            return 0.0
        S = patch[..., 1][inside].astype(float)
        V = patch[..., 2][inside].astype(float)
        low_sat = 1.0 - min(np.mean(S) / max(self.a.sat_max, 1), 1.0)
        spec = np.mean((V >= self.a.spec_value) & (S <= self.a.sat_max))
        highlight = min(spec / 0.02, 1.0)            # >=2% of pixels specular
        contrast = min(np.std(V) / 40.0, 1.0)         # metal shading gradient
        score = 0.35 * low_sat + 0.35 * highlight + 0.30 * contrast
        if fg is not None:
            fgp = fg[y0:y1, x0:x1][inside]
            score = 0.7 * score + 0.3 * float(np.mean(fgp > 0))
        return score

    def color_score(self, mask, x, y, r):
        h, w = mask.shape
        x0, x1 = int(max(x - r, 0)), int(min(x + r + 1, w))
        y0, y1 = int(max(y - r, 0)), int(min(y + r + 1, h))
        if x1 <= x0 or y1 <= y0:
            return 0.0
        return float(np.mean(mask[y0:y1, x0:x1] > 0))

    # -- main ------------------------------------------------------------------
    def candidates(self, frame):
        """Return list of (x, y, r, score) for the current frame."""
        a = self.a
        hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        fg = self.bg.apply(frame) if self.bg is not None else None
        cands = []
        if a.mode == "silver":
            silver_mask = cv2.inRange(hsv, (0, 0, a.value_min), (180, a.sat_max, 255))
            raw = self.hough_candidates(gray) + self.blob_candidates(silver_mask)
            for x, y, r in raw:
                s = self.silver_score(hsv, fg, x, y, r)
                if s >= a.min_score:
                    cands.append((x, y, r, s))
        elif a.mode == "color":
            mask = cv2.inRange(hsv, self.lower, self.upper)
            if fg is not None:
                mask = cv2.bitwise_and(mask, fg)
            for x, y, r in self.blob_candidates(mask):
                cands.append((x, y, r, 1.0))
            if not cands and a.hough_fallback:
                for x, y, r in self.hough_candidates(gray):
                    cands.append((x, y, r, self.color_score(mask, x, y, r)))
        else:  # hough
            for x, y, r in self.hough_candidates(gray):
                cands.append((x, y, r, 1.0))
        return cands


def make_kalman(x, y, dt):
    kf = cv2.KalmanFilter(4, 2)
    kf.transitionMatrix = np.array([[1, 0, dt, 0], [0, 1, 0, dt],
                                    [0, 0, 1, 0], [0, 0, 0, 1]], np.float32)
    kf.measurementMatrix = np.array([[1, 0, 0, 0], [0, 1, 0, 0]], np.float32)
    kf.processNoiseCov = np.eye(4, dtype=np.float32) * 5.0
    kf.measurementNoiseCov = np.eye(2, dtype=np.float32) * 2.0
    kf.errorCovPost = np.eye(4, dtype=np.float32) * 10.0
    kf.statePost = np.array([[x], [y], [0], [0]], np.float32)
    return kf


def make_csrt():
    for factory in ("TrackerCSRT_create",):
        if hasattr(cv2, factory):
            return getattr(cv2, factory)()
    if hasattr(cv2, "legacy") and hasattr(cv2.legacy, "TrackerCSRT_create"):
        return cv2.legacy.TrackerCSRT_create()
    return None


# ----------------------------------------------------------------------------
# Interactive helpers (only used when a display is available)
# ----------------------------------------------------------------------------
def click_point(frame, title):
    pts = []

    def cb(ev, x, y, *_):
        if ev == cv2.EVENT_LBUTTONDOWN:
            pts.append((x, y))

    disp = frame.copy()
    cv2.putText(disp, title + "  (click, then any key; ESC = skip)", (10, 25),
                cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
    cv2.namedWindow(title)
    cv2.setMouseCallback(title, cb)
    while True:
        d = disp.copy()
        for p in pts[-1:]:
            cv2.drawMarker(d, p, (0, 0, 255), cv2.MARKER_CROSS, 20, 2)
        cv2.imshow(title, d)
        k = cv2.waitKey(20) & 0xFF
        if k == 27:
            pts = []
            break
        if k != 255 and pts:
            break
    cv2.destroyWindow(title)
    return pts[-1] if pts else None


def hsv_range_from_point(frame, x, y, half=4, dh=10, ds=60, dv=60):
    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
    patch = hsv[max(y - half, 0):y + half + 1, max(x - half, 0):x + half + 1].reshape(-1, 3)
    med = np.median(patch, axis=0)
    lo = [max(med[0] - dh, 0), max(med[1] - ds, 0), max(med[2] - dv, 0)]
    hi = [min(med[0] + dh, 180), min(med[1] + ds, 255), min(med[2] + dv, 255)]
    return [int(v) for v in lo], [int(v) for v in hi]


# ----------------------------------------------------------------------------
# Main processing
# ----------------------------------------------------------------------------
def parse_args(argv=None):
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("source", help="video file path or camera index (e.g. 0)")
    p.add_argument("--mode", choices=["silver", "color", "hough"], default="silver",
                   help="detection mode (default: silver = metallic ball)")
    p.add_argument("--hsv-lower", nargs=3, type=int, default=[0, 0, 150], metavar=("H", "S", "V"))
    p.add_argument("--hsv-upper", nargs=3, type=int, default=[180, 60, 255], metavar=("H", "S", "V"))
    p.add_argument("--hough-fallback", action="store_true", help="color mode: use HoughCircles when colour fails")
    p.add_argument("--min-radius", type=int, default=8, help="min sphere radius in px")
    p.add_argument("--max-radius", type=int, default=60, help="max sphere radius in px")
    p.add_argument("--hough-dp", type=float, default=1.2)
    p.add_argument("--hough-param1", type=float, default=100)
    p.add_argument("--hough-param2", type=float, default=22, help="lower = more circles")
    p.add_argument("--sat-max", type=int, default=60, help="silver: max saturation of metal pixels")
    p.add_argument("--value-min", type=int, default=120, help="silver: min brightness for mask")
    p.add_argument("--spec-value", type=int, default=235, help="silver: brightness of specular highlight")
    p.add_argument("--min-score", type=float, default=0.35, help="silver: min candidate score")
    p.add_argument("--bg-sub", action="store_true", help="use MOG2 background subtraction to favour moving objects")
    p.add_argument("--pick", action="store_true",
                   help="click the target sphere in the first frame (locks onto it; in color mode also picks HSV)")
    p.add_argument("--init", nargs=2, type=float, metavar=("X", "Y"),
                   help="initial position of the target sphere (non-interactive lock)")
    p.add_argument("--center", nargs=2, type=float, metavar=("X", "Y"), help="plate centre in px")
    p.add_argument("--click-center", action="store_true", help="click the plate centre in the first frame")
    p.add_argument("--gate", type=float, default=60.0, help="max px jump from predicted position")
    p.add_argument("--orbit-tol", type=float, default=25.0,
                   help="reject candidates whose distance from the centre differs from the target's orbit radius by more than this (px)")
    p.add_argument("--no-orbit-gate", action="store_true", help="disable orbit-aware prediction/gating")
    p.add_argument("--no-kalman", action="store_true", help="disable Kalman prediction (gate around last position)")
    p.add_argument("--tracker", choices=["none", "csrt"], default="none",
                   help="add a CSRT tracker (needs opencv-contrib-python)")
    p.add_argument("--max-missed", type=int, default=15, help="frames without detection before re-acquiring globally")
    p.add_argument("--fps", type=float, default=None, help="override video FPS")
    p.add_argument("--smooth", type=int, default=15, help="Savitzky-Golay window (frames)")
    p.add_argument("--trail", type=int, default=300, help="path trail length drawn on overlay")
    p.add_argument("--output", help="annotated output video (.mp4/.avi)")
    p.add_argument("--csv", default="sphere_track.csv")
    p.add_argument("--plot", default="sphere_track.png")
    p.add_argument("--no-display", action="store_true", help="headless: no GUI windows")
    p.add_argument("--max-frames", type=int, default=0)
    return p.parse_args(argv)


def orbit_model(xs, ys, ts, center, min_pts=20, k=15):
    """Fit the orbit from recent good detections: returns
    (cx, cy, radius, theta_last, omega, t_last) or None."""
    xs = np.asarray(xs[-400:], float); ys = np.asarray(ys[-400:], float); ts = np.asarray(ts[-400:], float)
    g = ~np.isnan(xs)
    if g.sum() < min_pts:
        return None
    gx, gy, gt = xs[g], ys[g], ts[g]
    if center is None:
        cx, cy, _ = fit_circle(gx, gy)
    else:
        cx, cy = center
    rad = np.hypot(gx - cx, gy - cy)
    # need the path to span some angle before trusting a fitted centre
    th = np.unwrap(angles_about(gx, gy, cx, cy))
    if center is None and abs(th[-1] - th[0]) < 1.0:
        return None
    kk = min(k, len(th))
    omega = np.polyfit(gt[-kk:], th[-kk:], 1)[0] if gt[-1] > gt[-kk] else 0.0
    return cx, cy, float(np.median(rad[-60:])), th[-1], omega, gt[-1]


def predict_on_orbit(model, t):
    cx, cy, R, th, w, tl = model
    a = th + w * (t - tl)
    return cx + R * math.cos(a), cy - R * math.sin(a)


def select(cands, pred, gate, locked, orbit=None, orbit_tol=None):
    """Pick a candidate. When locked, nearest to prediction within gate wins
    (with a small score bonus); if an orbit model is known, candidates whose
    distance from the centre differs from the orbit radius by more than
    orbit_tol are rejected (this rejects spheres on other orbits).
    Otherwise the highest-score candidate."""
    if not cands:
        return None
    if orbit is not None and orbit_tol:
        cx, cy, R = orbit[:3]
        cands = [c for c in cands if abs(math.hypot(c[0] - cx, c[1] - cy) - R) <= orbit_tol]
        if not cands:
            return None
    if locked and pred is not None:
        best, bestc = None, 1e18
        for c in cands:
            d = math.hypot(c[0] - pred[0], c[1] - pred[1])
            if d > gate:
                continue
            cost = d / gate + 0.3 * (1.0 - c[3])
            if cost < bestc:
                best, bestc = c, cost
        return best
    return max(cands, key=lambda c: c[3])


def run(args):
    src = int(args.source) if args.source.isdigit() else args.source
    live = isinstance(src, int)
    cap = cv2.VideoCapture(src)
    if not cap.isOpened():
        sys.exit(f"Cannot open source {args.source}")
    fps = args.fps or cap.get(cv2.CAP_PROP_FPS) or 30.0
    if fps <= 1 or fps > 1000:
        fps = 30.0
    ok, frame = cap.read()
    if not ok:
        sys.exit("Cannot read first frame")
    H, W = frame.shape[:2]
    display = not args.no_display

    init = tuple(args.init) if args.init else None
    center = tuple(args.center) if args.center else None
    if display and args.pick:
        pt = click_point(frame, "Click the target sphere")
        if pt:
            init = pt
            if args.mode == "color":
                args.hsv_lower, args.hsv_upper = hsv_range_from_point(frame, *pt)
                print(f"Picked HSV range lower={args.hsv_lower} upper={args.hsv_upper}")
    if display and args.click_center:
        pt = click_point(frame, "Click the plate centre")
        if pt:
            center = pt
    elif (args.pick or args.click_center) and not display:
        print("Note: --pick/--click-center ignored with --no-display (use --init / --center)")

    det = Detector(args)
    writer = None
    if args.output:
        fourcc = cv2.VideoWriter_fourcc(*("mp4v" if args.output.lower().endswith(".mp4") else "XVID"))
        writer = cv2.VideoWriter(args.output, fourcc, fps, (W, H))

    dt = 1.0 / fps
    kf = None
    tracker = None
    if args.tracker == "csrt":
        tracker = make_csrt()
        if tracker is None:
            print("CSRT not available (pip install opencv-contrib-python); continuing without it")
    tracker_ok = False

    last = init
    locked = init is not None
    missed = 0
    frames, ts, xs, ys, rs = [], [], [], [], []
    trail = deque(maxlen=args.trail)
    t0 = time.time()
    idx = 0
    while ok:
        t = (time.time() - t0) if live else idx / fps
        cands = det.candidates(frame)

        # prediction
        pred = last
        if kf is not None:
            p = kf.predict()
            pred = (float(p[0, 0]), float(p[1, 0]))
        if tracker is not None and tracker_ok:
            tok, bb = tracker.update(frame)
            if tok:
                pred = (bb[0] + bb[2] / 2.0, bb[1] + bb[3] / 2.0)
        # orbit-aware prediction (circular motion about the plate centre):
        # much better than constant velocity when the sphere is occluded
        orbit = None if args.no_orbit_gate else orbit_model(xs, ys, ts, center)
        if orbit is not None:
            pred = predict_on_orbit(orbit, t)

        # widen gate slowly while missing
        gate = args.gate * (1.0 + 0.08 * missed)
        sel = select(cands, pred, gate, locked, orbit, args.orbit_tol)
        if sel is None and locked and missed >= args.max_missed and last is not None:
            # lost: re-acquire anywhere on the same orbit (or nearest to last position)
            sel = select(cands, pred if pred is not None else last, max(W, H), True, orbit, args.orbit_tol)

        if sel is not None:
            x, y, r, _ = sel
            missed = 0
            locked = True
            if kf is None and not args.no_kalman:
                kf = make_kalman(x, y, dt)
            elif kf is not None:
                kf.correct(np.array([[x], [y]], np.float32))
            if tracker is not None and not tracker_ok:
                s = int(2.4 * r)
                tracker.init(frame, (int(x - s / 2), int(y - s / 2), s, s))
                tracker_ok = True
            last = (x, y)
            frames.append(idx); ts.append(t); xs.append(x); ys.append(y); rs.append(r)
            trail.append((int(round(x)), int(round(y))))
        else:
            missed += 1
            frames.append(idx); ts.append(t); xs.append(np.nan); ys.append(np.nan); rs.append(np.nan)
            trail.append(None)  # break the trail at missed detections

        # live centre/omega estimate for overlay
        if display or writer is not None:
            vis = frame.copy()
            c_live = center
            vx = np.array(xs[-600:]); vy = np.array(ys[-600:]); vt = np.array(ts[-600:])
            good = ~np.isnan(vx)
            if c_live is None and good.sum() >= 15:
                cx_, cy_, _ = fit_circle(vx[good], vy[good])
                c_live = (cx_, cy_)
            omega_live = None
            if c_live is not None and good.sum() >= 5:
                k = min(int(fps), 30)
                gx, gy, gt = vx[good][-k:], vy[good][-k:], vt[good][-k:]
                th = np.unwrap(angles_about(gx, gy, *c_live))
                if len(th) >= 3 and gt[-1] > gt[0]:
                    omega_live = np.polyfit(gt, th, 1)[0]
            seg = []
            for ptt in list(trail) + [None]:
                if ptt is None:
                    if len(seg) > 1:
                        cv2.polylines(vis, [np.array(seg, np.int32)], False, (0, 255, 255), 2)
                    seg = []
                else:
                    seg.append(ptt)
            if c_live is not None:
                cc = (int(round(c_live[0])), int(round(c_live[1])))
                cv2.drawMarker(vis, cc, (255, 0, 255), cv2.MARKER_CROSS, 24, 2)
                if sel is not None:
                    cv2.line(vis, cc, (int(sel[0]), int(sel[1])), (255, 0, 255), 1)
            for c in cands:
                cv2.circle(vis, (int(c[0]), int(c[1])), int(c[2]), (128, 128, 128), 1)
            if sel is not None:
                cv2.circle(vis, (int(sel[0]), int(sel[1])), int(sel[2]), (0, 255, 0), 2)
            elif pred is not None:
                cv2.circle(vis, (int(pred[0]), int(pred[1])), 6, (0, 0, 255), 2)
            txt = "omega: --" if omega_live is None else \
                f"omega: {omega_live:+.3f} rad/s  {omega_live * 60 / (2 * math.pi):+.2f} RPM"
            cv2.putText(vis, txt, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 0), 4)
            cv2.putText(vis, txt, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
            cv2.putText(vis, f"t={t:6.2f}s  frame {idx}" + ("  MISSED" if sel is None else ""),
                        (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
            if writer is not None:
                writer.write(vis)
            if display:
                cv2.imshow("sphere tracker (q to quit)", vis)
                if (cv2.waitKey(1) & 0xFF) in (ord("q"), 27):
                    break

        idx += 1
        if args.max_frames and idx >= args.max_frames:
            break
        ok, frame = cap.read()

    cap.release()
    if writer is not None:
        writer.release()
    if display:
        cv2.destroyAllWindows()
    return analyse(args, np.array(frames), np.array(ts, float), np.array(xs, float),
                   np.array(ys, float), center, (W, H))


def analyse(args, frames, ts, xs, ys, center, size):
    good = ~np.isnan(xs)
    n_good = int(good.sum())
    if n_good < 5:
        sys.exit(f"Only {n_good} detections - check detection parameters")
    if center is None:
        cx, cy, R = fit_circle(xs[good], ys[good])
        print(f"Auto centre (circle fit): ({cx:.1f}, {cy:.1f}), orbit radius {R:.1f}px")
    else:
        cx, cy = center
        R = float(np.nanmean(np.hypot(xs - cx, ys - cy)))
        print(f"Centre: ({cx:.1f}, {cy:.1f}), mean orbit radius {R:.1f}px")

    theta_good = np.unwrap(angles_about(xs[good], ys[good], cx, cy))
    t_good = ts[good]
    # interpolate missed frames so the derivative is computed on a full grid
    theta_all = np.interp(ts, t_good, theta_good)
    theta_s = smooth(theta_all, args.smooth)
    omega = np.gradient(theta_s, ts) if len(ts) > 1 else np.zeros_like(ts)
    rpm = omega * 60.0 / (2 * math.pi)
    slope = np.polyfit(t_good, theta_good, 1)[0]
    mean_rpm = slope * 60 / (2 * math.pi)
    print(f"Detections: {n_good}/{len(xs)} frames ({100.0 * n_good / len(xs):.1f}%)")
    print(f"Mean angular speed (linear fit of theta(t)): {slope:+.4f} rad/s = {mean_rpm:+.3f} RPM")
    print(f"Median smoothed omega: {np.median(rpm[good]):+.3f} RPM  (sign: + = counter-clockwise on screen)")

    with open(args.csv, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["frame", "t", "x", "y", "theta_unwrapped", "omega_rad_s", "rpm", "detected"])
        for i in range(len(ts)):
            w.writerow([int(frames[i]), f"{ts[i]:.4f}",
                        "" if np.isnan(xs[i]) else f"{xs[i]:.2f}",
                        "" if np.isnan(ys[i]) else f"{ys[i]:.2f}",
                        f"{theta_all[i]:.5f}", f"{omega[i]:.5f}", f"{rpm[i]:.4f}", int(good[i])])
    print(f"CSV  -> {args.csv}")

    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        fig, (a1, a2) = plt.subplots(1, 2, figsize=(13, 5.5))
        sc = a1.scatter(xs[good], ys[good], c=t_good, s=4, cmap="viridis")
        a1.plot([cx], [cy], "m+", ms=14, mew=2, label="centre")
        a1.add_patch(plt.Circle((cx, cy), R, fill=False, ls="--", color="gray", label="fitted orbit"))
        a1.set_xlim(0, size[0]); a1.set_ylim(size[1], 0); a1.set_aspect("equal")
        a1.set_title("Tracked path (image px)"); a1.legend(loc="upper right")
        fig.colorbar(sc, ax=a1, label="t [s]")
        a2.plot(ts, rpm, "b-", lw=1.2, label="smoothed RPM")
        a2.axhline(mean_rpm, color="r", ls="--", label=f"mean {mean_rpm:.2f} RPM")
        miss = ~good
        if miss.any():
            a2.plot(ts[miss], rpm[miss], "rx", ms=4, label="interpolated (missed)")
        a2.set_xlabel("t [s]"); a2.set_ylabel("RPM"); a2.grid(alpha=0.3)
        ax2 = a2.twinx(); lo, hi = a2.get_ylim()
        ax2.set_ylim(lo * 2 * math.pi / 60, hi * 2 * math.pi / 60); ax2.set_ylabel("rad/s")
        a2.set_title("Angular speed vs time"); a2.legend(loc="lower right")
        fig.tight_layout(); fig.savefig(args.plot, dpi=110); plt.close(fig)
        print(f"Plot -> {args.plot}")
    except Exception as e:
        print(f"Plot skipped: {e}")
    return {"mean_rpm": mean_rpm, "omega_rad_s": slope, "center": (cx, cy), "radius": R,
            "detected": n_good, "frames": len(xs)}


if __name__ == "__main__":
    run(parse_args())
