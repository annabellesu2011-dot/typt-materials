// magnetic_carousel.cpp -- IYPT/TYPT 2026 Problem 16 "Magnetic carousel".
//
// N neodymium magnets with alternating vertical moments sit on a disc at
// radius R_m that turns at angular speed Omega. A steel ball rests on a fixed
// non-magnetic plate a gap h above the magnet tops. The ball's induced
// magnetisation lags the changing field (time constant tau). Depending on the
// parameters the ball either gets locked in a field maximum and CO-rotates
// with the disc, or slips out and is rolled backwards (COUNTER-rotation) by
// the torque m x B of its lagging moment.
//
// Model (SI units, z axis vertical, disc turning counter-clockwise for Omega>0)
//   magnets   : point dipoles (or a sub-dipole grid per cylinder),
//               |m_k| = Br V_mag / mu0, sign (-1)^k, at angle 2 pi k/N + Omega t
//   field     : B = mu0/(4 pi) [3 r (m.r)/r^5 - m/r^3]   (summed)
//   ball      : point dipole at centre (x, y, z_b), z_b = t_mag/2 + h + a
//   induced   : m_eq = m_S tanh(|m_lin|/m_S) B^,  m_lin = V chi_eff B/mu0,
//               chi_eff = 3(mu_r-1)/(mu_r+2),   m_S = M_s V
//   lag       : dm/dt = (m_eq - m)/tau + spin_carry * (w x m)
//   force     : F = grad(m.B) at fixed m (analytic dipole-dipole formula)
//   torque    : T_mag = m x B
//   contact   : N = M g - F_z (>=0), slip velocity v_c = v + w x (-a z)
//               friction: force that keeps v_c -> 0 (rolling, relaxation time
//               tau_stick) if it is <= mu N, else mu N along it (sliding)
//               rolling resistance torque  -mu_rr_len N w_h/|w_h|
//               pivot (spin) friction      -mu_spin_len N w_z/|w_z|
//               small viscous terms -c_v v, -c_w w
//   EOM       : M dv/dt = F_xy + f;   I dw/dt = T_mag + (-a z) x f + T_rr + T_sp - c_w w
// State vector (10): x, y, vx, vy, wx, wy, wz, mx, my, mz. Hand-written RK4.
//
// Build: g++ -O2 -std=c++17 -pthread -o magnetic_carousel magnetic_carousel.cpp
// Run  : ./magnetic_carousel [out_dir=out_mag] [mode=all] [key=value ...]
//        mode: all | quick (Omega sweep + console table) | traj (single run of the
//        defaults -> traj_single.csv) | any combination of section names, e.g.
//        "Omega,ramp,map,traj,checks,sweeps" or a single variable "h_mm", "tau_ms" ...
//        key=value overrides any Params field (e.g. h=0.008 tau=2e-3 N=12) or n_phase=4
// Standard library only (std::thread for parallel sweeps).

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

constexpr double PI  = 3.14159265358979323846;
constexpr double MU0 = 4e-7 * PI;

// ================================ PARAMETER BLOCK ================================
struct Params {
    // --- magnets / disc ---
    int    N          = 8;        // number of magnets (even -> alternating pattern closes)
    double R_m        = 0.05;     // radius of magnet circle [m]
    double mag_d      = 0.010;    // magnet diameter [m]
    double mag_t      = 0.005;    // magnet thickness [m]
    double Br         = 1.3;      // remanence (N42) [T]
    int    mag_model  = 0;        // 0 = point dipole, 1 = 2 layers x 7 sub-dipoles per cylinder
    double Omega      = 20.0;     // disc angular speed [rad/s] (>0 = counter-clockwise)
    double Omega_rate = 0.0;      // dOmega/dt [rad/s^2] (only for ramp runs; Omega(t) = Omega + rate*t)
    // --- geometry ---
    double h          = 0.005;    // gap: magnet top -> plate top surface (= ball bottom) [m]
    // --- steel ball ---
    double a          = 0.005;    // ball radius [m]
    double rho        = 7850.0;   // density [kg/m^3]
    double mu_r       = 100.0;    // relative permeability (chi_eff = 3(mu_r-1)/(mu_r+2))
    double Ms         = 1.6e6;    // saturation magnetisation [A/m]
    double tau        = 1e-3;     // magnetisation lag time [s]
    double spin_carry = 1.0;      // 1: magnetisation is carried by ball rotation (w x m), 0: off
    // --- contact ---
    double g          = 9.81;     // [m/s^2]
    double mu         = 0.3;      // Coulomb friction coefficient ball/plate
    double mu_rr_len  = 5e-6;     // rolling-resistance length [m] (default 1e-3 * a)
    double mu_spin_len= 2e-5;     // pivot (spin about z) friction length [m]
    double c_v        = 1e-4;     // viscous translational damping [N s/m]
    double c_w        = 1e-10;    // viscous rotational damping [N m s]
    double eps_w      = 0.05;     // regularisation of rolling/spin friction [rad/s]
    // --- initial condition ---
    double phi0_frac  = 0.25;     // initial ball angle in units of magnet spacing (0 = above magnet 0)
    // --- numerics ---
    double dt_max     = 5e-5;     // max RK4 step [s]; also dt <= tau/3, <= field period/400
    double T_run      = 3.0;      // simulated time [s]
    double avg_frac   = 0.5;      // average omega_ball over last avg_frac of the run
    double escape_dr  = 0.03;     // |r - R_m| beyond this -> ball escaped, run stops [m]
};

