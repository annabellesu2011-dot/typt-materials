// tracker.cpp - detection, gating and geometry
#include "tracker.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace {
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
}

// ============================================================== geometry
Circle fitCircle(const std::vector<double>& xs, const std::vector<double>& ys) {
    // Solve [x y 1] * [a b c]^T = x^2 + y^2 in the least-squares sense.
    const int n = static_cast<int>(xs.size());
    cv::Mat A(n, 3, CV_64F), b(n, 1, CV_64F), sol;
    for (int i = 0; i < n; ++i) {
        A.at<double>(i, 0) = xs[i];
        A.at<double>(i, 1) = ys[i];
        A.at<double>(i, 2) = 1.0;
        b.at<double>(i, 0) = xs[i] * xs[i] + ys[i] * ys[i];
    }
    cv::solve(A, b, sol, cv::DECOMP_SVD);
    double cx = sol.at<double>(0) / 2.0, cy = sol.at<double>(1) / 2.0;
    double r = std::sqrt(std::max(sol.at<double>(2) + cx * cx + cy * cy, 0.0));
    return {cx, cy, r};
}

double angleAbout(double x, double y, double cx, double cy) {
    return std::atan2(-(y - cy), x - cx);  // image y points down -> flip for CCW+
}

std::vector<double> unwrap(const std::vector<double>& a) {
    std::vector<double> out(a);
    double offset = 0;
    for (size_t i = 1; i < a.size(); ++i) {
        double d = a[i] - a[i - 1];
        if (d > CV_PI) offset -= 2 * CV_PI * std::round(d / (2 * CV_PI));
        else if (d < -CV_PI) offset -= 2 * CV_PI * std::round(d / (2 * CV_PI));
        out[i] = a[i] + offset;
    }
    return out;
}

double linearSlope(const std::vector<double>& t, const std::vector<double>& y) {
    const size_t n = t.size();
    if (n < 2) return 0;
    double mt = std::accumulate(t.begin(), t.end(), 0.0) / n;
    double my = std::accumulate(y.begin(), y.end(), 0.0) / n;
    double num = 0, den = 0;
    for (size_t i = 0; i < n; ++i) {
        num += (t[i] - mt) * (y[i] - my);
        den += (t[i] - mt) * (t[i] - mt);
    }
    return den > 0 ? num / den : 0;
}

std::optional<Orbit> orbitModel(const std::vector<double>& xs, const std::vector<double>& ys,
                                const std::vector<double>& ts,
                                const std::optional<cv::Point2d>& center, int minPts, int k) {
    const size_t start = xs.size() > 400 ? xs.size() - 400 : 0;
    std::vector<double> gx, gy, gt;
    for (size_t i = start; i < xs.size(); ++i)
        if (!std::isnan(xs[i])) { gx.push_back(xs[i]); gy.push_back(ys[i]); gt.push_back(ts[i]); }
    if (static_cast<int>(gx.size()) < minPts) return std::nullopt;
    double cx, cy;
    if (center) { cx = center->x; cy = center->y; }
    else { Circle c = fitCircle(gx, gy); cx = c.cx; cy = c.cy; }
    std::vector<double> th(gx.size()), rad(gx.size());
    for (size_t i = 0; i < gx.size(); ++i) {
        th[i] = angleAbout(gx[i], gy[i], cx, cy);
        rad[i] = std::hypot(gx[i] - cx, gy[i] - cy);
    }
    th = unwrap(th);
    // a fitted centre is only trustworthy once the path spans some angle
    if (!center && std::abs(th.back() - th.front()) < 1.0) return std::nullopt;
    const size_t kk = std::min<size_t>(k, th.size());
    std::vector<double> tk(gt.end() - kk, gt.end()), thk(th.end() - kk, th.end());
    double omega = (tk.back() > tk.front()) ? linearSlope(tk, thk) : 0.0;
    const size_t m = std::min<size_t>(60, rad.size());
    std::vector<double> rr(rad.end() - m, rad.end());
    std::nth_element(rr.begin(), rr.begin() + rr.size() / 2, rr.end());
    return Orbit{cx, cy, rr[rr.size() / 2], th.back(), omega, gt.back()};
}

