#!/usr/bin/env python3
"""Generate a synthetic test video: a SILVER (metallic-looking) sphere orbiting
the centre of a plate at a known speed, plus a second silver distractor sphere
on a different orbit, sensor noise, brightness flicker and a short occlusion.
Writes the video and a JSON file with the ground truth."""
import argparse
import json
import math

import cv2
import numpy as np


def silver_sprite(r, ss=4):
    """Render an anti-aliased metallic sphere sprite (BGR float + alpha)."""
    n = 2 * r + 4
    N = n * ss
    yy, xx = np.mgrid[0:N, 0:N].astype(np.float32)
    c = N / 2.0
    nx = (xx - c) / (r * ss); ny = (yy - c) / (r * ss)
    d2 = nx ** 2 + ny ** 2
    inside = d2 <= 1.0
    nz = np.sqrt(np.clip(1 - d2, 0, 1))
    # environment reflection: reflected vector's y picks a grey gradient
    # (bright "sky" top, dark band, mid "floor") -> typical chrome look
    ry = 2 * nz * ny  # reflected y for view along -z
    env = np.interp(ry, [-1, -0.35, -0.05, 0.15, 0.5, 1], [235, 200, 70, 110, 150, 120])
    light = np.array([-0.45, -0.55, 0.70]); light /= np.linalg.norm(light)
    ndotl = np.clip(nx * light[0] + ny * light[1] + nz * light[2], 0, 1)
    hvec = light + np.array([0, 0, 1.0]); hvec /= np.linalg.norm(hvec)
    ndoth = np.clip(nx * hvec[0] + ny * hvec[1] + nz * hvec[2], 0, 1)
    spec = 255 * ndoth ** 80
    rim = (1 - nz) ** 3 * 60  # darker silhouette edge
    val = np.clip(0.65 * env + 0.25 * 255 * ndotl + spec - rim, 0, 255)
    img = np.stack([val * 1.02, val * 1.0, val * 0.97], -1)  # very slightly cool tint
    img = np.clip(img, 0, 255)
    alpha = inside.astype(np.float32)
    img = cv2.resize(img, (n, n), interpolation=cv2.INTER_AREA)
    alpha = cv2.resize(alpha, (n, n), interpolation=cv2.INTER_AREA)
    return img.astype(np.float32), alpha[..., None]


def paste(frame, sprite, alpha, x, y):
    n = sprite.shape[0]
    # sub-pixel placement via affine warp of the sprite
    M = np.float32([[1, 0, x - n / 2.0], [0, 1, y - n / 2.0]])
    H, W = frame.shape[:2]
    spr = cv2.warpAffine(sprite, M, (W, H), flags=cv2.INTER_LINEAR)
    a = cv2.warpAffine(alpha, M, (W, H), flags=cv2.INTER_LINEAR)[..., None]
    frame[:] = frame * (1 - a) + spr * a
    # soft contact shadow is drawn separately


def background(W, H, cx, cy, R, rng):
    bg = np.zeros((H, W, 3), np.float32)
    # wooden table
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    grain = 20 * np.sin(xx / 9.0 + 3 * np.sin(yy / 40.0)) + rng.normal(0, 6, (H, W))
    bg[..., 0] = 40 + grain * 0.4
    bg[..., 1] = 75 + grain * 0.7
    bg[..., 2] = 120 + grain
    # plate: matte blue-grey ceramic with rim and a soft radial gradient
    d = np.hypot(xx - cx, yy - cy)
    plate = d <= R
    grad = 1 - 0.18 * (d / R) ** 2
    for ch, v in enumerate((150, 128, 112)):
        bg[..., ch] = np.where(plate, v * grad + rng.normal(0, 3, (H, W)), bg[..., ch])
    rim = (d > R - 10) & (d <= R)
    bg[rim] = bg[rim] * 0.8
    cv2.circle(bg, (int(cx), int(cy)), 6, (60, 60, 60), -1)  # centre spindle
    return np.clip(bg, 0, 255)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--out", default="synthetic_silver.mp4")
    p.add_argument("--rpm", type=float, default=30.0, help="target speed, + = counter-clockwise on screen")
    p.add_argument("--distractor-rpm", type=float, default=-18.0)
    p.add_argument("--fps", type=float, default=30.0)
    p.add_argument("--seconds", type=float, default=10.0)
    p.add_argument("--size", nargs=2, type=int, default=[800, 600])
    p.add_argument("--seed", type=int, default=1)
    a = p.parse_args()

    rng = np.random.default_rng(a.seed)
    W, H = a.size
    cx, cy, R_plate = W / 2 + 7, H / 2 - 5, 270
    r_target, orbit_t = 18, 195
    r_dis, orbit_d = 18, 110
    bg = background(W, H, cx, cy, R_plate, rng)
    sprite, alpha = silver_sprite(r_target)
    sprite_d, alpha_d = silver_sprite(r_dis)
    w_t = a.rpm * 2 * math.pi / 60
    w_d = a.distractor_rpm * 2 * math.pi / 60
    th0_t, th0_d = 0.3, 2.0

    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    vw = cv2.VideoWriter(a.out, fourcc, a.fps, (W, H))
    nframes = int(round(a.seconds * a.fps))
    occl = range(int(4.0 * a.fps), int(4.0 * a.fps) + 8)  # ~0.27 s occlusion
    init = None
    for i in range(nframes):
        t = i / a.fps
        f = bg.copy()
        pos = []
        for th0, w, orb, spr, al in ((th0_t, w_t, orbit_t, sprite, alpha), (th0_d, w_d, orbit_d, sprite_d, alpha_d)):
            th = th0 + w * t
            x = cx + orb * math.cos(th)
            y = cy - orb * math.sin(th)  # screen y down -> CCW for positive w
            cv2.circle(f, (int(x + 4), int(y + 5)), 19, tuple(float(v) * 0.75 for v in (150, 128, 112)), -1, cv2.LINE_AA)
            pos.append((x, y))

        for (x, y), spr, al in zip(pos, (sprite, sprite_d), (alpha, alpha_d)):
            paste(f, spr, al, x, y)
        if i == 0:
            init = pos[0]
        if i in occl:  # a "hand" passes over the target sphere
            x, y = pos[0]
            cv2.rectangle(f, (int(x - 40), int(y - 60)), (int(x + 45), int(y + 50)), (110, 150, 200), -1)
        f *= 1 + 0.04 * math.sin(2 * math.pi * 0.7 * t)  # lighting flicker
        f += rng.normal(0, 4, f.shape)
        vw.write(np.clip(f, 0, 255).astype(np.uint8))
    vw.release()
    truth = {"video": a.out, "true_rpm": a.rpm, "distractor_rpm": a.distractor_rpm, "fps": a.fps,
             "frames": nframes, "center": [cx, cy], "target_orbit_px": orbit_t,
             "distractor_orbit_px": orbit_d, "sphere_radius_px": r_target,
             "target_initial_xy": [round(init[0], 1), round(init[1], 1)],
             "occluded_frames": [occl.start, occl.stop - 1]}
    with open(a.out.rsplit(".", 1)[0] + "_truth.json", "w") as fh:
        json.dump(truth, fh, indent=2)
    print(json.dumps(truth, indent=2))


if __name__ == "__main__":
    main()
