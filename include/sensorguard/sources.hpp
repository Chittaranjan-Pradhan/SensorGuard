#pragma once
// Sample sources: the real kernel device, or a bit-exact user-space simulator.
#include <cstdint>
#include <string>

#include "sensorguard/types.hpp"

namespace sg {

class ISampleSource {
public:
    virtual ~ISampleSource() = default;
    virtual bool read(Sample& out) = 0;
    virtual bool setFault(InjectMode mode) = 0;
    virtual std::string describe() const = 0;
};

// Talks to /dev/vsensor using read(2) and ioctl(2). Throws std::runtime_error on open failure.
class DeviceSource final : public ISampleSource {
public:
    explicit DeviceSource(const std::string& path);
    ~DeviceSource() override;
    DeviceSource(const DeviceSource&) = delete;
    DeviceSource& operator=(const DeviceSource&) = delete;
    bool read(Sample& out) override;
    bool setFault(InjectMode mode) override;
    bool reset();
    bool getFault(InjectMode& mode);
    std::string describe() const override { return "device:" + path_; }
private:
    std::string path_;
    int fd_ = -1;
};

// Mirrors driver/vsensor.c so the daemon and tests run without the kernel module.
class SimulatedSource final : public ISampleSource {
public:
    explicit SimulatedSource(double base_c = 25.0, uint32_t seed = 12345);
    bool read(Sample& out) override;
    bool setFault(InjectMode mode) override;
    std::string describe() const override { return "simulated"; }
private:
    int32_t noise(int32_t amp);
    int32_t base_mc_;
    uint32_t rng_, seq_ = 0, fault_seq_ = 0;
    InjectMode fault_ = InjectMode::None;
    int32_t last_good_mc_;
};

}  // namespace sg
