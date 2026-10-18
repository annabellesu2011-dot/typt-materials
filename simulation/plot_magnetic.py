#!/usr/bin/env python3
"""Plots for magnetic_carousel.cpp output.   Usage: python3 plot_magnetic.py [out_dir=out_mag]"""
import csv, math, os, sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

D = sys.argv[1] if len(sys.argv) > 1 else "out_mag"
os.makedirs(D, exist_ok=True)


def load(name):
    with open(os.path.join(D, name)) as f:
        rows = [r for r in csv.reader(f) if r and not r[0].startswith("#")]
    hdr, data = rows[0], rows[1:]
    out = {}
    for j, k in enumerate(hdr):
        col = []
        for r in data:
            try:
                col.append(float(r[j]))
            except ValueError:
                col.append(r[j])
        out[k] = np.array(col) if all(isinstance(c, float) for c in col) else col
    return out


def save(fig, name):
    fig.tight_layout()
    p = os.path.join(D, name)
    fig.savefig(p, dpi=130)
    plt.close(fig)
    print("wrote", p)


P = {k: v for k, v in zip(load("params.csv")["name"], load("params.csv")["value"])}

# ---------------- omega_ball vs Omega ----------------
def panel_omega(ax, s):
    Om = s["Omega"]
    ax.fill_between(Om, s["omega_min"], s["omega_max"], color="C0", alpha=.25, lw=0, label="range over start phases")
    ax.plot(Om, s["omega_ball"], "C0.-", ms=3, label="mean (non-escaped)")
    ax.plot(Om, Om, "k--", lw=.8, label=r"$\omega_{ball}=\Omega$")
    esc = s["f_escape"] > 0.5
    if esc.any():
        ax.plot(Om[esc], np.zeros(esc.sum()), "rx", ms=4, label="mostly escaped")
    ax.axhline(0, color="gray", lw=.5)
    ax.set_xlabel(r"disc speed $\Omega$ [rad/s]"); ax.set_ylabel(r"$\omega_{ball}$ [rad/s]")
    ax.set_title(r"Ball orbital speed vs $\Omega$"); ax.legend(fontsize=7); ax.grid(alpha=.3)

def panel_ratio(ax, s):
    Om = s["Omega"]
    ax.plot(Om, s["ratio"], "C1.-", ms=3)
    ax.axhline(1, color="k", ls="--", lw=.8); ax.axhline(0, color="gray", lw=.5)
    ax.set_xlabel(r"$\Omega$ [rad/s]"); ax.set_ylabel(r"$\omega_{ball}/\Omega$")
    ax.set_title("co-rotation (+1) vs counter-rotation (<0)"); ax.grid(alpha=.3)

s = load("sweep_Omega.csv")
fig, ax = plt.subplots(3, 1, figsize=(8, 10), sharex=True)
panel_omega(ax[0], s); panel_ratio(ax[1], s)
ax[2].stackplot(s["Omega"], s["f_lock"], s["f_drift"], s["f_counter"], s["f_escape"],
                labels=["locked co-rotation", "drift", "counter-rotation", "escaped"],
                colors=["tab:green", "tab:gray", "tab:blue", "tab:red"], alpha=.7)
ax[2].set_ylabel("fraction of start phases"); ax[2].set_xlabel(r"$\Omega$ [rad/s]"); ax[2].legend(fontsize=7, loc="center right")
ax[2].set_ylim(0, 1)
save(fig, "omega_vs_Omega.png")

# ---------------- hysteresis ramps ----------------
def panel_ramp(ax, r):
    for d, c in (("up", "C2"), ("down", "C3")):
        ph = sorted(set(int(p) for p, dd in zip(r["phase"], r["direction"]) if dd == d))
        for k in ph:
            idx = [i for i in range(len(r["t"])) if r["direction"][i] == d and int(r["phase"][i]) == k]
            ax.plot(r["Omega"][idx], r["omega_ball_window"][idx], color=c, lw=.8, alpha=.7,
                    label=f"ramp {d}" if k == ph[0] else None)
    fin = np.isfinite(r["omega_ball_window"])
    omx = max(r["Omega"][fin]) * 1.1 if fin.any() else max(r["Omega"])
    om = np.linspace(0, omx, 10)
    ax.plot(om, om, "k--", lw=.7, label=r"$\omega_{ball}=\Omega$")
    ax.set_xlim(0, omx)
    ax.text(0.02, 0.02, "curves end where the ball is thrown off the magnet ring", transform=ax.transAxes, fontsize=7)
    ax.axhline(0, color="gray", lw=.5)
    ax.set_xlabel(r"$\Omega$ [rad/s]"); ax.set_ylabel(r"$\omega_{ball}$ (0.25 s window) [rad/s]")
    ax.set_title("Slow ramps (hysteresis)"); ax.legend(fontsize=8); ax.grid(alpha=.3)

