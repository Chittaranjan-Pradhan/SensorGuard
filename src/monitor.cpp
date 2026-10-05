#include "sensorguard/monitor.hpp"

namespace sg {

Monitor::Monitor(const DetectorConfig& dc, const StateConfig& sc) : engine_(dc), sm_(sc) {}

Verdict Monitor::onSample(const Sample& s) {
    Verdict v;
    v.fault = engine_.process(s);
    v.state = sm_.update(v.fault != FaultType::None);
    v.previous = sm_.previous();
    v.transitioned = sm_.transitioned();

    std::lock_guard<std::mutex> l(mu_);
    ++stats_.samples;
    ++stats_.fault_counts[static_cast<std::size_t>(v.fault)];
    if (v.transitioned) ++stats_.transitions;
    stats_.state = v.state;
    return v;
}

Stats Monitor::stats() const {
    std::lock_guard<std::mutex> l(mu_);
    return stats_;
}

}  // namespace sg
