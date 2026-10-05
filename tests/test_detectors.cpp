#include "sensorguard/detectors.hpp"
#include "test_framework.hpp"

using namespace sg;

static Sample mk(double v, bool valid = true) { Sample s; s.value_c = v; s.valid = valid; return s; }

TEST(dropout_flags_invalid_samples) {
    DropoutDetector d;
    CHECK(d.evaluate(mk(25.0)) == FaultType::None);
    CHECK(d.evaluate(mk(0.0, false)) == FaultType::Dropout);
}

TEST(range_detects_out_of_range) {
    RangeDetector d(-40, 85);
    CHECK(d.evaluate(mk(25)) == FaultType::None);
    CHECK(d.evaluate(mk(120)) == FaultType::OutOfRange);
    CHECK(d.evaluate(mk(-60)) == FaultType::OutOfRange);
    CHECK(d.evaluate(mk(85)) == FaultType::None);  // boundary is inclusive
}

TEST(stuck_needs_n_identical_values) {
    StuckDetector d(8);
    for (int i = 0; i < 7; ++i) CHECK(d.evaluate(mk(25.0)) == FaultType::None);
    CHECK(d.evaluate(mk(25.0)) == FaultType::StuckAt);   // 8th identical
    CHECK(d.evaluate(mk(25.1)) == FaultType::None);      // changes => cleared
}

TEST(stuck_run_resets_on_change) {
    StuckDetector d(4);
    for (int i = 0; i < 3; ++i) d.evaluate(mk(10.0));
    d.evaluate(mk(11.0));
    for (int i = 0; i < 2; ++i) CHECK(d.evaluate(mk(11.0)) == FaultType::None);
    CHECK(d.evaluate(mk(11.0)) == FaultType::StuckAt);
}

TEST(spike_detected_and_not_added_to_window) {
    SpikeDetector d(5.0, 5);
    for (int i = 0; i < 5; ++i) CHECK(d.evaluate(mk(25.0 + 0.1 * i)) == FaultType::None);
    CHECK(d.evaluate(mk(50.0)) == FaultType::Spike);
    CHECK(d.evaluate(mk(25.2)) == FaultType::None);      // window not polluted by the spike
    CHECK(d.evaluate(mk(50.0)) == FaultType::Spike);
}

TEST(spike_ignores_small_variation) {
    SpikeDetector d(5.0, 5);
    for (int i = 0; i < 50; ++i) CHECK(d.evaluate(mk(25.0 + ((i % 2) ? 0.2 : -0.2))) == FaultType::None);
}

TEST(drift_detects_slow_ramp) {
    DriftDetector d(20, 10, 3.0);
    for (int i = 0; i < 20; ++i) CHECK(d.evaluate(mk(25.0)) == FaultType::None);   // baseline
    bool detected = false;
    for (int i = 0; i < 100 && !detected; ++i)
        detected = d.evaluate(mk(25.0 + 0.1 * (i + 1))) == FaultType::Drift;
    CHECK(detected);
}

TEST(drift_ignores_small_noise) {
    DriftDetector d(20, 10, 3.0);
    for (int i = 0; i < 200; ++i)
        CHECK(d.evaluate(mk(25.0 + ((i % 2) ? 0.2 : -0.2))) == FaultType::None);
}

TEST(noise_detects_high_variance) {
    NoiseDetector d(10, 1.0);
    FaultType last = FaultType::None;
    for (int i = 0; i < 12; ++i) last = d.evaluate(mk((i % 2) ? 30.0 : 20.0));
    CHECK(last == FaultType::Noise);
}

TEST(noise_ignores_quiet_signal) {
    NoiseDetector d(10, 1.0);
    for (int i = 0; i < 50; ++i) CHECK(d.evaluate(mk(25.0 + 0.05 * (i % 3))) == FaultType::None);
}

TEST(engine_priority_dropout_first) {
    FaultEngine e;
    CHECK(e.process(mk(999.0, false)) == FaultType::Dropout);  // would also be out-of-range
    CHECK(e.process(mk(999.0, true)) == FaultType::OutOfRange);
}

TEST(engine_reset_clears_history) {
    FaultEngine e;
    for (int i = 0; i < 7; ++i) e.process(mk(25.0));
    e.reset();
    for (int i = 0; i < 7; ++i) CHECK(e.process(mk(25.0)) == FaultType::None);
}
