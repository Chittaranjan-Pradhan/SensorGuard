#pragma once
// Health state machine:  INIT -> NORMAL <-> SUSPECT -> FAULT -> RECOVERING -> NORMAL
#include <cstddef>
#include <deque>

namespace sg {

enum class State { Init, Normal, Suspect, Fault, Recovering };
const char* toString(State s);

struct StateConfig {
    int warmup_samples      = 20;  // healthy samples needed to leave INIT
    std::size_t confirm_window = 10;  // look at last N verdicts ...
    int confirm_faults      = 3;   // ... >= K faulty => FAULT
    int suspect_clear       = 5;   // healthy streak that clears SUSPECT
    int fault_to_recovering = 3;   // healthy streak that moves FAULT -> RECOVERING
    int recovering_clear    = 10;  // healthy streak that moves RECOVERING -> NORMAL
};

class StateMachine {
public:
    explicit StateMachine(const StateConfig& cfg = StateConfig{}) : cfg_(cfg) {}
    State update(bool faulty);           // feed one verdict, returns new state
    State state() const { return state_; }
    State previous() const { return prev_; }
    bool  transitioned() const { return prev_ != state_; }
    void  reset();
private:
    void enter(State s);
    StateConfig cfg_;
    State state_ = State::Init, prev_ = State::Init;
    int healthy_ = 0;
    std::deque<bool> window_;
};

}  // namespace sg