// sweep settings
struct SweepSettings {
    double Om_min = 0.0, Om_max = 60.0, Om_step = 0.5;           // main Omega sweep
    std::vector<double> Om_fixed = {4.0, 30.0};                  // Omega values for 1-D parameter sweeps
    std::vector<double> h_vals   = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};   // [mm]
    std::vector<int>    N_vals   = {4,6,8,10,12,14,16};
    std::vector<double> a_vals   = {2,3,4,5,6,7,8,9,10};                     // [mm]
    std::vector<double> tau_vals = {0.01,0.02,0.05,0.1,0.2,0.5,1,2,5,10,20};// [ms]
    std::vector<double> mu_vals  = {0.05,0.1,0.2,0.3,0.5,0.8,1.0};
    std::vector<double> rr_vals  = {1e-7,3e-7,1e-6,3e-6,1e-5,3e-5,1e-4};     // rolling-resistance length [m]
    // 2-D map
    double map_Om_min = 1.0, map_Om_max = 60.0; int map_nOm = 40;
    double map_h_min = 1.0, map_h_max = 15.0;   int map_nh  = 15;    // [mm]
    // critical-Omega search: coarse scan then bisection
    double crit_step = 2.0, crit_max = 80.0; int crit_bisect = 5;
    double lock_ratio = 0.9;          // omega_ball/Omega above this = locked co-rotation
    // example trajectories
    double traj_Om_co = 5.0, traj_Om_counter = 30.0, traj_T = 3.0; int traj_every = 20;
    // hysteresis ramps (Omega swept slowly up from 1 and down from Om_max)
    double ramp_up_from = 1.0, ramp_up_to = 120.0, ramp_up_rate = 4.0;      // [rad/s], [rad/s^2]
    double ramp_dn_from = 40.0, ramp_dn_to = 1.0, ramp_dn_rate = 2.0;
    double ramp_window = 0.25; int ramp_nphase = 4;                            // [s]
};
// =================================================================================

using V3 = std::array<double, 3>;
static inline V3 operator+(V3 a, V3 b) { return {a[0]+b[0], a[1]+b[1], a[2]+b[2]}; }
static inline V3 operator-(V3 a, V3 b) { return {a[0]-b[0], a[1]-b[1], a[2]-b[2]}; }
static inline V3 operator*(double s, V3 a) { return {s*a[0], s*a[1], s*a[2]}; }
static inline double dot(V3 a, V3 b) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }
static inline V3 cross(V3 a, V3 b) { return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]}; }
static inline double nrm(V3 a) { return std::sqrt(dot(a, a)); }

using State = std::array<double, 10>;

struct Model {
    Params p;
    double M, I, V, chi, mS, mmag_tot, zb;
    std::vector<V3> sub_off;     // sub-dipole offsets relative to magnet centre (local frame, x radial)
    std::vector<double> sub_w;   // weights (sum 1)
    explicit Model(const Params& pp) : p(pp) {
        V = 4.0 / 3.0 * PI * p.a * p.a * p.a;
        M = p.rho * V;
        I = 0.4 * M * p.a * p.a;
        chi = 3.0 * (p.mu_r - 1.0) / (p.mu_r + 2.0);
        mS = p.Ms * V;
        double Vm = PI * 0.25 * p.mag_d * p.mag_d * p.mag_t;
        mmag_tot = p.Br * Vm / MU0;
        zb = 0.5 * p.mag_t + p.h + p.a;
        if (p.mag_model == 0) { sub_off = {{0, 0, 0}}; sub_w = {1.0}; }
        else {
            // 7 points per layer (centre + hexagon at r = sqrt(2/3) R: equal-area-ish), 2 layers at +-t/4
            double r = std::sqrt(2.0 / 3.0) * 0.5 * p.mag_d;
            for (double zz : {-0.25 * p.mag_t, 0.25 * p.mag_t}) {
                sub_off.push_back({0, 0, zz});
                for (int j = 0; j < 6; ++j) sub_off.push_back({r * std::cos(j * PI / 3), r * std::sin(j * PI / 3), zz});
            }
            sub_w.assign(sub_off.size(), 1.0 / sub_off.size());
        }
    }
    double Om(double t) const { return p.Omega + p.Omega_rate * t; }
    double disc_angle(double t) const { return p.Omega * t + 0.5 * p.Omega_rate * t * t; }

