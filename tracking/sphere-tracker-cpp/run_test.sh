#!/usr/bin/env bash
# Headless check on the synthetic silver-sphere video (30 RPM target + decoy + occlusion).
# Usage: ./run_test.sh [path/to/synthetic_silver.mp4]
# (generate the video with ../sphere-tracker/make_synthetic_video.py if you don't have it)
set -euo pipefail
cd "$(dirname "$0")"
VIDEO="${1:-../sphere-tracker/test_output/synthetic_silver.mp4}"
BIN=./build/sphere_tracker
[ -x "$BIN" ] || { cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j; }
mkdir -p test_output
out=$("$BIN" "$VIDEO" --no-display --init 593.3 237.4 --min-radius 12 --max-radius 26 \
      --csv test_output/auto_center.csv --plot test_output/auto_center.png --output test_output/annotated.mp4)
echo "$out"
rpm=$(echo "$out" | sed -n 's/.*= \([+-][0-9.]*\) RPM.*/\1/p' | head -1)
# decoy check: every detection must lie on the target orbit (~195 px), not the decoy's (~110 px)
awk -F, -v rpm="$rpm" 'NR>1 && $8==1 { d=sqrt(($3-407)^2+($4-295)^2); if(d<mn||mn=="")mn=d; if(d>mx)mx=d }
  END { err=(rpm-30)/30*100; if(err<0)err=-err;
        printf "measured %.3f RPM vs true 30.000 -> error %.3f%%; detection orbit radius %.1f..%.1f px\n", rpm, err, mn, mx;
        if (err<=2 && mn>170) { print "PASS"; exit 0 } else { print "FAIL"; exit 1 } }' test_output/auto_center.csv
