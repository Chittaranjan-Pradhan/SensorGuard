#pragma once
// Fault detectors (one class per fault type) and the FaultEngine that chains them.
#include <deque>
#include <memory>
#include <vector>

#include "sensorguard/types.hpp"

namespace sg {

struct DetectorConfig {
    double      min_c = -40.0, max_c = 85.0;  // physical range of the sensor
    int         stuck_count = 8;              // identical readings => stuck
    double      spike_threshold_c = 5.0;      // deviation from window median
    std::size_t spike_window = 5;
    std::size_t drift_baseline_samples = 20;  // baseline learned from first N samples
    std::size_t drift_window = 10;
    double      drift_threshold_c = 3.0;      // |moving mean - baseline|
    std::size_t noise_window = 10;
    double      noise_stddev_c = 1.0;         // window std-dev limit
};

class IDetector {
public:
    virtual ~IDetector() = default;
    virtual FaultType  evaluate(const Sample& s) = 0;
    virtual void       reset() = 0;
    virtual const char* name() const = 0;
};

class DropoutDetector final : public IDetector {
public:
    FaultType evaluate(const Sample& s) override;
    void reset() override {}
    const char* name() const override { return "dropout"; }
};

class RangeDetector final : public IDetector {
public:
    RangeDetector(double lo, double hi) : lo_(lo), hi_(hi) {}
    FaultType evaluate(const Sample& s) override;
    void reset() override {}
    const char* name() const override { return "range"; }
private:
    double lo_, hi_;
};

class StuckDetector final : public IDetector {
public:
    explicit StuckDetector(int count) : n_(count) {}
    FaultType evaluate(const Sample& s) override;
    void reset() override { run_ = 0; has_ = false; }
    const char* name() const override { return "stuck"; }
private:
    int n_, run_ = 0;
    double last_ = 0.0;
    bool has_ = false;
};

class SpikeDetector final : public IDetector {
public:
    SpikeDetector(double threshold, std::size_t window) : thr_(threshold), w_(window) {}
    FaultType evaluate(const Sample& s) override;
    void reset() override { window_.clear(); }
    const char* name() const override { return "spike"; }
private:
    double thr_;
    std::size_t w_;
    std::deque<double> window_;  // only *accepted* (non-spike) values
};

class DriftDetector final : public IDetector {
public:
    DriftDetector(std::size_t baselineN, std::size_t window, double threshold)
        : bn_(baselineN), w_(window), thr_(threshold) {}
    FaultType evaluate(const Sample& s) override;
    void reset() override { count_ = 0; sum_ = 0.0; baseline_ = 0.0; window_.clear(); }
    const char* name() const override { return "drift"; }
private:
    std::size_t bn_, w_, count_ = 0;
    double thr_, sum_ = 0.0, baseline_ = 0.0;
    std::deque<double> window_;
};

class NoiseDetector final : public IDetector {
public:
    NoiseDetector(std::size_t window, double stddev) : w_(window), thr_(stddev) {}
    FaultType evaluate(const Sample& s) override;
    void reset() override { window_.clear(); }
    const char* name() const override { return "noise"; }
private:
    std::size_t w_;
    double thr_;
    std::deque<double> window_;
};

// Runs detectors in priority order (dropout, range, stuck, spike, drift, noise);
// the first detector that reports a fault wins and later ones do not see the sample.
class FaultEngine {
public:
    explicit FaultEngine(const DetectorConfig& cfg = DetectorConfig{});
    FaultType process(const Sample& s);
    void reset();
private:
    std::vector<std::unique_ptr<IDetector>> detectors_;
};

}  // namespace sg