    // Field B and force grad(mb.B) at point P (ball centre) for disc angle th.
    void field_force(double th, V3 P, V3 mb, V3& B, V3& F) const {
        B = {0, 0, 0}; F = {0, 0, 0};
        const double k = 1e-7;
        for (int i = 0; i < p.N; ++i) {
            double ang = 2.0 * PI * i / p.N + th, c = std::cos(ang), s = std::sin(ang);
            double sgn = (i % 2 == 0) ? 1.0 : -1.0;
            for (size_t j = 0; j < sub_off.size(); ++j) {
                const V3& o = sub_off[j];
                V3 pos{(p.R_m + o[0]) * c - o[1] * s, (p.R_m + o[0]) * s + o[1] * c, o[2]};
                double mz = sgn * mmag_tot * sub_w[j];
                V3 m{0, 0, mz};
                V3 r = P - pos;
                double r2 = dot(r, r), rn = std::sqrt(r2), r5 = r2 * r2 * rn;
                double mr = mz * r[2], br = dot(mb, r), mm = mz * mb[2];
                B = B + (k / r5) * (3.0 * mr * r - r2 * m);
                F = F + (3.0 * k / r5) * (mr * mb + br * m + mm * r - (5.0 * mr * br / r2) * r);
            }
        }
    }

    struct Aux { V3 B, F, f, Tm; double Nn; bool stick; };

    State rhs(double t, const State& s, Aux* aux = nullptr) const {
        V3 P{s[0], s[1], zb}, v{s[2], s[3], 0}, w{s[4], s[5], s[6]}, m{s[7], s[8], s[9]};
        V3 B, F;
        field_force(disc_angle(t), P, m, B, F);
        // equilibrium moment
        double Bn = nrm(B);
        V3 meq{0, 0, 0};
        if (Bn > 1e-15) {
            double mlin = V * chi * Bn / MU0;
            meq = (mS * std::tanh(mlin / mS) / Bn) * B;
        }
        V3 dm = (1.0 / p.tau) * (meq - m);
        if (p.spin_carry != 0.0) dm = dm + p.spin_carry * cross(w, m);
        V3 Tm = cross(m, B);
        double Nn = std::max(0.0, M * p.g - F[2]);
        // non-friction torques
        double wh = std::hypot(w[0], w[1]);
        V3 T = Tm - p.c_w * w;
        double rr = p.mu_rr_len * Nn / std::sqrt(wh * wh + p.eps_w * p.eps_w);
        T[0] -= rr * w[0]; T[1] -= rr * w[1];
        T[2] -= p.mu_spin_len * Nn * w[2] / std::sqrt(w[2] * w[2] + p.eps_w * p.eps_w);
        double Fx = F[0] - p.c_v * v[0], Fy = F[1] - p.c_v * v[1];
        // contact slip velocity and friction
        double vcx = v[0] - p.a * w[1], vcy = v[1] + p.a * w[0];
        double tau_st = 3.0 * dt_used;
        double kk = 1.0 / M + p.a * p.a / I;
        double fx = (-vcx / tau_st - (Fx / M - p.a * T[1] / I)) / kk;
        double fy = (-vcy / tau_st - (Fy / M + p.a * T[0] / I)) / kk;
        double fn = std::hypot(fx, fy), fmax = p.mu * Nn;
        bool stick = true;
        if (fn > fmax) { fx *= fmax / fn; fy *= fmax / fn; stick = false; }
        State d;
        d[0] = v[0]; d[1] = v[1];
        d[2] = (Fx + fx) / M; d[3] = (Fy + fy) / M;
        d[4] = (T[0] + p.a * fy) / I;
        d[5] = (T[1] - p.a * fx) / I;
        d[6] = T[2] / I;
        d[7] = dm[0]; d[8] = dm[1]; d[9] = dm[2];
        if (aux) *aux = {B, F, {fx, fy, 0}, Tm, Nn, stick};
        return d;
    }

    double dt_used = 1e-5;
    double choose_dt() {
        double dt = p.dt_max;
        dt = std::min(dt, p.tau / 3.0);
        double Ommax = std::max(std::fabs(p.Omega), std::fabs(p.Omega + p.Omega_rate * p.T_run));
        if (Ommax > 0) dt = std::min(dt, 2.0 * PI / (p.N * Ommax) / 400.0);
        dt_used = dt;
        return dt;
    }

    void rk4(double t, double h, State& s) const {
        State k1 = rhs(t, s), k2, k3, k4, tmp;
        for (int i = 0; i < 10; ++i) tmp[i] = s[i] + 0.5 * h * k1[i];
        k2 = rhs(t + 0.5 * h, tmp);
        for (int i = 0; i < 10; ++i) tmp[i] = s[i] + 0.5 * h * k2[i];
        k3 = rhs(t + 0.5 * h, tmp);
        for (int i = 0; i < 10; ++i) tmp[i] = s[i] + h * k3[i];
        k4 = rhs(t + h, tmp);
        for (int i = 0; i < 10; ++i) s[i] += h / 6.0 * (k1[i] + 2 * k2[i] + 2 * k3[i] + k4[i]);
    }

    State initial() const {
        double phi = 2.0 * PI / p.N * p.phi0_frac;
        State s{};
        s[0] = p.R_m * std::cos(phi); s[1] = p.R_m * std::sin(phi);
        V3 B, F;
        field_force(0.0, {s[0], s[1], zb}, {0, 0, 0}, B, F);
        double Bn = nrm(B);
        if (Bn > 0) { V3 meq = (mS * std::tanh(V * chi * Bn / MU0 / mS) / Bn) * B; s[7] = meq[0]; s[8] = meq[1]; s[9] = meq[2]; }
        return s;
    }
};

