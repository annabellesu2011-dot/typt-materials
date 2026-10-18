// main.cpp - CLI and per-frame loop of the orbiting-sphere tracker (C++17 / OpenCV 4)
#include "tracker.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/videoio.hpp>
#ifdef HAVE_OPENCV_CSRT
#include <opencv2/tracking.hpp>
#endif

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <limits>
#include <string>

namespace {
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

const char* kUsage = R"(Usage: sphere_tracker <video|camera-index> [options]

Measures the angular speed of a sphere orbiting a plate centre and tracks its path.

Detection
  --mode silver|color|hough   detection mode (default silver = shiny metal ball)
  --min-radius N --max-radius N   sphere radius range in px (default 8 60)
  --hough-dp F --hough-param1 F --hough-param2 F   HoughCircles params (1.2 100 22)
  --sat-max N --value-min N --spec-value N   silver: saturation/brightness/specular (60 120 235)
  --min-score F               silver: min candidate score (0.35)
  --hsv-lower H S V --hsv-upper H S V   color mode range (default 0 0 150 / 180 60 255)
  --hough-fallback            color mode: fall back to HoughCircles
  --bg-sub                    MOG2 background subtraction (favour moving objects)
Target / centre
  --pick                      click the target sphere in the first frame (color: also picks HSV)
  --init X Y                  initial target position (non-interactive lock)
  --click-center              click the plate centre in the first frame
  --center X Y                plate centre in px (default: least-squares circle fit of path)
Tracking
  --gate PX                   max distance from prediction (60)
  --orbit-tol PX              max deviation from target orbit radius (25)
  --no-orbit-gate             disable orbit-following prediction / gating
  --no-kalman                 disable Kalman prediction
  --tracker none|csrt         add CSRT tracker (needs opencv_contrib 'tracking')
  --max-missed N              frames before global re-acquire on the orbit (15)
Timing / output
  --fps F                     override video FPS
  --smooth N                  Savitzky-Golay window in frames (15)
  --trail N                   overlay trail length (300)
  --output FILE               annotated video (.mp4 / .avi)
  --csv FILE                  CSV output (sphere_track.csv)
  --plot FILE                 plot PNG drawn with OpenCV (sphere_track.png, "" = none)
  --no-display                headless, no windows
  --max-frames N              stop after N frames
)";

[[noreturn]] void die(const std::string& m) {
    std::cerr << "Error: " << m << "\n\n" << kUsage;
    std::exit(2);
}

Options parseArgs(int argc, char** argv) {
    Options o;
    auto need = [&](int& i, int k) {
        if (i + k >= argc) die(std::string("missing value for ") + argv[i]);
    };
    auto num = [&](const char* s) {
        char* end = nullptr;
        double v = std::strtod(s, &end);
        if (end == s || *end) die(std::string("not a number: ") + s);
        return v;
    };
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help") { std::cout << kUsage; std::exit(0); }
        else if (a == "--mode") { need(i, 1); o.mode = argv[++i];
            if (o.mode != "silver" && o.mode != "color" && o.mode != "hough") die("bad --mode"); }
        else if (a == "--hsv-lower") { need(i, 3); o.hsvLower = {num(argv[i + 1]), num(argv[i + 2]), num(argv[i + 3])}; i += 3; }
        else if (a == "--hsv-upper") { need(i, 3); o.hsvUpper = {num(argv[i + 1]), num(argv[i + 2]), num(argv[i + 3])}; i += 3; }
        else if (a == "--hough-fallback") o.houghFallback = true;
        else if (a == "--min-radius") { need(i, 1); o.minRadius = int(num(argv[++i])); }
        else if (a == "--max-radius") { need(i, 1); o.maxRadius = int(num(argv[++i])); }
        else if (a == "--hough-dp") { need(i, 1); o.houghDp = num(argv[++i]); }
        else if (a == "--hough-param1") { need(i, 1); o.houghParam1 = num(argv[++i]); }
        else if (a == "--hough-param2") { need(i, 1); o.houghParam2 = num(argv[++i]); }
        else if (a == "--sat-max") { need(i, 1); o.satMax = int(num(argv[++i])); }
        else if (a == "--value-min") { need(i, 1); o.valueMin = int(num(argv[++i])); }
        else if (a == "--spec-value") { need(i, 1); o.specValue = int(num(argv[++i])); }
        else if (a == "--min-score") { need(i, 1); o.minScore = num(argv[++i]); }
        else if (a == "--bg-sub") o.bgSub = true;
        else if (a == "--pick") o.pick = true;
        else if (a == "--init") { need(i, 2); o.init = cv::Point2d(num(argv[i + 1]), num(argv[i + 2])); i += 2; }
        else if (a == "--center") { need(i, 2); o.center = cv::Point2d(num(argv[i + 1]), num(argv[i + 2])); i += 2; }
        else if (a == "--click-center") o.clickCenter = true;
        else if (a == "--gate") { need(i, 1); o.gate = num(argv[++i]); }
        else if (a == "--orbit-tol") { need(i, 1); o.orbitTol = num(argv[++i]); }
        else if (a == "--no-orbit-gate") o.noOrbitGate = true;
        else if (a == "--no-kalman") o.noKalman = true;
        else if (a == "--tracker") { need(i, 1); o.tracker = argv[++i];
            if (o.tracker != "none" && o.tracker != "csrt") die("bad --tracker"); }
        else if (a == "--max-missed") { need(i, 1); o.maxMissed = int(num(argv[++i])); }
        else if (a == "--fps") { need(i, 1); o.fps = num(argv[++i]); }
        else if (a == "--smooth") { need(i, 1); o.smooth = int(num(argv[++i])); }
        else if (a == "--trail") { need(i, 1); o.trail = int(num(argv[++i])); }
        else if (a == "--output") { need(i, 1); o.output = argv[++i]; }
        else if (a == "--csv") { need(i, 1); o.csv = argv[++i]; }
        else if (a == "--plot") { need(i, 1); o.plot = argv[++i]; }
        else if (a == "--no-display") o.noDisplay = true;
        else if (a == "--max-frames") { need(i, 1); o.maxFrames = int(num(argv[++i])); }
        else if (!a.empty() && a[0] == '-' && a.size() > 1 && !std::isdigit(static_cast<unsigned char>(a[1])))
            die("unknown option " + a);
        else if (o.source.empty()) o.source = a;
        else die("unexpected argument " + a);
    }
    if (o.source.empty()) die("no source given");
    return o;
}

