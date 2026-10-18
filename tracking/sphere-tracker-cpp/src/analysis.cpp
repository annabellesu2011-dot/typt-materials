// analysis.cpp - post-processing: centre, unwrapped angle, smoothed omega, CSV and plot
#include "tracker.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace {

// Linear interpolation of (xp, fp) at x (xp increasing); clamps at the ends like np.interp.
std::vector<double> interp(const std::vector<double>& x, const std::vector<double>& xp,
                           const std::vector<double>& fp) {
    std::vector<double> out(x.size());
    for (size_t i = 0; i < x.size(); ++i) {
        if (x[i] <= xp.front()) { out[i] = fp.front(); continue; }
        if (x[i] >= xp.back()) { out[i] = fp.back(); continue; }
        size_t j = std::upper_bound(xp.begin(), xp.end(), x[i]) - xp.begin();
        double w = (x[i] - xp[j - 1]) / (xp[j] - xp[j - 1]);
        out[i] = fp[j - 1] + w * (fp[j] - fp[j - 1]);
    }
    return out;
}

// Savitzky-Golay style smoothing: local quadratic least-squares fit over an odd window,
// the window is shifted at the edges (equivalent to scipy's mode="interp").
std::vector<double> savgol(const std::vector<double>& y, int window) {
    const int n = static_cast<int>(y.size());
    if (window % 2 == 0) ++window;
    if (n % 2 == 0) window = std::min(window, n - 1); else window = std::min(window, n);
    if (window < 5) return y;
    const int half = window / 2;
    std::vector<double> out(n);
    for (int i = 0; i < n; ++i) {
        int s = std::clamp(i - half, 0, n - window);
        cv::Mat A(window, 3, CV_64F), b(window, 1, CV_64F), c;
        for (int k = 0; k < window; ++k) {
            double u = double(s + k - i);
            A.at<double>(k, 0) = 1; A.at<double>(k, 1) = u; A.at<double>(k, 2) = u * u;
            b.at<double>(k) = y[s + k];
        }
        cv::solve(A, b, c, cv::DECOMP_QR);
        out[i] = c.at<double>(0);  // value of the local fit at u = 0
    }
    return out;
}

// Same as numpy.gradient (2nd-order interior, non-uniform spacing, 1st-order edges).
std::vector<double> gradient(const std::vector<double>& f, const std::vector<double>& t) {
    const size_t n = f.size();
    std::vector<double> g(n, 0.0);
    if (n < 2) return g;
    g[0] = (f[1] - f[0]) / (t[1] - t[0]);
    g[n - 1] = (f[n - 1] - f[n - 2]) / (t[n - 1] - t[n - 2]);
    for (size_t i = 1; i + 1 < n; ++i) {
        double hs = t[i] - t[i - 1], hd = t[i + 1] - t[i];
        g[i] = (hs * hs * f[i + 1] + (hd * hd - hs * hs) * f[i] - hd * hd * f[i - 1]) /
               (hs * hd * (hd + hs));
    }
    return g;
}

std::string fmt(const char* f, double v) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), f, v);
    return buf;
}

