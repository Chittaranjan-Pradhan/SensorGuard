// sgctl - tiny control tool for /dev/vsensor (fault injection via ioctl, raw reads).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>

#include "sensorguard/sources.hpp"

using namespace sg;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr,
            "Usage: %s <device> get | set <mode> | reset | read [count]\n"
            "  mode: none|stuck|spike|drift|noise|dropout\n", argv[0]);
        return 1;
    }
    try {
        DeviceSource dev(argv[1]);
        const std::string cmd = argv[2];
        if (cmd == "get") {
            InjectMode m;
            if (!dev.getFault(m)) { std::perror("ioctl GET_FAULT"); return 1; }
            std::printf("%s\n", toString(m));
        } else if (cmd == "set" && argc >= 4) {
            InjectMode m;
            if (!parseInjectMode(argv[3], m)) { std::fprintf(stderr, "unknown mode '%s'\n", argv[3]); return 1; }
            if (!dev.setFault(m)) { std::perror("ioctl SET_FAULT"); return 1; }
            std::printf("fault mode set to %s\n", toString(m));
        } else if (cmd == "reset") {
            if (!dev.reset()) { std::perror("ioctl RESET"); return 1; }
            std::printf("sensor reset\n");
        } else if (cmd == "read") {
            const int n = argc >= 4 ? std::atoi(argv[3]) : 1;
            for (int i = 0; i < n; ++i) {
                Sample s;
                if (!dev.read(s)) { std::perror("read"); return 1; }
                std::printf("seq=%u value=%.3fC valid=%d\n", s.seq, s.value_c, s.valid ? 1 : 0);
            }
        } else {
            std::fprintf(stderr, "bad command\n");
            return 1;
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    return 0;
}