// ---- interactive click helper
struct ClickState { bool has = false; cv::Point p; };
void onMouse(int ev, int x, int y, int, void* ud) {
    if (ev == cv::EVENT_LBUTTONDOWN) { auto* s = static_cast<ClickState*>(ud); s->has = true; s->p = {x, y}; }
}
std::optional<cv::Point> clickPoint(const cv::Mat& frame, const std::string& title) {
    ClickState st;
    cv::namedWindow(title);
    cv::setMouseCallback(title, onMouse, &st);
    for (;;) {
        cv::Mat d = frame.clone();
        cv::putText(d, title + "  (click, then any key; ESC = skip)", {10, 25},
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, {0, 255, 255}, 2);
        if (st.has) cv::drawMarker(d, st.p, {0, 0, 255}, cv::MARKER_CROSS, 20, 2);
        cv::imshow(title, d);
        int k = cv::waitKey(20);
        if (k == 27) { st.has = false; break; }
        if (k >= 0 && st.has) break;
    }
    cv::destroyWindow(title);
    if (!st.has) return std::nullopt;
    return st.p;
}

cv::KalmanFilter makeKalman(double x, double y, double dt) {
    cv::KalmanFilter kf(4, 2, 0, CV_32F);
    kf.transitionMatrix = (cv::Mat_<float>(4, 4) << 1, 0, dt, 0, 0, 1, 0, dt, 0, 0, 1, 0, 0, 0, 0, 1);
    kf.measurementMatrix = (cv::Mat_<float>(2, 4) << 1, 0, 0, 0, 0, 1, 0, 0);
    kf.processNoiseCov = cv::Mat::eye(4, 4, CV_32F) * 5.0f;
    kf.measurementNoiseCov = cv::Mat::eye(2, 2, CV_32F) * 2.0f;
    kf.errorCovPost = cv::Mat::eye(4, 4, CV_32F) * 10.0f;
    kf.statePost = (cv::Mat_<float>(4, 1) << float(x), float(y), 0, 0);
    return kf;
}

std::string fmt2(const char* f, double a, double b) {
    char buf[128];
    std::snprintf(buf, sizeof(buf), f, a, b);
    return buf;
}
}  // namespace