static bool set_param(Params& p, const std::string& k, double v) {
#define P_(name) if (k == #name) { p.name = v; return true; }
    P_(R_m) P_(mag_d) P_(mag_t) P_(Br) P_(Omega) P_(h) P_(a) P_(rho) P_(mu_r) P_(Ms) P_(tau) P_(spin_carry)
    P_(g) P_(mu) P_(mu_rr_len) P_(mu_spin_len) P_(c_v) P_(c_w) P_(eps_w) P_(phi0_frac) P_(dt_max) P_(T_run)
    P_(avg_frac) P_(escape_dr) P_(Omega_rate)
#undef P_
    if (k == "N") { p.N = (int)v; return true; }
    if (k == "mag_model") { p.mag_model = (int)v; return true; }
    return false;
}

struct Result {
    double omega_ball = 0, ratio = 0, r_mean = 0, spin_h = 0, slip_frac = 0, N_mean = 0, m_mean = 0;
    bool escaped = false; double t_escape = NAN, r_escape = NAN;
    int regime = 0;   // +1 co-rotation (locked), -1 counter-rotation, 0 drift/stall, 2 escaped
};

using TrajCB = std::function<void(double, const State&, const Model::Aux&, double theta_unwrapped)>;

static Result simulate(const Params& p, int traj_every = 0, const TrajCB& cb = nullptr, double lock_ratio = 0.9) {
    Model mdl(p);
    double dt = mdl.choose_dt();
    long n = std::lround(p.T_run / dt);
    long n_avg0 = std::lround((1.0 - p.avg_frac) * n);
    State s = mdl.initial();
    double th = std::atan2(s[1], s[0]), th_prev = th, th_unw = th, th_at_avg0 = th;
    Result R;
    double sum_r = 0, sum_spin = 0, sum_N = 0, sum_m = 0; long cnt = 0, slipcnt = 0;
    for (long k = 0; k < n; ++k) {
        double t = k * dt;
        if (cb && traj_every > 0 && k % traj_every == 0) {
            Model::Aux ax; mdl.rhs(t, s, &ax); cb(t, s, ax, th_unw);
        }
        mdl.rk4(t, dt, s);
        th = std::atan2(s[1], s[0]);
        double d = th - th_prev;
        if (d > PI) d -= 2 * PI; else if (d < -PI) d += 2 * PI;
        th_unw += d; th_prev = th;
        double r = std::hypot(s[0], s[1]);
        if (std::fabs(r - p.R_m) > p.escape_dr || !std::isfinite(r)) {
            R.escaped = true; R.t_escape = (k + 1) * dt; R.r_escape = r; break;
        }
        if (k + 1 == n_avg0) th_at_avg0 = th_unw;
        if (k + 1 > n_avg0) {
            Model::Aux ax;
            if ((k & 7) == 0) {  // sample diagnostics sparsely
                mdl.rhs((k + 1) * dt, s, &ax);
                sum_N += ax.Nn; slipcnt += ax.stick ? 0 : 1;
                sum_m += std::sqrt(s[7]*s[7] + s[8]*s[8] + s[9]*s[9]);
                sum_r += r; sum_spin += std::hypot(s[4], s[5]); ++cnt;
            }
        }
    }
    if (R.escaped) { R.regime = 2; R.omega_ball = NAN; R.ratio = NAN; return R; }
    double Tav = (n - n_avg0) * dt;
    R.omega_ball = (th_unw - th_at_avg0) / Tav;
    R.ratio = p.Omega != 0 ? R.omega_ball / p.Omega : 0;
    if (cnt) { R.r_mean = sum_r / cnt; R.spin_h = sum_spin / cnt; R.N_mean = sum_N / cnt; R.m_mean = sum_m / cnt; R.slip_frac = double(slipcnt) / cnt; }
    if (p.Omega == 0) R.regime = 0;
    else if (R.ratio > lock_ratio) R.regime = 1;
    else if (R.ratio < -0.02) R.regime = -1;
    else R.regime = 0;
    return R;
}

// ---------------- parallel job runner ----------------
static void parallel_for(size_t n, const std::function<void(size_t)>& f) {
    unsigned nt = std::max(1u, std::thread::hardware_concurrency());
    std::atomic<size_t> next{0};
    std::vector<std::thread> pool;
    for (unsigned i = 0; i < nt; ++i)
        pool.emplace_back([&] { size_t j; while ((j = next++) < n) f(j); });
    for (auto& t : pool) t.join();
}

// Phase-averaged result: the same case started from n_phase initial ball positions
// spread over one period of the alternating pattern (2 magnet spacings).
struct PResult {
    double w_mean = NAN, w_min = NAN, w_max = NAN, ratio = NAN;
    double f_lock = 0, f_counter = 0, f_escape = 0, f_drift = 0;
    double spin_h = NAN, slip_frac = NAN, N_mean = NAN, m_mean = NAN;
    int regime = 0; // majority: +1 co, -1 counter, 0 drift, 2 escape
};