void drawPlot(const std::string& path, const TrackData& d, const std::vector<double>& rpm,
              const std::vector<bool>& good, const Result& r, cv::Size frameSize) {
    const int W = 1300, H = 560, pad = 55;
    cv::Mat img(H, W, CV_8UC3, cv::Scalar(255, 255, 255));
    const auto font = cv::FONT_HERSHEY_SIMPLEX;
    const cv::Scalar black(0, 0, 0), gray(170, 170, 170), light(230, 230, 230);

    // ---- left panel: path in image coordinates (aspect preserved)
    cv::Rect L(pad, pad, 520, H - 2 * pad);
    double s = std::min(L.width / double(frameSize.width), L.height / double(frameSize.height));
    cv::Rect P(L.x, L.y, int(frameSize.width * s), int(frameSize.height * s));
    cv::rectangle(img, P, black, 1);
    auto toP = [&](double x, double y) { return cv::Point(int(P.x + x * s), int(P.y + y * s)); };
    cv::circle(img, toP(r.cx, r.cy), int(r.R * s), gray, 1, cv::LINE_AA);
    const double tmax = d.ts.empty() ? 1 : std::max(d.ts.back(), 1e-9);
    for (size_t i = 0; i < d.xs.size(); ++i) {
        if (!good[i]) continue;
        cv::Mat c(1, 1, CV_8UC1, cv::Scalar(int(255 * d.ts[i] / tmax))), cc;
        cv::applyColorMap(c, cc, cv::COLORMAP_VIRIDIS);
        cv::Vec3b col = cc.at<cv::Vec3b>(0, 0);
        cv::circle(img, toP(d.xs[i], d.ys[i]), 2, cv::Scalar(col[0], col[1], col[2]), -1, cv::LINE_AA);
    }
    cv::drawMarker(img, toP(r.cx, r.cy), cv::Scalar(255, 0, 255), cv::MARKER_CROSS, 18, 2);
    cv::putText(img, "Tracked path (image px), colour = time", {P.x, P.y - 12}, font, 0.5, black, 1, cv::LINE_AA);
    cv::putText(img, "0", {P.x - 5, P.y + P.height + 18}, font, 0.4, black, 1, cv::LINE_AA);
    cv::putText(img, std::to_string(frameSize.width), {P.x + P.width - 20, P.y + P.height + 18}, font, 0.4, black, 1, cv::LINE_AA);
    cv::putText(img, std::to_string(frameSize.height), {P.x - 35, P.y + P.height}, font, 0.4, black, 1, cv::LINE_AA);

    // ---- right panel: RPM vs time
    cv::Rect R(640, pad, W - 640 - pad, H - 2 * pad);
    double lo = r.meanRpm, hi = r.meanRpm;
    for (double v : rpm) { lo = std::min(lo, v); hi = std::max(hi, v); }
    double span = std::max(hi - lo, 1e-3);
    lo -= 0.1 * span; hi += 0.1 * span;
    auto toR = [&](double t, double v) {
        return cv::Point(int(R.x + R.width * t / tmax), int(R.y + R.height * (1 - (v - lo) / (hi - lo))));
    };
    for (int k = 0; k <= 5; ++k) {  // grid + ticks
        double v = lo + (hi - lo) * k / 5.0, t = tmax * k / 5.0;
        cv::line(img, toR(0, v), toR(tmax, v), light, 1);
        cv::line(img, toR(t, lo), toR(t, hi), light, 1);
        cv::putText(img, fmt("%.2f", v), {R.x - 52, toR(0, v).y + 4}, font, 0.4, black, 1, cv::LINE_AA);
        cv::putText(img, fmt("%.1f", t), {toR(t, lo).x - 10, R.y + R.height + 18}, font, 0.4, black, 1, cv::LINE_AA);
    }
    cv::rectangle(img, R, black, 1);
    std::vector<cv::Point> pts;
    for (size_t i = 0; i < rpm.size(); ++i) pts.push_back(toR(d.ts[i], rpm[i]));
    if (pts.size() > 1) cv::polylines(img, pts, false, cv::Scalar(200, 60, 0), 1, cv::LINE_AA);
    for (size_t i = 0; i < rpm.size(); ++i)
        if (!good[i]) cv::drawMarker(img, pts[i], cv::Scalar(0, 0, 220), cv::MARKER_TILTED_CROSS, 6, 1);
    cv::line(img, toR(0, r.meanRpm), toR(tmax, r.meanRpm), cv::Scalar(0, 0, 220), 1, cv::LINE_AA);
    cv::putText(img, "Angular speed [RPM] vs time [s]", {R.x, R.y - 30}, font, 0.55, black, 1, cv::LINE_AA);
    cv::putText(img, "blue = smoothed, red = mean " + fmt("%.3f", r.meanRpm) +
                " RPM, x = interpolated (missed frame)", {R.x, R.y - 10}, font, 0.42, black, 1, cv::LINE_AA);
    cv::imwrite(path, img);
}

}  // namespace

