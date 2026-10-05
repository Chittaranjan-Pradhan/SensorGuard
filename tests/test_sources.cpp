#include <algorithm>
#include <cmath>

#include "sensorguard/sources.hpp"
#include "test_framework.hpp"

using namespace sg;

static std::vector<Sample> grab(SimulatedSource& s, int n) {
    std::vector<Sample> v(n);
    for (auto& x : v) s.read(x);
    return v;
}

TEST(sim_healthy_values_within_noise_band) {
    SimulatedSource s;
    for (const auto& x : grab(s, 500)) { CHECK(x.valid); CHECK(std::fabs(x.value_c - 25.0) <= 0.2001); }
}

TEST(sim_is_deterministic_for_same_seed) {
    SimulatedSource a(25.0, 7), b(25.0, 7);
    for (int i = 0; i < 100; ++i) { Sample x, y; a.read(x); b.read(y); CHECK(x.value_c == y.value_c); }
}

TEST(sim_sequence_numbers_increase) {
    SimulatedSource s;
    auto v = grab(s, 10);
    for (int i = 0; i < 10; ++i) CHECK(v[i].seq == static_cast<uint32_t>(i));
}

TEST(sim_stuck_freezes_value) {
    SimulatedSource s;
    grab(s, 10);
    s.setFault(InjectMode::Stuck);
    auto v = grab(s, 20);
    for (const auto& x : v) CHECK(x.value_c == v[0].value_c);
}

TEST(sim_dropout_marks_invalid) {
    SimulatedSource s;
    s.setFault(InjectMode::Dropout);
    for (const auto& x : grab(s, 5)) CHECK(!x.valid);
}

TEST(sim_spike_every_third_sample) {
    SimulatedSource s;
    s.setFault(InjectMode::Spike);
    auto v = grab(s, 9);
    for (int i = 0; i < 9; ++i) CHECK((v[i].value_c > 40.0) == (i % 3 == 2));
}

TEST(sim_drift_grows_monotonically_on_average) {
    SimulatedSource s;
    s.setFault(InjectMode::Drift);
    auto v = grab(s, 50);
    CHECK(v[49].value_c - 25.0 > 4.5);
    CHECK(v[49].value_c > v[10].value_c);
}

TEST(sim_noise_has_large_amplitude) {
    SimulatedSource s;
    s.setFault(InjectMode::Noise);
    auto v = grab(s, 200);
    double mx = 0;
    for (const auto& x : v) mx = std::max(mx, std::fabs(x.value_c - 25.0));
    CHECK(mx > 2.0 && mx <= 4.0001);
}

TEST(sim_clear_fault_restores_normal) {
    SimulatedSource s;
    s.setFault(InjectMode::Spike);
    grab(s, 10);
    s.setFault(InjectMode::None);
    for (const auto& x : grab(s, 30)) CHECK(std::fabs(x.value_c - 25.0) <= 0.2001);
}

TEST(inject_mode_parsing) {
    InjectMode m;
    CHECK(parseInjectMode("drift", m) && m == InjectMode::Drift);
    CHECK(parseInjectMode("none", m) && m == InjectMode::None);
    CHECK(!parseInjectMode("bogus", m));
}

TEST(device_source_missing_node_throws) {
    bool threw = false;
    try { DeviceSource d("/dev/does_not_exist_sensor"); } catch (const std::exception&) { threw = true; }
    CHECK(threw);
}
