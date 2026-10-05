#include "sensorguard/sources.hpp"

#include <chrono>

namespace sg {

// Constants mirror driver/vsensor.c
static constexpr int32_t kNoiseNormalMc = 200;
static constexpr int32_t kNoiseFaultMc  = 4000;
static constexpr int32_t kSpikeMc       = 25000;
static constexpr uint32_t kSpikePeriod  = 3;
static constexpr int32_t kDriftStepMc   = 100;

SimulatedSource::SimulatedSource(double base_c, uint32_t seed)
    : base_mc_(static_cast<int32_t>(base_c * 1000.0)), rng_(seed), last_good_mc_(base_mc_) {}

int32_t SimulatedSource::noise(int32_t amp) {
    rng_ = rng_ * 1664525u + 1013904223u;
    const uint32_t r = rng_ >> 16;
    return static_cast<int32_t>(r % static_cast<uint32_t>(2 * amp + 1)) - amp;
}

bool SimulatedSource::setFault(InjectMode mode) {
    fault_ = mode;
    fault_seq_ = seq_;
    return true;
}

bool SimulatedSource::read(Sample& out) {
    const int32_t normal = base_mc_ + noise(kNoiseNormalMc);
    const uint32_t since = seq_ - fault_seq_;
    int32_t value = normal;
    bool valid = true;

    switch (fault_) {
        case InjectMode::None:  last_good_mc_ = normal; break;
        case InjectMode::Stuck: value = last_good_mc_; break;
        case InjectMode::Spike:
            if (since % kSpikePeriod == kSpikePeriod - 1) value = normal + kSpikeMc;
            break;
        case InjectMode::Drift: value = normal + static_cast<int32_t>(since) * kDriftStepMc; break;
        case InjectMode::Noise: value = base_mc_ + noise(kNoiseFaultMc); break;
        case InjectMode::Dropout: value = 0; valid = false; break;
    }

    out.timestamp_ns = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    out.value_c = value / 1000.0;
    out.seq = seq_++;
    out.valid = valid;
    return true;
}

}  // namespace sg
