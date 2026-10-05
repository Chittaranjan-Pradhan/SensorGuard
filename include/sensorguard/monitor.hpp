#pragma once
// Monitor = FaultEngine + StateMachine + statistics (thread-safe stats snapshot).
#include <array>
#include <cstdint>
#include <mutex>

#include "sensorguard/detectors.hpp"
#include "sensorguard/state_machine.hpp"
#include "sensorguard/types.hpp"

namespace sg {

struct Verdict {
    FaultType fault = FaultType::None;
    State     previous = State::Init;
    State     state = State::Init;
    bool      transitioned = false;
};

struct Stats {
    uint64_t samples = 0;
    uint64_t transitions = 0;
    std::array<uint64_t, kFaultTypeCount> fault_counts{};
    State state = State::Init;
};

class Monitor {
public:
    Monitor(const DetectorConfig& dc = DetectorConfig{}, const StateConfig& sc = StateConfig{});
    Verdict onSample(const Sample& s);
    Stats   stats() const;  // safe to call from another thread
private:
    FaultEngine  engine_;
    StateMachine sm_;
    mutable std::mutex mu_;
    Stats stats_;
};

}  // namespace sg