if os.path.exists(os.path.join(D, "ramp.csv")):
    r = load("ramp.csv")
    fig, ax = plt.subplots(figsize=(8, 5)); panel_ramp(ax, r); save(fig, "ramp_hysteresis.png")

# ---------------- 1-D parameter sweeps ----------------
LABEL = {"h_mm": "gap h [mm]", "N": "number of magnets N", "a_mm": "ball radius a [mm]",
         "tau_ms": r"magnetisation lag $\tau$ [ms]", "mu": r"friction coefficient $\mu$",
         "mu_rr_len": "rolling-resistance length [m]"}
LOGX = {"tau_ms", "mu_rr_len"}

def panel_var_omega(ax, var, sw, ratio=False):
    for Om in sorted(set(sw["Omega"])):
        m = sw["Omega"] == Om
        y = sw["ratio"][m] if ratio else sw["omega_ball"][m]
        ax.plot(sw[var][m], y, "o-", ms=4, label=fr"$\Omega$={Om:g} rad/s")
        esc = m & (sw["f_escape"] > 0.5)
        if esc.any(): ax.plot(sw[var][esc], np.zeros(esc.sum()), "rx")
    ax.axhline(0, color="gray", lw=.5)
    if ratio: ax.axhline(1, color="k", ls="--", lw=.7)
    ax.set_xlabel(LABEL[var]); ax.set_ylabel(r"$\omega_{ball}/\Omega$" if ratio else r"$\omega_{ball}$ [rad/s]")
    if var in LOGX: ax.set_xscale("log")
    ax.legend(fontsize=7); ax.grid(alpha=.3)

def panel_crit(ax, var, cr):
    ax.plot(cr[var], cr["Omega_unlock"], "gs-", label=r"$\Omega_{unlock}$ (co-rotation lost)")
    ax.plot(cr[var], cr["Omega_counter"], "b^-", label=r"$\Omega_{counter}$ (stable counter-rot.)")
    ax.plot(cr[var], cr["Omega_capture_estimate"], "k:", label="capture estimate")
    ax.set_xlabel(LABEL[var]); ax.set_ylabel(r"critical $\Omega$ [rad/s]")
    if var in LOGX: ax.set_xscale("log")
    ax.legend(fontsize=7); ax.grid(alpha=.3)

for var in LABEL:
    fn = f"sweep_{var}.csv"
    if not os.path.exists(os.path.join(D, fn)): continue
    sw, cr = load(fn), load(f"crit_{var}.csv")
    fig, ax = plt.subplots(1, 3, figsize=(15, 4.3))
    panel_var_omega(ax[0], var, sw); panel_var_omega(ax[1], var, sw, ratio=True); panel_crit(ax[2], var, cr)
    fig.suptitle(f"Dependence on {LABEL[var]}")
    save(fig, f"sweep_{var}.png")

# ---------------- 2-D map ----------------
def panel_map(ax, fig):
    mp = load("map_Omega_h.csv")
    hs, Oms = np.unique(mp["h_mm"]), np.unique(mp["Omega"])
    Z = np.full((len(hs), len(Oms)), np.nan); E = np.zeros_like(Z)
    for h, O, rt, fe in zip(mp["h_mm"], mp["Omega"], mp["ratio"], mp["f_escape"]):
        i, j = np.searchsorted(hs, h), np.searchsorted(Oms, O)
        Z[i, j] = rt if fe <= 0.5 else np.nan; E[i, j] = fe
    cmap = plt.get_cmap("coolwarm").copy(); cmap.set_bad("0.75")
    from matplotlib.colors import TwoSlopeNorm
    pc = ax.pcolormesh(Oms, hs, np.clip(Z, -0.5, 1.0), cmap=cmap, norm=TwoSlopeNorm(vmin=-0.5, vcenter=0.0, vmax=1.0),
                       shading="nearest")
    fig.colorbar(pc, ax=ax, label=r"$\omega_{ball}/\Omega$  (grey = escaped)")
    ax.contour(Oms, hs, np.nan_to_num(Z, nan=0.0), levels=[0.5], colors="k", linewidths=.8)
    if os.path.exists(os.path.join(D, "crit_h_mm.csv")):
        cr = load("crit_h_mm.csv")
        ax.plot(cr["Omega_unlock"], cr["h_mm"], "g--", lw=1.2, label=r"$\Omega_{unlock}$")
        ax.plot(cr["Omega_capture_estimate"], cr["h_mm"], "k:", lw=1.2, label="capture estimate")
        ax.legend(fontsize=7, loc="upper right")
    ax.set_xlim(Oms[0], Oms[-1])
    ax.set_xlabel(r"$\Omega$ [rad/s]"); ax.set_ylabel("gap h [mm]"); ax.set_title("Regime map (red = co, blue = counter)")

