#pragma once
// Core value types shared by every SensorGuard component.
#include <cstddef>
#include <cstdint>
#include <string>

namespace sg {

struct Sample {
    uint64_t timestamp_ns = 0;
    double   value_c      = 0.0;   // temperature in degrees Celsius
    uint32_t seq          = 0;
    bool     valid        = true;  // false => sensor reported "no data"
};

// What the *detectors* conclude about a sample.
enum class FaultType { None, Dropout, OutOfRange, StuckAt, Spike, Drift, Noise };
constexpr std::size_t kFaultTypeCount = 7;

// What the *driver/simulator* is told to inject (matches enum vsensor_fault_mode).
enum class InjectMode : uint32_t { None = 0, Stuck = 1, Spike = 2, Drift = 3, Noise = 4, Dropout = 5 };

const char* toString(FaultType f);
const char* toString(InjectMode m);
bool parseInjectMode(const std::string& text, InjectMode& out);

}  // namespace sg