cv::Point2d predictOnOrbit(const Orbit& o, double t) {
    double a = o.thetaLast + o.omega * (t - o.tLast);
    return {o.cx + o.R * std::cos(a), o.cy - o.R * std::sin(a)};
}

// ============================================================== detection
Detector::Detector(const Options& o) : opt_(o) {
    if (o.bgSub) bg_ = cv::createBackgroundSubtractorMOG2(300, 25, false);
}

std::vector<cv::Vec3f> Detector::hough(const cv::Mat& gray) const {
    cv::Mat g;
    cv::medianBlur(gray, g, 5);
    std::vector<cv::Vec3f> circles;
    cv::HoughCircles(g, circles, cv::HOUGH_GRADIENT, opt_.houghDp,
                     std::max(opt_.minRadius * 1.5, 5.0), opt_.houghParam1, opt_.houghParam2,
                     opt_.minRadius, opt_.maxRadius);
    return circles;
}

std::vector<cv::Vec3f> Detector::blobs(const cv::Mat& maskIn) const {
    cv::Mat mask;
    cv::morphologyEx(maskIn, mask, cv::MORPH_OPEN, cv::Mat::ones(3, 3, CV_8U));
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, cv::Mat::ones(5, 5, CV_8U));
    std::vector<std::vector<cv::Point>> cnts;
    cv::findContours(mask, cnts, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    std::vector<cv::Vec3f> out;
    for (const auto& c : cnts) {
        double area = cv::contourArea(c);
        if (area < 10) continue;
        cv::Point2f ctr; float r;
        cv::minEnclosingCircle(c, ctr, r);
        if (r < opt_.minRadius * 0.6 || r > opt_.maxRadius * 1.3) continue;
        if (area / (CV_PI * r * r + 1e-9) < 0.45) continue;  // circularity
        cv::Moments m = cv::moments(c);
        if (m.m00 > 0) ctr = {float(m.m10 / m.m00), float(m.m01 / m.m00)};
        out.emplace_back(ctr.x, ctr.y, r);
    }
    return out;
}

double Detector::silverScore(const cv::Mat& hsv, const cv::Mat& fg, double x, double y,
                             double r) const {
    r = std::max(r, 2.0);
    const int x0 = std::max(int(x - r), 0), x1 = std::min(int(x + r + 1), hsv.cols);
    const int y0 = std::max(int(y - r), 0), y1 = std::min(int(y + r + 1), hsv.rows);
    if (x1 - x0 < 2 || y1 - y0 < 2) return 0;
    const double rr = (r * 0.9) * (r * 0.9);
    double sumS = 0, sumV = 0, sumV2 = 0, fgSum = 0;
    int n = 0, spec = 0;
    for (int yy = y0; yy < y1; ++yy) {
        const cv::Vec3b* row = hsv.ptr<cv::Vec3b>(yy);
        for (int xx = x0; xx < x1; ++xx) {
            if ((xx - x) * (xx - x) + (yy - y) * (yy - y) > rr) continue;
            const double S = row[xx][1], V = row[xx][2];
            sumS += S; sumV += V; sumV2 += V * V; ++n;
            if (V >= opt_.specValue && S <= opt_.satMax) ++spec;
            if (!fg.empty() && fg.at<uchar>(yy, xx) > 0) fgSum += 1;
        }
    }
    if (n < 5) return 0;
    const double meanS = sumS / n, meanV = sumV / n;
    const double stdV = std::sqrt(std::max(sumV2 / n - meanV * meanV, 0.0));
    const double lowSat = 1.0 - std::min(meanS / std::max(opt_.satMax, 1), 1.0);
    const double highlight = std::min((double(spec) / n) / 0.02, 1.0);  // >=2% specular pixels
    const double contrast = std::min(stdV / 40.0, 1.0);                 // metal shading
    double score = 0.35 * lowSat + 0.35 * highlight + 0.30 * contrast;
    if (!fg.empty()) score = 0.7 * score + 0.3 * (fgSum / n);
    return score;
}

