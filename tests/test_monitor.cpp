// Integration tests: SimulatedSource -> Monitor (detectors + state machine) end to end.
#include <map>

#include "sensorguard/monitor.hpp"
#include "sensorguard/sources.hpp"
#include "test_framework.hpp"

using namespace sg;

static void run(SimulatedSource& src, Monitor& mon, int n, Verdict* last = nullptr,
                std::map<FaultType, int>* seen = nullptr) {
    for (int i = 0; i < n; ++i) {
        Sample s; src.read(s);
        Verdict v = mon.onSample(s);
        if (last) *last = v;
        if (seen) ++(*seen)[v.fault];
    }
}

TEST(no_false_positives_on_healthy_signal_many_seeds) {
    for (uint32_t seed = 1; seed <= 25; ++seed) {
        SimulatedSource src(25.0, seed);
        Monitor mon;
        run(src, mon, 600);
        Stats st = mon.stats();
        CHECK(st.state == State::Normal);
        CHECK(st.transitions == 1);                 // only INIT -> NORMAL
        CHECK(st.fault_counts[0] == st.samples);
    }
}

static void expectFaultDetected(InjectMode mode, FaultType expected) {
    for (uint32_t seed = 1; seed <= 10; ++seed) {
        SimulatedSource src(25.0, seed);
        Monitor mon;
        run(src, mon, 40);                          // warm-up + a few healthy
        CHECK(mon.stats().state == State::Normal);
        src.setFault(mode);
        std::map<FaultType, int> seen;
        Verdict last;
        run(src, mon, 150, &last, &seen);
        CHECK(seen[expected] > 0);
        CHECK(last.state == State::Fault);
    }
}

TEST(detects_stuck_fault)   { expectFaultDetected(InjectMode::Stuck,   FaultType::StuckAt); }
TEST(detects_spike_fault)   { expectFaultDetected(InjectMode::Spike,   FaultType::Spike); }
TEST(detects_drift_fault)   { expectFaultDetected(InjectMode::Drift,   FaultType::Drift); }
TEST(detects_noise_fault)   { expectFaultDetected(InjectMode::Noise,   FaultType::Noise); }
TEST(detects_dropout_fault) { expectFaultDetected(InjectMode::Dropout, FaultType::Dropout); }

TEST(fault_then_recovery_returns_to_normal) {
    SimulatedSource src;
    Monitor mon;
    run(src, mon, 40);
    src.setFault(InjectMode::Dropout);
    run(src, mon, 10);
    CHECK(mon.stats().state == State::Fault);
    src.setFault(InjectMode::None);
    run(src, mon, 3);
    CHECK(mon.stats().state == State::Recovering);
    run(src, mon, 20);
    CHECK(mon.stats().state == State::Normal);
}

TEST(stats_count_every_sample) {
    SimulatedSource src;
    Monitor mon;
    run(src, mon, 123);
    CHECK(mon.stats().samples == 123);
}
