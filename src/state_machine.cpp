#include "sensorguard/state_machine.hpp"

#include <algorithm>

namespace sg {

const char* toString(State s) {
    switch (s) {
        case State::Init:       return "INIT";
        case State::Normal:     return "NORMAL";
        case State::Suspect:    return "SUSPECT";
        case State::Fault:      return "FAULT";
        case State::Recovering: return "RECOVERING";
    }
    return "UNKNOWN";
}

void StateMachine::enter(State s) {
    state_ = s;
    if (s == State::Normal) { window_.clear(); healthy_ = 0; }
}

State StateMachine::update(bool faulty) {
    prev_ = state_;
    window_.push_back(faulty);
    if (window_.size() > cfg_.confirm_window) window_.pop_front();
    healthy_ = faulty ? 0 : healthy_ + 1;

    switch (state_) {
        case State::Init:
            if (!faulty && healthy_ >= cfg_.warmup_samples) enter(State::Normal);
            break;
        case State::Normal:
            if (faulty) enter(State::Suspect);
            break;
        case State::Suspect: {
            const int faults = static_cast<int>(std::count(window_.begin(), window_.end(), true));
            if (faults >= cfg_.confirm_faults)            enter(State::Fault);
            else if (healthy_ >= cfg_.suspect_clear)      enter(State::Normal);
            break;
        }
        case State::Fault:
            if (healthy_ >= cfg_.fault_to_recovering) enter(State::Recovering);
            break;
        case State::Recovering:
            if (faulty)                                   enter(State::Fault);
            else if (healthy_ >= cfg_.recovering_clear)   enter(State::Normal);
            break;
    }
    return state_;
}

void StateMachine::reset() {
    state_ = prev_ = State::Init;
    healthy_ = 0;
    window_.clear();
}

}  // namespace sg