double Detector::colorScore(const cv::Mat& mask, double x, double y, double r) const {
    const int x0 = std::max(int(x - r), 0), x1 = std::min(int(x + r + 1), mask.cols);
    const int y0 = std::max(int(y - r), 0), y1 = std::min(int(y + r + 1), mask.rows);
    if (x1 <= x0 || y1 <= y0) return 0;
    return cv::countNonZero(mask(cv::Range(y0, y1), cv::Range(x0, x1))) /
           double((x1 - x0) * (y1 - y0));
}

std::vector<Candidate> Detector::candidates(const cv::Mat& bgr) {
    cv::Mat hsv, gray, fg;
    cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
    cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);
    if (bg_) bg_->apply(bgr, fg);
    std::vector<Candidate> out;
    if (opt_.mode == "silver") {
        cv::Mat mask;
        cv::inRange(hsv, cv::Scalar(0, 0, opt_.valueMin), cv::Scalar(180, opt_.satMax, 255), mask);
        auto raw = hough(gray);
        auto b = blobs(mask);
        raw.insert(raw.end(), b.begin(), b.end());
        for (const auto& c : raw) {
            double s = silverScore(hsv, fg, c[0], c[1], c[2]);
            if (s >= opt_.minScore) out.push_back({c[0], c[1], c[2], s});
        }
    } else if (opt_.mode == "color") {
        cv::Mat mask;
        cv::inRange(hsv, opt_.hsvLower, opt_.hsvUpper, mask);
        if (!fg.empty()) cv::bitwise_and(mask, fg, mask);
        for (const auto& c : blobs(mask)) out.push_back({c[0], c[1], c[2], 1.0});
        if (out.empty() && opt_.houghFallback)
            for (const auto& c : hough(gray))
                out.push_back({c[0], c[1], c[2], colorScore(mask, c[0], c[1], c[2])});
    } else {  // hough
        for (const auto& c : hough(gray)) out.push_back({c[0], c[1], c[2], 1.0});
    }
    return out;
}

std::optional<Candidate> selectCandidate(const std::vector<Candidate>& candsIn,
                                         const std::optional<cv::Point2d>& pred, double gate,
                                         bool locked, const std::optional<Orbit>& orbit,
                                         double orbitTol) {
    std::vector<Candidate> cands;
    for (const auto& c : candsIn) {
        // reject spheres on a different orbit (e.g. a decoy closer to the centre)
        if (orbit && orbitTol > 0 &&
            std::abs(std::hypot(c.x - orbit->cx, c.y - orbit->cy) - orbit->R) > orbitTol)
            continue;
        cands.push_back(c);
    }
    if (cands.empty()) return std::nullopt;
    if (locked && pred) {
        std::optional<Candidate> best;
        double bestCost = 1e18;
        for (const auto& c : cands) {
            double d = std::hypot(c.x - pred->x, c.y - pred->y);
            if (d > gate) continue;
            double cost = d / gate + 0.3 * (1.0 - c.score);
            if (cost < bestCost) { bestCost = cost; best = c; }
        }
        return best;
    }
    return *std::max_element(cands.begin(), cands.end(),
                             [](const Candidate& a, const Candidate& b) { return a.score < b.score; });
}

void hsvRangeFromPoint(const cv::Mat& bgr, int x, int y, cv::Scalar& lo, cv::Scalar& hi) {
    const int half = 4;
    cv::Rect roi(std::max(x - half, 0), std::max(y - half, 0), 2 * half + 1, 2 * half + 1);
    roi &= cv::Rect(0, 0, bgr.cols, bgr.rows);
    cv::Mat hsv;
    cv::cvtColor(bgr(roi), hsv, cv::COLOR_BGR2HSV);
    std::vector<cv::Mat> ch;
    cv::split(hsv.reshape(1, 1).reshape(3, 1), ch);
    double med[3];
    for (int k = 0; k < 3; ++k) {
        std::vector<uchar> v(ch[k].begin<uchar>(), ch[k].end<uchar>());
        std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
        med[k] = v[v.size() / 2];
    }
    lo = cv::Scalar(std::max(med[0] - 10, 0.0), std::max(med[1] - 60, 0.0), std::max(med[2] - 60, 0.0));
    hi = cv::Scalar(std::min(med[0] + 10, 180.0), std::min(med[1] + 60, 255.0), std::min(med[2] + 60, 255.0));
}