static PResult combine(const std::vector<Result>& rs, double Omega) {
    PResult P; int ok = 0; double sw = 0, ss = 0, sl = 0, sN = 0, sm = 0;
    int nl = 0, nc = 0, ne = 0, nd = 0;
    for (auto& r : rs) {
        if (r.regime == 2) { ++ne; continue; }
        if (r.regime == 1) ++nl; else if (r.regime == -1) ++nc; else ++nd;
        ++ok; sw += r.omega_ball; ss += r.spin_h; sl += r.slip_frac; sN += r.N_mean; sm += r.m_mean;
        P.w_min = std::isnan(P.w_min) ? r.omega_ball : std::min(P.w_min, r.omega_ball);
        P.w_max = std::isnan(P.w_max) ? r.omega_ball : std::max(P.w_max, r.omega_ball);
    }
    double n = (double)rs.size();
    P.f_lock = nl / n; P.f_counter = nc / n; P.f_escape = ne / n; P.f_drift = nd / n;
    if (ok) { P.w_mean = sw / ok; P.spin_h = ss / ok; P.slip_frac = sl / ok; P.N_mean = sN / ok; P.m_mean = sm / ok;
              P.ratio = Omega != 0 ? P.w_mean / Omega : 0; }
    int best = std::max({nl, nc, ne, nd});
    P.regime = (best == nl) ? 1 : (best == nc) ? -1 : (best == ne) ? 2 : 0;
    return P;
}

static int g_nphase = 8;
static std::vector<PResult> run_all(const std::vector<Params>& ps, double lock_ratio) {
    const int np = g_nphase;
    std::vector<Result> raw(ps.size() * np);
    parallel_for(raw.size(), [&](size_t j) {
        Params p = ps[j / np];
        if (np > 1) p.phi0_frac = 2.0 * (j % np) / np;   // else keep Params::phi0_frac
        raw[j] = simulate(p, 0, nullptr, lock_ratio);
    });
    std::vector<PResult> out(ps.size());
    for (size_t i = 0; i < ps.size(); ++i)
        out[i] = combine(std::vector<Result>(raw.begin() + i * np, raw.begin() + (i + 1) * np), ps[i].Omega);
    return out;
}

// phase-averaged single case (serial; used inside parallel critical searches)
static PResult run_case(Params p, double lock_ratio) {
    std::vector<Result> rs;
    for (int k = 0; k < g_nphase; ++k) { if (g_nphase > 1) p.phi0_frac = 2.0 * k / g_nphase; rs.push_back(simulate(p, 0, nullptr, lock_ratio)); }
    return combine(rs, p.Omega);
}

// Critical speeds:
//  Om_unlock : lowest Omega at which fewer than half of the starting phases lock (co-rotation lost)
//  Om_counter: lowest Omega >= Om_unlock above which at least half of the phases counter-rotate
struct Crit { double Om_unlock = NAN, Om_counter = NAN, Om_estimate = NAN; };

static Crit critical_Omega(Params p, const SweepSettings& S) {
    Crit C;
    auto lockfrac = [&](double Om) { p.Omega = Om; return run_case(p, S.lock_ratio).f_lock; };
    auto ctrfrac  = [&](double Om) { p.Omega = Om; return run_case(p, S.lock_ratio).f_counter; };
    double lo = 0.0, hi = NAN;
    for (double Om = S.crit_step; Om <= S.crit_max + 1e-9; Om += S.crit_step) {
        if (lockfrac(Om) >= 0.5) lo = Om; else { hi = Om; break; }
    }
    if (!std::isnan(hi)) {
        for (int i = 0; i < S.crit_bisect; ++i) { double mid = 0.5 * (lo + hi); (lockfrac(mid) >= 0.5 ? lo : hi) = mid; }
        C.Om_unlock = 0.5 * (lo + hi);
        double lo2 = C.Om_unlock, hi2 = NAN;
        for (double Om = C.Om_unlock; Om <= S.crit_max + 1e-9; Om += S.crit_step) {
            if (ctrfrac(Om) >= 0.5) { hi2 = Om; break; } else lo2 = Om;
        }
        if (!std::isnan(hi2)) {
            for (int i = 0; i < S.crit_bisect; ++i) { double mid = 0.5 * (lo2 + hi2); (ctrfrac(mid) >= 0.5 ? hi2 : lo2) = mid; }
            C.Om_counter = 0.5 * (lo2 + hi2);
        }
    }
    // capture estimate: ball at rest must not overcome the barrier of U = -(chi V/2mu0) B^2
    // in the frame co-rotating with the disc:  0.7 M (Omega R)^2 = Delta U
    {
        Model m(p); double Bmax = 0, Bmin = 1e9;
        for (int k = 0; k < 400; ++k) {
            double ph = 2.0 * PI / p.N * 2.0 * k / 400.0;
            V3 B, F; m.field_force(0.0, {p.R_m * std::cos(ph), p.R_m * std::sin(ph), m.zb}, {0, 0, 0}, B, F);
            double b = nrm(B); Bmax = std::max(Bmax, b); Bmin = std::min(Bmin, b);
        }
        double dU_per_M = m.chi * (Bmax * Bmax - Bmin * Bmin) / (2.0 * MU0 * p.rho);
        C.Om_estimate = std::sqrt(dU_per_M / 0.7) / p.R_m;
    }
    return C;
}

