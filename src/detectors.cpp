#include "sensorguard/detectors.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace sg {

namespace {
double mean(const std::deque<double>& d) {
    double s = 0.0;
    for (double v : d) s += v;
    return d.empty() ? 0.0 : s / static_cast<double>(d.size());
}
double median(const std::deque<double>& d) {
    std::vector<double> t(d.begin(), d.end());
    std::nth_element(t.begin(), t.begin() + static_cast<long>(t.size() / 2), t.end());
    return t[t.size() / 2];
}
}  // namespace

FaultType DropoutDetector::evaluate(const Sample& s) {
    return s.valid ? FaultType::None : FaultType::Dropout;
}

FaultType RangeDetector::evaluate(const Sample& s) {
    return (s.value_c < lo_ || s.value_c > hi_) ? FaultType::OutOfRange : FaultType::None;
}

FaultType StuckDetector::evaluate(const Sample& s) {
    if (has_ && std::fabs(s.value_c - last_) < 1e-9) ++run_; else run_ = 1;
    last_ = s.value_c;
    has_ = true;
    return run_ >= n_ ? FaultType::StuckAt : FaultType::None;
}

FaultType SpikeDetector::evaluate(const Sample& s) {
    if (!window_.empty() && std::fabs(s.value_c - median(window_)) > thr_)
        return FaultType::Spike;               // outlier is NOT added to the window
    window_.push_back(s.value_c);
    if (window_.size() > w_) window_.pop_front();
    return FaultType::None;
}

FaultType DriftDetector::evaluate(const Sample& s) {
    if (count_ < bn_) {                         // learning phase
        sum_ += s.value_c;
        if (++count_ == bn_) baseline_ = sum_ / static_cast<double>(bn_);
        return FaultType::None;
    }
    window_.push_back(s.value_c);
    if (window_.size() > w_) window_.pop_front();
    if (window_.size() < w_) return FaultType::None;
    return std::fabs(mean(window_) - baseline_) > thr_ ? FaultType::Drift : FaultType::None;
}

FaultType NoiseDetector::evaluate(const Sample& s) {
    window_.push_back(s.value_c);
    if (window_.size() > w_) window_.pop_front();
    if (window_.size() < w_) return FaultType::None;
    const double m = mean(window_);
    double var = 0.0;
    for (double v : window_) var += (v - m) * (v - m);
    var /= static_cast<double>(window_.size());
    return std::sqrt(var) > thr_ ? FaultType::Noise : FaultType::None;
}

FaultEngine::FaultEngine(const DetectorConfig& c) {
    detectors_.emplace_back(new DropoutDetector());
    detectors_.emplace_back(new RangeDetector(c.min_c, c.max_c));
    detectors_.emplace_back(new StuckDetector(c.stuck_count));
    detectors_.emplace_back(new SpikeDetector(c.spike_threshold_c, c.spike_window));
    detectors_.emplace_back(new DriftDetector(c.drift_baseline_samples, c.drift_window, c.drift_threshold_c));
    detectors_.emplace_back(new NoiseDetector(c.noise_window, c.noise_stddev_c));
}

FaultType FaultEngine::process(const Sample& s) {
    for (auto& d : detectors_) {
        FaultType f = d->evaluate(s);
        if (f != FaultType::None) return f;
    }
    return FaultType::None;
}

void FaultEngine::reset() {
    for (auto& d : detectors_) d->reset();
}

}  // namespace sg
