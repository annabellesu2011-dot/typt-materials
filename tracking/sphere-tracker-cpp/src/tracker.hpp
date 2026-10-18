// tracker.hpp - detection, prediction and geometry for the orbiting-sphere tracker
#pragma once
#include <opencv2/core.hpp>
#include <opencv2/video/background_segm.hpp>
#include <optional>
#include <string>
#include <vector>

struct Options {
    std::string source;
    std::string mode = "silver";  // silver | color | hough
    cv::Scalar hsvLower{0, 0, 150}, hsvUpper{180, 60, 255};
    bool houghFallback = false;
    int minRadius = 8, maxRadius = 60;
    double houghDp = 1.2, houghParam1 = 100, houghParam2 = 22;
    int satMax = 60, valueMin = 120, specValue = 235;
    double minScore = 0.35;
    bool bgSub = false;
    bool pick = false, clickCenter = false;
    std::optional<cv::Point2d> init, center;
    double gate = 60.0, orbitTol = 25.0;
    bool noOrbitGate = false, noKalman = false;
    std::string tracker = "none";  // none | csrt
    int maxMissed = 15;
    double fps = 0;  // 0 = from video
    int smooth = 15, trail = 300;
    std::string output, csv = "sphere_track.csv", plot = "sphere_track.png";
    bool noDisplay = false;
    int maxFrames = 0;
};

struct Candidate {
    double x, y, r, score;
};

// ---------------------------------------------------------------- geometry
struct Circle { double cx, cy, r; };
Circle fitCircle(const std::vector<double>& xs, const std::vector<double>& ys);  // Kasa LSQ
double angleAbout(double x, double y, double cx, double cy);  // CCW+ on screen
std::vector<double> unwrap(const std::vector<double>& a);
double linearSlope(const std::vector<double>& t, const std::vector<double>& y);

// Orbit model fitted from recent detections (NaN = missed).
struct Orbit { double cx, cy, R, thetaLast, omega, tLast; };
std::optional<Orbit> orbitModel(const std::vector<double>& xs, const std::vector<double>& ys,
                                const std::vector<double>& ts,
                                const std::optional<cv::Point2d>& center, int minPts = 20, int k = 15);
cv::Point2d predictOnOrbit(const Orbit& o, double t);

// ---------------------------------------------------------------- detection
class Detector {
public:
    explicit Detector(const Options& o);
    std::vector<Candidate> candidates(const cv::Mat& bgr);
    void setHsv(const cv::Scalar& lo, const cv::Scalar& hi) { opt_.hsvLower = lo; opt_.hsvUpper = hi; }

private:
    std::vector<cv::Vec3f> hough(const cv::Mat& gray) const;
    std::vector<cv::Vec3f> blobs(const cv::Mat& mask) const;
    double silverScore(const cv::Mat& hsv, const cv::Mat& fg, double x, double y, double r) const;
    double colorScore(const cv::Mat& mask, double x, double y, double r) const;
    Options opt_;
    cv::Ptr<cv::BackgroundSubtractorMOG2> bg_;
};

// Pick a candidate (gating by prediction distance and orbit radius).
std::optional<Candidate> selectCandidate(const std::vector<Candidate>& cands,
                                         const std::optional<cv::Point2d>& pred, double gate,
                                         bool locked, const std::optional<Orbit>& orbit,
                                         double orbitTol);

void hsvRangeFromPoint(const cv::Mat& bgr, int x, int y, cv::Scalar& lo, cv::Scalar& hi);

// ---------------------------------------------------------------- analysis
struct TrackData {
    std::vector<int> frames;
    std::vector<double> ts, xs, ys;  // xs/ys NaN when missed
};
struct Result {
    double meanRpm = 0, omega = 0, cx = 0, cy = 0, R = 0;
    int detected = 0, total = 0;
};
Result analyse(const Options& opt, const TrackData& d, cv::Size frameSize);