int main(int argc, char** argv) {
    Options opt = parseArgs(argc, argv);
    const bool live = !opt.source.empty() &&
                      opt.source.find_first_not_of("0123456789") == std::string::npos;
    cv::VideoCapture cap;
    if (live) cap.open(std::stoi(opt.source)); else cap.open(opt.source);
    if (!cap.isOpened()) { std::cerr << "Cannot open source " << opt.source << '\n'; return 1; }
    double fps = opt.fps > 0 ? opt.fps : cap.get(cv::CAP_PROP_FPS);
    if (!(fps > 1 && fps <= 1000)) fps = 30.0;
    cv::Mat frame;
    if (!cap.read(frame) || frame.empty()) { std::cerr << "Cannot read first frame\n"; return 1; }
    const cv::Size size = frame.size();
    const bool display = !opt.noDisplay;

    Detector det(opt);
    if (display && opt.pick) {
        if (auto p = clickPoint(frame, "Click the target sphere")) {
            opt.init = cv::Point2d(p->x, p->y);
            if (opt.mode == "color") {
                hsvRangeFromPoint(frame, p->x, p->y, opt.hsvLower, opt.hsvUpper);
                det.setHsv(opt.hsvLower, opt.hsvUpper);
                std::printf("Picked HSV lower=(%g,%g,%g) upper=(%g,%g,%g)\n", opt.hsvLower[0], opt.hsvLower[1],
                            opt.hsvLower[2], opt.hsvUpper[0], opt.hsvUpper[1], opt.hsvUpper[2]);
            }
        }
    }
    if (display && opt.clickCenter)
        if (auto p = clickPoint(frame, "Click the plate centre")) opt.center = cv::Point2d(p->x, p->y);
    if (!display && (opt.pick || opt.clickCenter))
        std::cout << "Note: --pick/--click-center ignored with --no-display (use --init / --center)\n";

    cv::VideoWriter writer;
    if (!opt.output.empty()) {
        const bool mp4 = opt.output.size() >= 4 && opt.output.substr(opt.output.size() - 4) == ".mp4";
        writer.open(opt.output, mp4 ? cv::VideoWriter::fourcc('m', 'p', '4', 'v')
                                    : cv::VideoWriter::fourcc('X', 'V', 'I', 'D'), fps, size);
        if (!writer.isOpened()) std::cerr << "Warning: cannot open output video " << opt.output << '\n';
    }

#ifdef HAVE_OPENCV_CSRT
    cv::Ptr<cv::Tracker> csrt;
    if (opt.tracker == "csrt") csrt = cv::TrackerCSRT::create();
#else
    if (opt.tracker == "csrt")
        std::cout << "CSRT not available (OpenCV built without opencv_contrib 'tracking'); continuing without it\n";
#endif
    bool trackerOk = false;

    const double dt = 1.0 / fps;
    std::optional<cv::KalmanFilter> kf;
    std::optional<cv::Point2d> last = opt.init;
    bool locked = opt.init.has_value();
    int missed = 0;
    TrackData data;
    std::deque<std::optional<cv::Point>> trail;
    const auto t0 = std::chrono::steady_clock::now();

    for (int idx = 0; !frame.empty(); ++idx) {
        const double t = live ? std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count()
                              : idx / fps;
        std::vector<Candidate> cands = det.candidates(frame);

        // ---- prediction: last position -> Kalman -> CSRT -> orbit model
        std::optional<cv::Point2d> pred = last;
        if (kf) { cv::Mat p = kf->predict(); pred = cv::Point2d(p.at<float>(0), p.at<float>(1)); }
#ifdef HAVE_OPENCV_CSRT
        if (csrt && trackerOk) {
            cv::Rect bb;
            if (csrt->update(frame, bb)) pred = cv::Point2d(bb.x + bb.width / 2.0, bb.y + bb.height / 2.0);
        }
#endif
        std::optional<Orbit> orbit;
        if (!opt.noOrbitGate) orbit = orbitModel(data.xs, data.ys, data.ts, opt.center);
        if (orbit) pred = predictOnOrbit(*orbit, t);

        const double gate = opt.gate * (1.0 + 0.08 * missed);
        auto sel = selectCandidate(cands, pred, gate, locked, orbit, opt.orbitTol);
        if (!sel && locked && missed >= opt.maxMissed && last)
            sel = selectCandidate(cands, pred ? pred : last, std::max(size.width, size.height), true,
                                  orbit, opt.orbitTol);

        data.frames.push_back(idx);
        data.ts.push_back(t);
        if (sel) {
            missed = 0;
            locked = true;
            if (!kf && !opt.noKalman) kf = makeKalman(sel->x, sel->y, dt);
            else if (kf) kf->correct((cv::Mat_<float>(2, 1) << float(sel->x), float(sel->y)));
#ifdef HAVE_OPENCV_CSRT
            if (csrt && !trackerOk) {
                int s = int(2.4 * sel->r);
                csrt->init(frame, cv::Rect(int(sel->x - s / 2), int(sel->y - s / 2), s, s));
                trackerOk = true;
            }
#endif
            last = cv::Point2d(sel->x, sel->y);
            data.xs.push_back(sel->x);
            data.ys.push_back(sel->y);
            trail.emplace_back(cv::Point(int(std::lround(sel->x)), int(std::lround(sel->y))));
        } else {
            ++missed;
            data.xs.push_back(kNaN);
            data.ys.push_back(kNaN);
            trail.emplace_back(std::nullopt);  // break the trail at missed detections
        }
        while (static_cast<int>(trail.size()) > opt.trail) trail.pop_front();
        (void)trackerOk;

        // ---- overlay
        if (display || writer.isOpened()) {
            cv::Mat vis = frame.clone();
            std::optional<cv::Point2d> cLive = opt.center;
            const size_t st = data.xs.size() > 600 ? data.xs.size() - 600 : 0;
            std::vector<double> gx, gy, gt;
            for (size_t i = st; i < data.xs.size(); ++i)
                if (!std::isnan(data.xs[i])) { gx.push_back(data.xs[i]); gy.push_back(data.ys[i]); gt.push_back(data.ts[i]); }
            if (!cLive && gx.size() >= 15) { Circle c = fitCircle(gx, gy); cLive = cv::Point2d(c.cx, c.cy); }
            std::optional<double> omegaLive;
            if (cLive && gx.size() >= 5) {
                const size_t k = std::min<size_t>(std::min(int(fps), 30), gx.size());
                std::vector<double> th, tk;
                for (size_t i = gx.size() - k; i < gx.size(); ++i) {
                    th.push_back(angleAbout(gx[i], gy[i], cLive->x, cLive->y));
                    tk.push_back(gt[i]);
                }
                th = unwrap(th);
                if (th.size() >= 3 && tk.back() > tk.front()) omegaLive = linearSlope(tk, th);
            }
            std::vector<cv::Point> seg;
            auto flush = [&] {
                if (seg.size() > 1) cv::polylines(vis, seg, false, {0, 255, 255}, 2, cv::LINE_AA);
                seg.clear();
            };
            for (const auto& p : trail) { if (p) seg.push_back(*p); else flush(); }
            flush();
            if (cLive) {
                cv::Point cc(int(std::lround(cLive->x)), int(std::lround(cLive->y)));
                cv::drawMarker(vis, cc, {255, 0, 255}, cv::MARKER_CROSS, 24, 2);
                if (sel) cv::line(vis, cc, cv::Point(int(sel->x), int(sel->y)), {255, 0, 255}, 1);
            }
            for (const auto& c : cands) cv::circle(vis, {int(c.x), int(c.y)}, int(c.r), {128, 128, 128}, 1);
            if (sel) cv::circle(vis, {int(sel->x), int(sel->y)}, int(sel->r), {0, 255, 0}, 2);
            else if (pred) cv::circle(vis, {int(pred->x), int(pred->y)}, 6, {0, 0, 255}, 2);
            std::string txt = omegaLive ? fmt2("omega: %+.3f rad/s  %+.2f RPM", *omegaLive, *omegaLive * 60 / (2 * CV_PI))
                                        : "omega: --";
            cv::putText(vis, txt, {10, 30}, cv::FONT_HERSHEY_SIMPLEX, 0.8, {0, 0, 0}, 4);
            cv::putText(vis, txt, {10, 30}, cv::FONT_HERSHEY_SIMPLEX, 0.8, {0, 255, 0}, 2);
            cv::putText(vis, fmt2("t=%6.2fs  frame %.0f", t, idx) + (sel ? "" : "  MISSED"), {10, 60},
                        cv::FONT_HERSHEY_SIMPLEX, 0.6, {255, 255, 255}, 2);
            if (writer.isOpened()) writer.write(vis);
            if (display) {
                cv::imshow("sphere tracker (q to quit)", vis);
                int k = cv::waitKey(1);
                if (k == 'q' || k == 27) break;
            }
        }
        if (opt.maxFrames && idx + 1 >= opt.maxFrames) break;
        if (!cap.read(frame)) break;
    }
    cap.release();
    writer.release();
    if (display) cv::destroyAllWindows();

    Result r = analyse(opt, data, size);
    return r.detected >= 5 ? 0 : 1;
}
