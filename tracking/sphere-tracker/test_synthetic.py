#!/usr/bin/env python3
"""End-to-end headless test: generate a silver-sphere video (30 RPM target +
distractor sphere + occlusion), run the tracker locked onto the target, and
check the recovered speed is within 2%."""
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "test_output")
os.makedirs(OUT, exist_ok=True)
py = sys.executable
video = os.path.join(OUT, "synthetic_silver.mp4")

subprocess.run([py, os.path.join(HERE, "make_synthetic_video.py"), "--out", video, "--rpm", "30"],
               check=True, stdout=subprocess.DEVNULL)
truth = json.load(open(video.rsplit(".", 1)[0] + "_truth.json"))
ix, iy = truth["target_initial_xy"]

sys.path.insert(0, HERE)
import sphere_tracker as st  # noqa: E402

results = {}
cases = {
    # auto centre (circle fit), lock via --init (what --pick does interactively)
    "auto_center": ["--init", str(ix), str(iy), "--min-radius", "12", "--max-radius", "26",
                    "--output", os.path.join(OUT, "annotated.mp4")],
    # known centre given on the CLI
    "given_center": ["--init", str(ix), str(iy), "--min-radius", "12", "--max-radius", "26",
                     "--center", str(truth["center"][0]), str(truth["center"][1])],
}
ok_all = True
for name, extra in cases.items():
    args = st.parse_args([video, "--no-display", "--csv", os.path.join(OUT, f"{name}.csv"),
                          "--plot", os.path.join(OUT, f"{name}.png")] + extra)
    print(f"\n=== {name} ===")
    r = st.run(args)
    err = abs(r["mean_rpm"] - truth["true_rpm"]) / truth["true_rpm"] * 100
    ok = err <= 2.0
    ok_all = ok_all and bool(ok)
    results[name] = {"measured_rpm": round(r["mean_rpm"], 4), "true_rpm": truth["true_rpm"],
                     "error_pct": round(err, 3), "detected": f'{r["detected"]}/{r["frames"]}',
                     "center": [round(v, 2) for v in r["center"]], "orbit_px": round(r["radius"], 2),
                     "pass": bool(ok)}
print("\n" + json.dumps(results, indent=2))
json.dump(results, open(os.path.join(OUT, "results.json"), "w"), indent=2)
print("ALL PASS" if ok_all else "FAIL")
sys.exit(0 if ok_all else 1)