fig, ax = plt.subplots(figsize=(8, 5.5)); panel_map(ax, fig); save(fig, "map_Omega_h.png")

# ---------------- example trajectories ----------------
def traj_plot(name):
    t = load(f"traj_{name}.csv")
    Om = t["disc_angle"][-1] / t["t"][-1] if t["t"][-1] > 0 else 0
    N, R = int(P["N"]), P["R_m"]
    fig, ax = plt.subplots(1, 3, figsize=(16, 5))
    # top view, lab frame
    a = ax[0]
    a.plot(t["x"] * 100, t["y"] * 100, lw=.7, label="ball centre")
    a.plot(t["x"][0] * 100, t["y"][0] * 100, "go", label="start")
    th = np.linspace(0, 2 * math.pi, 200); a.plot(R * 100 * np.cos(th), R * 100 * np.sin(th), "k:", lw=.6)
    for k in range(N):
        ph = 2 * math.pi * k / N
        a.add_patch(plt.Circle((R * 100 * math.cos(ph), R * 100 * math.sin(ph)), P["mag_d"] * 50,
                               color="r" if k % 2 == 0 else "b", alpha=.3))
    a.set_aspect("equal"); a.set_xlabel("x [cm]"); a.set_ylabel("y [cm]"); a.set_title("top view, lab frame (magnets at t=0)")
    a.legend(fontsize=7)
    # rotating-frame view
    a = ax[1]
    ca, sa = np.cos(-t["disc_angle"]), np.sin(-t["disc_angle"])
    xr, yr = t["x"] * ca - t["y"] * sa, t["x"] * sa + t["y"] * ca
    a.plot(xr * 100, yr * 100, lw=.6, color="C1")
    for k in range(N):
        ph = 2 * math.pi * k / N
        a.add_patch(plt.Circle((R * 100 * math.cos(ph), R * 100 * math.sin(ph)), P["mag_d"] * 50,
                               color="r" if k % 2 == 0 else "b", alpha=.3))
    a.plot(R * 100 * np.cos(th), R * 100 * np.sin(th), "k:", lw=.6)
    a.set_aspect("equal"); a.set_xlabel("x' [cm]"); a.set_ylabel("y' [cm]"); a.set_title("path in the disc frame")
    # omega vs time
    a = ax[2]
    a.plot(t["t"], t["omega_inst"], lw=.5, alpha=.6, label=r"instantaneous $\dot\theta$")
    dtm = t["t"][1] - t["t"][0]; w = max(1, int(0.2 / dtm))
    run = np.convolve(t["omega_inst"], np.ones(w) / w, mode="valid")
    a.plot(t["t"][w // 2: w // 2 + len(run)], run, lw=1.5, label="0.2 s running mean")
    a.axhline(Om, color="k", ls="--", lw=.8, label=fr"$\Omega$={Om:g}")
    a.axhline(0, color="gray", lw=.5)
    a.set_xlabel("t [s]"); a.set_ylabel(r"$\omega_{ball}$ [rad/s]"); a.legend(fontsize=7); a.grid(alpha=.3)
    a.set_title(f"{name}-rotating example")
    save(fig, f"traj_{name}.png")

for nm in ("co", "counter"):
    if os.path.exists(os.path.join(D, f"traj_{nm}.csv")): traj_plot(nm)

# ---------------- summary ----------------
fig, ax = plt.subplots(3, 3, figsize=(18, 14))
panel_omega(ax[0, 0], s); panel_ratio(ax[0, 1], s)
if os.path.exists(os.path.join(D, "ramp.csv")): panel_ramp(ax[0, 2], load("ramp.csv"))
panel_map(ax[1, 0], fig)
for a, var in zip([ax[1, 1], ax[1, 2], ax[2, 0], ax[2, 1]], ["h_mm", "N", "a_mm", "tau_ms"]):
    if os.path.exists(os.path.join(D, f"crit_{var}.csv")): panel_crit(a, var, load(f"crit_{var}.csv"))
if os.path.exists(os.path.join(D, "sweep_tau_ms.csv")):
    panel_var_omega(ax[2, 2], "tau_ms", load("sweep_tau_ms.csv")); ax[2, 2].set_title(r"$\omega_{ball}$ vs lag time")
fig.suptitle(f"Magnetic carousel: N={int(P['N'])}, R_m={P['R_m']*100:g} cm, h={P['h']*1e3:g} mm, a={P['a']*1e3:g} mm, "
             fr"$\tau$={P['tau']*1e3:g} ms, $\mu$={P['mu']:g}", fontsize=14)
save(fig, "summary.png")
