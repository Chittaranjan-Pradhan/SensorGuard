#include "sensorguard/state_machine.hpp"
#include "test_framework.hpp"

using namespace sg;

static void feed(StateMachine& sm, bool faulty, int n) { for (int i = 0; i < n; ++i) sm.update(faulty); }

TEST(sm_starts_in_init_and_warms_up) {
    StateMachine sm;
    CHECK(sm.state() == State::Init);
    feed(sm, false, 19);
    CHECK(sm.state() == State::Init);
    sm.update(false);
    CHECK(sm.state() == State::Normal);
    CHECK(sm.transitioned());
}

TEST(sm_fault_during_init_restarts_warmup) {
    StateMachine sm;
    feed(sm, false, 15);
    sm.update(true);
    feed(sm, false, 19);
    CHECK(sm.state() == State::Init);
    sm.update(false);
    CHECK(sm.state() == State::Normal);
}

TEST(sm_isolated_fault_returns_to_normal) {
    StateMachine sm;
    feed(sm, false, 20);
    sm.update(true);
    CHECK(sm.state() == State::Suspect);
    feed(sm, false, 5);
    CHECK(sm.state() == State::Normal);
}

TEST(sm_repeated_faults_confirm_fault) {
    StateMachine sm;
    feed(sm, false, 20);
    feed(sm, true, 3);
    CHECK(sm.state() == State::Fault);
}

TEST(sm_every_third_sample_faulty_still_confirms) {   // the driver's spike pattern
    StateMachine sm;
    feed(sm, false, 20);
    for (int i = 0; i < 12; ++i) sm.update(i % 3 == 0);
    CHECK(sm.state() == State::Fault);
}

TEST(sm_full_recovery_path) {
    StateMachine sm;
    feed(sm, false, 20);
    feed(sm, true, 3);
    CHECK(sm.state() == State::Fault);
    feed(sm, false, 3);
    CHECK(sm.state() == State::Recovering);
    feed(sm, false, 7);                      // 10 healthy in total
    CHECK(sm.state() == State::Normal);
}

TEST(sm_fault_during_recovery_returns_to_fault) {
    StateMachine sm;
    feed(sm, false, 20);
    feed(sm, true, 3);
    feed(sm, false, 3);
    CHECK(sm.state() == State::Recovering);
    sm.update(true);
    CHECK(sm.state() == State::Fault);
}

TEST(sm_reset) {
    StateMachine sm;
    feed(sm, false, 20);
    sm.reset();
    CHECK(sm.state() == State::Init);
}