Result analyse(const Options& opt, const TrackData& d, cv::Size frameSize) {
    Result r;
    const size_t n = d.xs.size();
    r.total = static_cast<int>(n);
    std::vector<bool> good(n);
    std::vector<double> gx, gy, gt;
    for (size_t i = 0; i < n; ++i) {
        good[i] = !std::isnan(d.xs[i]);
        if (good[i]) { gx.push_back(d.xs[i]); gy.push_back(d.ys[i]); gt.push_back(d.ts[i]); }
    }
    r.detected = static_cast<int>(gx.size());
    if (r.detected < 5) {
        std::cerr << "Only " << r.detected << " detections - check detection parameters\n";
        return r;
    }
    if (!opt.center) {
        Circle c = fitCircle(gx, gy);
        r.cx = c.cx; r.cy = c.cy; r.R = c.r;
        std::printf("Auto centre (circle fit): (%.1f, %.1f), orbit radius %.1fpx\n", r.cx, r.cy, r.R);
    } else {
        r.cx = opt.center->x; r.cy = opt.center->y;
        double s = 0;
        for (size_t i = 0; i < gx.size(); ++i) s += std::hypot(gx[i] - r.cx, gy[i] - r.cy);
        r.R = s / gx.size();
        std::printf("Centre: (%.1f, %.1f), mean orbit radius %.1fpx\n", r.cx, r.cy, r.R);
    }
    std::vector<double> thGood(gx.size());
    for (size_t i = 0; i < gx.size(); ++i) thGood[i] = angleAbout(gx[i], gy[i], r.cx, r.cy);
    thGood = unwrap(thGood);
    std::vector<double> thAll = interp(d.ts, gt, thGood);  // fill missed frames
    std::vector<double> thS = savgol(thAll, opt.smooth);
    std::vector<double> omega = gradient(thS, d.ts);
    std::vector<double> rpm(n);
    for (size_t i = 0; i < n; ++i) rpm[i] = omega[i] * 60.0 / (2 * CV_PI);
    r.omega = linearSlope(gt, thGood);
    r.meanRpm = r.omega * 60.0 / (2 * CV_PI);
    std::vector<double> rg;
    for (size_t i = 0; i < n; ++i) if (good[i]) rg.push_back(rpm[i]);
    std::nth_element(rg.begin(), rg.begin() + rg.size() / 2, rg.end());
    std::printf("Detections: %d/%d frames (%.1f%%)\n", r.detected, r.total, 100.0 * r.detected / r.total);
    std::printf("Mean angular speed (linear fit of theta(t)): %+.4f rad/s = %+.3f RPM\n", r.omega, r.meanRpm);
    std::printf("Median smoothed omega: %+.3f RPM  (sign: + = counter-clockwise on screen)\n", rg[rg.size() / 2]);

    std::ofstream f(opt.csv);
    f << "frame,t,x,y,theta_unwrapped,omega_rad_s,rpm,detected\n" << std::fixed;
    for (size_t i = 0; i < n; ++i) {
        f << d.frames[i] << ',' << std::setprecision(4) << d.ts[i] << ',';
        if (good[i]) f << std::setprecision(2) << d.xs[i] << ',' << d.ys[i] << ',';
        else f << ",,";
        f << std::setprecision(5) << thAll[i] << ',' << omega[i] << ',' << std::setprecision(4)
          << rpm[i] << ',' << int(good[i]) << '\n';
    }
    std::cout << "CSV  -> " << opt.csv << '\n';
    if (!opt.plot.empty()) {
        drawPlot(opt.plot, d, rpm, good, r, frameSize);
        std::cout << "Plot -> " << opt.plot << '\n';
    }
    return r;
}
