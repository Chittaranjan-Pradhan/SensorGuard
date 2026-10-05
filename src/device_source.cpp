#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>

#include "sensorguard/sources.hpp"
#include "vsensor_ioctl.h"   // shared kernel/user ABI

namespace sg {

// The user-space enum must stay in sync with the kernel enum.
static_assert(static_cast<uint32_t>(InjectMode::None)    == VS_FAULT_NONE,    "ABI mismatch");
static_assert(static_cast<uint32_t>(InjectMode::Stuck)   == VS_FAULT_STUCK,   "ABI mismatch");
static_assert(static_cast<uint32_t>(InjectMode::Spike)   == VS_FAULT_SPIKE,   "ABI mismatch");
static_assert(static_cast<uint32_t>(InjectMode::Drift)   == VS_FAULT_DRIFT,   "ABI mismatch");
static_assert(static_cast<uint32_t>(InjectMode::Noise)   == VS_FAULT_NOISE,   "ABI mismatch");
static_assert(static_cast<uint32_t>(InjectMode::Dropout) == VS_FAULT_DROPOUT, "ABI mismatch");

DeviceSource::DeviceSource(const std::string& path) : path_(path) {
    fd_ = ::open(path.c_str(), O_RDWR | O_CLOEXEC);
    if (fd_ < 0) throw std::runtime_error("cannot open " + path + ": " + std::strerror(errno));
}

DeviceSource::~DeviceSource() {
    if (fd_ >= 0) ::close(fd_);
}

bool DeviceSource::read(Sample& out) {
    vsensor_sample raw{};
    ssize_t n;
    do { n = ::read(fd_, &raw, sizeof raw); } while (n < 0 && errno == EINTR);
    if (n != static_cast<ssize_t>(sizeof raw)) return false;
    out.timestamp_ns = raw.timestamp_ns;
    out.value_c = raw.value_mC / 1000.0;
    out.seq = raw.seq;
    out.valid = (raw.flags & VS_FLAG_VALID) != 0;
    return true;
}

bool DeviceSource::setFault(InjectMode mode) {
    __u32 v = static_cast<__u32>(mode);
    return ::ioctl(fd_, VSENSOR_IOC_SET_FAULT, &v) == 0;
}

bool DeviceSource::getFault(InjectMode& mode) {
    __u32 v = 0;
    if (::ioctl(fd_, VSENSOR_IOC_GET_FAULT, &v) != 0) return false;
    mode = static_cast<InjectMode>(v);
    return true;
}

bool DeviceSource::reset() {
    return ::ioctl(fd_, VSENSOR_IOC_RESET) == 0;
}

}  // namespace sg