static std::vector<Crit> critical_all(const std::vector<Params>& ps, const SweepSettings& S) {
    std::vector<Crit> out(ps.size());
    parallel_for(ps.size(), [&](size_t i) { out[i] = critical_Omega(ps[i], S); });
    return out;
}

static const char* RES_HDR = "omega_ball,ratio,omega_min,omega_max,f_lock,f_counter,f_escape,f_drift,regime,spin_h,slip_frac,N_mean,m_mean";
static void write_res(std::ostream& f, const PResult& r) {
    f << r.w_mean << ',' << r.ratio << ',' << r.w_min << ',' << r.w_max << ',' << r.f_lock << ',' << r.f_counter << ','
      << r.f_escape << ',' << r.f_drift << ',' << r.regime << ',' << r.spin_h << ',' << r.slip_frac << ',' << r.N_mean << ',' << r.m_mean;
}

int main(int argc, char** argv) {
    namespace fs = std::filesystem;
    fs::path out = argc > 1 ? argv[1] : "out_mag";
    std::string mode = argc > 2 ? argv[2] : "all";
    fs::create_directories(out);
    Params P0;
    SweepSettings S;
    for (int i = 3; i < argc; ++i) {           // key=value overrides of Params (or n_phase=...)
        std::string kv = argv[i]; auto e = kv.find('=');
        if (e != std::string::npos && kv.substr(0, e) == "n_phase") { g_nphase = std::stoi(kv.substr(e + 1)); continue; }
        if (e == std::string::npos || !set_param(P0, kv.substr(0, e), std::stod(kv.substr(e + 1)))) {
            std::cerr << "bad override " << kv << "\n"; return 1; }
    }
    if (g_nphase < 1) g_nphase = 1;
    auto open = [&](const std::string& name) {
        std::ofstream f(out / name); f << std::setprecision(8);
        if (!f) { std::cerr << "cannot write " << name << "\n"; std::exit(1); }
        return f;
    };
    auto want = [&](const char* sec) { return mode == "all" || mode == "quick" || mode.find(sec) != std::string::npos; };
    auto tic = std::chrono::steady_clock::now();
    auto elapsed = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - tic).count(); };
    {
        Model m(P0);
        std::printf("Ball M=%.3g kg, magnet moment=%.3g A m^2, z_ball=%.1f mm, chi_eff=%.3f, m_sat=%.3g A m^2, phases/case=%d\n",
                    m.M, m.mmag_tot, m.zb * 1e3, m.chi, m.mS, g_nphase);
        auto f = open("params.csv");
        f << "name,value\nN," << P0.N << "\nR_m," << P0.R_m << "\nmag_d," << P0.mag_d << "\nmag_t," << P0.mag_t << "\nBr," << P0.Br
          << "\nmag_model," << P0.mag_model << "\nh," << P0.h << "\na," << P0.a << "\nrho," << P0.rho << "\nmu_r," << P0.mu_r
          << "\nMs," << P0.Ms << "\ntau," << P0.tau << "\nspin_carry," << P0.spin_carry << "\nmu," << P0.mu
          << "\nmu_rr_len," << P0.mu_rr_len << "\nmu_spin_len," << P0.mu_spin_len << "\nc_v," << P0.c_v << "\nc_w," << P0.c_w
          << "\nT_run," << P0.T_run << "\navg_frac," << P0.avg_frac << "\nn_phase," << g_nphase
          << "\nmagnet_moment," << m.mmag_tot << "\nball_mass," << m.M << "\n";
    }

    if (mode == "traj") {   // single trajectory with the (overridden) defaults -> traj_single.csv
        auto f = open("traj_single.csv");
        f << "t,x,y,r,theta_unwrapped,omega_inst,wx,wy,wz,N,stick\n";
        Params p = P0; p.escape_dr = 1.0;
        Result r = simulate(p, 50, [&](double t, const State& s, const Model::Aux& ax, double thu) {
            double r2 = s[0]*s[0] + s[1]*s[1];
            f << t << ',' << s[0] << ',' << s[1] << ',' << std::sqrt(r2) << ',' << thu << ',' << (s[0]*s[3] - s[1]*s[2]) / r2
              << ',' << s[4] << ',' << s[5] << ',' << s[6] << ',' << ax.Nn << ',' << ax.stick << '\n';
        });
        std::printf("omega_ball=%.4f ratio=%.4f regime=%d\n", r.omega_ball, r.ratio, r.regime);
        return 0;
    }
    // ---------- 1. omega_ball vs Omega ----------
    if (want("Omega")) {
        std::vector<Params> ps;
        int n = (int)std::lround((S.Om_max - S.Om_min) / S.Om_step);
        for (int i = 0; i <= n; ++i) { Params p = P0; p.Omega = S.Om_min + i * S.Om_step; ps.push_back(p); }
        auto rs = run_all(ps, S.lock_ratio);
        auto f = open("sweep_Omega.csv");
        f << "Omega," << RES_HDR << "\n";
        for (size_t i = 0; i < ps.size(); ++i) { f << ps[i].Omega << ','; write_res(f, rs[i]); f << '\n'; }
        std::printf("[sweep_Omega] %zu cases  (%.1f s)\n", ps.size(), elapsed());
        if (mode == "quick") {
            for (size_t i = 0; i < ps.size(); i += 4)
                std::printf("  Om=%5.1f w=%8.3f ratio=%7.3f [%7.3f,%7.3f] lock=%.2f ctr=%.2f esc=%.2f spin=%.1f slip=%.2f\n", ps[i].Omega,
                            rs[i].w_mean, rs[i].ratio, rs[i].w_min, rs[i].w_max, rs[i].f_lock, rs[i].f_counter, rs[i].f_escape,
                            rs[i].spin_h, rs[i].slip_frac);
            Crit c = critical_Omega(P0, S);
            std::printf("  Om_unlock=%.2f Om_counter=%.2f Om_estimate=%.2f\n", c.Om_unlock, c.Om_counter, c.Om_estimate);
            return 0;
        }
    }

    // ---------- 2. hysteresis: slow ramps up and down ----------
    if (want("ramp")) {
        auto f = open("ramp.csv");
        f << "direction,phase,t,Omega,omega_ball_window\n";
        struct R { std::string dir; double Om0, Om1, rate; };
        std::vector<R> ramps = {{"up", S.ramp_up_from, S.ramp_up_to, S.ramp_up_rate},
                                {"down", S.ramp_dn_from, S.ramp_dn_to, -S.ramp_dn_rate}};
        std::vector<std::vector<std::array<double, 3>>> res(ramps.size() * S.ramp_nphase);
        parallel_for(res.size(), [&](size_t j) {
            const R& rp = ramps[j / S.ramp_nphase];
            Params p = P0; p.Omega = rp.Om0; p.Omega_rate = rp.rate; p.T_run = (rp.Om1 - rp.Om0) / rp.rate;
            p.phi0_frac = 2.0 * (j % S.ramp_nphase) / S.ramp_nphase; p.escape_dr = 1e9;  // never stop; flag by r
            double tlast = 0, thlast = NAN;
            auto& v = res[j];
            simulate(p, 1, [&](double t, const State& s, const Model::Aux&, double thu) {
                if (std::isnan(thlast)) { thlast = thu; tlast = t; return; }
                if (t - tlast >= S.ramp_window) {
                    double r = std::hypot(s[0], s[1]);
                    double w = (std::fabs(r - p.R_m) > P0.escape_dr) ? NAN : (thu - thlast) / (t - tlast);
                    v.push_back({0.5 * (t + tlast), p.Omega + p.Omega_rate * 0.5 * (t + tlast), w});
                    tlast = t; thlast = thu;
                }
            });
        });
        for (size_t j = 0; j < res.size(); ++j)
            for (auto& e : res[j]) f << ramps[j / S.ramp_nphase].dir << ',' << j % S.ramp_nphase << ',' << e[0] << ',' << e[1] << ',' << e[2] << '\n';
        std::printf("[ramp] up %.0f->%.0f, down %.0f->%.0f rad/s  (%.1f s)\n", S.ramp_up_from, S.ramp_up_to, S.ramp_dn_from, S.ramp_dn_to, elapsed());
    }

    // ---------- 3. 1-D parameter sweeps at fixed Omega + critical Omega ----------
    struct Var { std::string name; std::vector<double> vals; std::function<void(Params&, double)> set; };
    std::vector<Var> vars = {
        {"h_mm",   S.h_vals,   [](Params& p, double v) { p.h = v * 1e-3; }},
        {"N",      std::vector<double>(S.N_vals.begin(), S.N_vals.end()), [](Params& p, double v) { p.N = (int)v; }},
        {"a_mm",   S.a_vals,   [](Params& p, double v) { p.a = v * 1e-3; }},
        {"tau_ms", S.tau_vals, [](Params& p, double v) { p.tau = v * 1e-3; }},
        {"mu",     S.mu_vals,  [](Params& p, double v) { p.mu = v; }},
        {"mu_rr_len", S.rr_vals, [](Params& p, double v) { p.mu_rr_len = v; }},
    };
    for (auto& V : vars) {
        if (!want(V.name.c_str()) && !want("sweeps")) continue;
        std::vector<Params> ps;
        for (double Om : S.Om_fixed) for (double v : V.vals) { Params p = P0; V.set(p, v); p.Omega = Om; ps.push_back(p); }
        auto rs = run_all(ps, S.lock_ratio);
        std::vector<Params> pc;
        for (double v : V.vals) { Params p = P0; V.set(p, v); pc.push_back(p); }
        auto crit = critical_all(pc, S);
        auto f = open("sweep_" + V.name + ".csv");
        f << V.name << ",Omega," << RES_HDR << "\n";
        size_t idx = 0;
        for (double Om : S.Om_fixed) for (double v : V.vals) { f << v << ',' << Om << ','; write_res(f, rs[idx++]); f << '\n'; }
        auto g = open("crit_" + V.name + ".csv");
        g << V.name << ",Omega_unlock,Omega_counter,Omega_capture_estimate\n";
        for (size_t i = 0; i < V.vals.size(); ++i)
            g << V.vals[i] << ',' << crit[i].Om_unlock << ',' << crit[i].Om_counter << ',' << crit[i].Om_estimate << '\n';
        std::printf("[sweep_%s] %zu cases + critical Omega for %zu values  (%.1f s)\n", V.name.c_str(), ps.size(), V.vals.size(), elapsed());
    }

    // ---------- 4. 2-D map Omega x h ----------
    if (want("map")) {
        std::vector<Params> ps;
        for (int j = 0; j < S.map_nh; ++j)
            for (int i = 0; i < S.map_nOm; ++i) {
                Params p = P0;
                p.h = 1e-3 * (S.map_h_min + (S.map_h_max - S.map_h_min) * j / std::max(1, S.map_nh - 1));
                p.Omega = S.map_Om_min + (S.map_Om_max - S.map_Om_min) * i / std::max(1, S.map_nOm - 1);
                ps.push_back(p);
            }
        auto rs = run_all(ps, S.lock_ratio);
        auto f = open("map_Omega_h.csv");
        f << "h_mm,Omega," << RES_HDR << "\n";
        for (size_t i = 0; i < ps.size(); ++i) { f << ps[i].h * 1e3 << ',' << ps[i].Omega << ','; write_res(f, rs[i]); f << '\n'; }
        std::printf("[map_Omega_h] %zu cases  (%.1f s)\n", ps.size(), elapsed());
    }

    // ---------- 5. example trajectories ----------
    if (want("traj")) for (auto [name, Om] : std::vector<std::pair<std::string, double>>{{"co", S.traj_Om_co}, {"counter", S.traj_Om_counter}}) {
        Params p = P0; p.Omega = Om; p.T_run = S.traj_T;
        auto f = open("traj_" + name + ".csv");
        f << "t,x,y,theta_unwrapped,omega_inst,disc_angle,wx,wy,wz,mx,my,mz,Bx,By,Bz,Fx,Fy,Fz,N,stick\n";
        Result r = simulate(p, S.traj_every, [&](double t, const State& s, const Model::Aux& ax, double thu) {
            double r2 = s[0]*s[0] + s[1]*s[1];
            f << t << ',' << s[0] << ',' << s[1] << ',' << thu << ',' << (s[0]*s[3] - s[1]*s[2]) / r2 << ',' << Om * t;
            for (int i = 4; i < 10; ++i) f << ',' << s[i];
            f << ',' << ax.B[0] << ',' << ax.B[1] << ',' << ax.B[2] << ',' << ax.F[0] << ',' << ax.F[1] << ',' << ax.F[2]
              << ',' << ax.Nn << ',' << (ax.stick ? 1 : 0) << '\n';
        }, S.lock_ratio);
        std::printf("[traj_%s] Omega=%.1f -> omega_ball=%.3f rad/s (ratio %.3f, regime %d)\n", name.c_str(), Om, r.omega_ball, r.ratio, r.regime);
    }

    // ---------- 6. model checks ----------
    if (want("checks")) {
        auto f = open("model_checks.csv");
        f << "check,Omega," << RES_HDR << "\n";
        std::vector<std::pair<std::string, Params>> cs;
        for (double Om : S.Om_fixed) {
            Params p = P0; p.Omega = Om;
            cs.push_back({"default", p});
            Params q = p; q.mag_model = 1; cs.push_back({"subdipole_magnets", q});
            q = p; q.spin_carry = 0; cs.push_back({"no_spin_carry", q});
            q = p; q.dt_max = 1e-5; cs.push_back({"dt_max_1e-5", q});
            q = p; q.T_run = 6.0; cs.push_back({"T_run_6s", q});
            q = p; q.mu_r = 1000; cs.push_back({"mu_r_1000", q});
            q = p; q.Ms = 1e9; cs.push_back({"no_saturation", q});
        }
        std::vector<Params> ps; for (auto& c : cs) ps.push_back(c.second);
        auto rs = run_all(ps, S.lock_ratio);
        for (size_t i = 0; i < cs.size(); ++i) { f << cs[i].first << ',' << cs[i].second.Omega << ','; write_res(f, rs[i]); f << '\n'; }
        // critical speed with sub-dipole magnets
        Params q = P0; q.mag_model = 1; Crit c1 = critical_Omega(q, S), c0 = critical_Omega(P0, S);
        f << "# Om_unlock dipole=" << c0.Om_unlock << " subdipole=" << c1.Om_unlock << "\n";
        std::printf("[model_checks] %zu cases  (%.1f s)\n", cs.size(), elapsed());
    }
    std::printf("done, CSVs in %s  (total %.1f s)\n", out.string().c_str(), elapsed());
    return 0;
}
