#include "sensorguard/types.hpp"

namespace sg {

const char* toString(FaultType f) {
    switch (f) {
        case FaultType::None:       return "none";
        case FaultType::Dropout:    return "dropout";
        case FaultType::OutOfRange: return "out_of_range";
        case FaultType::StuckAt:    return "stuck_at";
        case FaultType::Spike:      return "spike";
        case FaultType::Drift:      return "drift";
        case FaultType::Noise:      return "noise";
    }
    return "unknown";
}

const char* toString(InjectMode m) {
    switch (m) {
        case InjectMode::None:    return "none";
        case InjectMode::Stuck:   return "stuck";
        case InjectMode::Spike:   return "spike";
        case InjectMode::Drift:   return "drift";
        case InjectMode::Noise:   return "noise";
        case InjectMode::Dropout: return "dropout";
    }
    return "unknown";
}

bool parseInjectMode(const std::string& text, InjectMode& out) {
    for (uint32_t i = 0; i <= static_cast<uint32_t>(InjectMode::Dropout); ++i) {
        auto m = static_cast<InjectMode>(i);
        if (text == toString(m)) { out = m; return true; }
    }
    return false;
}

}  // namespace sg
