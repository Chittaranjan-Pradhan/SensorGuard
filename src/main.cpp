// sensorguardd - Smart Sensor Fault Detection daemon.
//
// Threads:   producer (timerfd + poll -> read sensor -> queue)
//            consumer (queue -> Monitor -> CSV + log)
//            main     (signalfd + poll: SIGINT/SIGTERM = stop, SIGUSR1 = dump stats)
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <thread>

#include "sensorguard/bounded_queue.hpp"
#include "sensorguard/logger.hpp"
#include "sensorguard/monitor.hpp"
#include "sensorguard/sources.hpp"

using namespace sg;

namespace {

struct Options {
    std::string device = "/dev/vsensor";
    bool simulate = false;
    bool quiet = false;
    int period_ms = 100;
    uint64_t samples = 0;           // 0 = run until signalled
    std::string log_file, csv_file;
    bool has_inject = false;
    InjectMode mode = InjectMode::None;
    uint64_t inj_start = 0, inj_end = 0;  // inj_end == 0: never cleared
    uint32_t seed = 12345;
};

void usage(const char* prog) {
    std::fprintf(stderr,
        "Usage: %s [options]\n"
        "  -d, --device PATH     sensor device (default /dev/vsensor)\n"
        "  -s, --simulate        use built-in simulator instead of the kernel driver\n"
        "  -p, --period-ms N     sampling period in ms (default 100)\n"
        "  -n, --samples N       stop after N samples (default: run until SIGINT)\n"
        "  -i, --inject SPEC     MODE:START[:END]  e.g. drift:60  or  spike:50:120\n"
        "                        MODE = stuck|spike|drift|noise|dropout\n"
        "  -l, --log FILE        also write log to FILE\n"
        "  -c, --csv FILE        write per-sample CSV to FILE\n"
        "  -q, --quiet           only log warnings/errors and state transitions\n"
        "      --seed N          simulator seed (default 12345)\n"
        "  -h, --help\n"
        "Signals: SIGINT/SIGTERM = graceful stop, SIGUSR1 = print statistics\n"
        "Exit code: 0 = ok, 1 = error, 2 = sensor ended in FAULT state\n", prog);
}

bool parseInject(const std::string& spec, Options& o) {
    const auto p1 = spec.find(':');
    if (p1 == std::string::npos) return false;
    if (!parseInjectMode(spec.substr(0, p1), o.mode) || o.mode == InjectMode::None) return false;
    const auto p2 = spec.find(':', p1 + 1);
    try {
        o.inj_start = std::stoull(spec.substr(p1 + 1, p2 == std::string::npos ? p2 : p2 - p1 - 1));
        o.inj_end = (p2 == std::string::npos) ? 0 : std::stoull(spec.substr(p2 + 1));
    } catch (...) { return false; }
    o.has_inject = true;
    return true;
}

bool parseArgs(int argc, char** argv, Options& o) {
    static const option longopts[] = {
        {"device", required_argument, nullptr, 'd'}, {"simulate", no_argument, nullptr, 's'},
        {"period-ms", required_argument, nullptr, 'p'}, {"samples", required_argument, nullptr, 'n'},
        {"inject", required_argument, nullptr, 'i'}, {"log", required_argument, nullptr, 'l'},
        {"csv", required_argument, nullptr, 'c'}, {"quiet", no_argument, nullptr, 'q'},
        {"seed", required_argument, nullptr, 1000}, {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0}};
    int c;
    while ((c = getopt_long(argc, argv, "d:sp:n:i:l:c:qh", longopts, nullptr)) != -1) {
        switch (c) {
            case 'd': o.device = optarg; break;
            case 's': o.simulate = true; break;
            case 'p': o.period_ms = std::atoi(optarg); break;
            case 'n': o.samples = std::strtoull(optarg, nullptr, 10); break;
            case 'i': if (!parseInject(optarg, o)) { std::fprintf(stderr, "bad --inject spec\n"); return false; } break;
            case 'l': o.log_file = optarg; break;
            case 'c': o.csv_file = optarg; break;
            case 'q': o.quiet = true; break;
            case 1000: o.seed = static_cast<uint32_t>(std::strtoul(optarg, nullptr, 10)); break;
            default: return false;
        }
    }
    return o.period_ms > 0;
}

void printStats(const Stats& s) {
    SG_INFO("---- stats: samples=%llu transitions=%llu state=%s",
            static_cast<unsigned long long>(s.samples),
            static_cast<unsigned long long>(s.transitions), toString(s.state));
    for (std::size_t i = 1; i < kFaultTypeCount; ++i)
        if (s.fault_counts[i])
            SG_INFO("     %-13s %llu", toString(static_cast<FaultType>(i)),
                    static_cast<unsigned long long>(s.fault_counts[i]));
}

// ---- producer: periodic sampling driven by a timerfd -----------------------
void producerThread(ISampleSource& src, BoundedQueue<Sample>& q, const Options& o, int stop_fd) {
    const int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    itimerspec its{};
    its.it_interval.tv_sec = its.it_value.tv_sec = o.period_ms / 1000;
    its.it_interval.tv_nsec = its.it_value.tv_nsec = (o.period_ms % 1000) * 1000000L;
    timerfd_settime(tfd, 0, &its, nullptr);

    pollfd fds[2] = {{tfd, POLLIN, 0}, {stop_fd, POLLIN, 0}};
    uint64_t idx = 0;
    bool running = true;
    while (running) {
        if (poll(fds, 2, -1) < 0) continue;       // EINTR
        if (fds[1].revents & POLLIN) break;
        if (!(fds[0].revents & POLLIN)) continue;

        uint64_t ticks = 0;
        if (::read(tfd, &ticks, sizeof ticks) != sizeof ticks) continue;
        if (ticks > 1) SG_WARN("sampling overrun: %llu ticks missed", static_cast<unsigned long long>(ticks - 1));

        if (o.has_inject && idx == o.inj_start) {
            src.setFault(o.mode);
            SG_INFO("[inject] fault '%s' enabled at sample %llu", toString(o.mode), static_cast<unsigned long long>(idx));
        }
        if (o.has_inject && o.inj_end && idx == o.inj_end) {
            src.setFault(InjectMode::None);
            SG_INFO("[inject] fault cleared at sample %llu", static_cast<unsigned long long>(idx));
        }

        Sample s;
        if (!src.read(s)) { SG_ERROR("sensor read failed"); break; }
        if (!q.push(s)) break;
        ++idx;
        if (o.samples && idx >= o.samples) running = false;
    }
    ::close(tfd);
    q.close();   // lets the consumer drain and exit
}

// ---- consumer: analysis ----------------------------------------------------
void consumerThread(BoundedQueue<Sample>& q, Monitor& mon, const Options& o, std::atomic<bool>& done) {
    std::ofstream csv;
    if (!o.csv_file.empty()) {
        csv.open(o.csv_file);
        csv << "seq,timestamp_ns,value_c,valid,fault,state\n";
    }
    Sample s;
    while (q.pop(s)) {
        const Verdict v = mon.onSample(s);
        if (csv.is_open())
            csv << s.seq << ',' << s.timestamp_ns << ',' << s.value_c << ',' << (s.valid ? 1 : 0)
                << ',' << toString(v.fault) << ',' << toString(v.state) << '\n';
        if (v.transitioned)
            SG_WARN("STATE %s -> %s (seq=%u, fault=%s, value=%.2fC)", toString(v.previous),
                    toString(v.state), s.seq, toString(v.fault), s.value_c);
        else if (!o.quiet && v.fault != FaultType::None)
            SG_INFO("fault detected: %s (seq=%u, value=%.2fC)", toString(v.fault), s.seq, s.value_c);
    }
    csv.flush();
    done = true;
}

}  // namespace

int main(int argc, char** argv) {
    Options opt;
    if (!parseArgs(argc, argv, opt)) { usage(argv[0]); return 1; }

    Logger::instance().setQuiet(opt.quiet);
    if (!opt.log_file.empty() && !Logger::instance().openFile(opt.log_file)) {
        std::fprintf(stderr, "cannot open log file %s\n", opt.log_file.c_str());
        return 1;
    }

    // Block signals BEFORE creating threads so they are inherited, then read via signalfd.
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &mask, nullptr);
    const int sfd = signalfd(-1, &mask, SFD_CLOEXEC);

    std::unique_ptr<ISampleSource> src;
    try {
        if (opt.simulate) src.reset(new SimulatedSource(25.0, opt.seed));
        else              src.reset(new DeviceSource(opt.device));
    } catch (const std::exception& e) {
        SG_ERROR("%s  (try --simulate, or load the driver: scripts/load_driver.sh)", e.what());
        return 1;
    }
    SG_INFO("sensorguardd started: source=%s period=%dms pid=%d", src->describe().c_str(), opt.period_ms, getpid());

    Monitor monitor;
    BoundedQueue<Sample> queue(256);
    std::atomic<bool> done{false};
    const int stop_fd = eventfd(0, EFD_CLOEXEC);

    std::thread prod(producerThread, std::ref(*src), std::ref(queue), std::cref(opt), stop_fd);
    std::thread cons(consumerThread, std::ref(queue), std::ref(monitor), std::cref(opt), std::ref(done));

    pollfd pfd{sfd, POLLIN, 0};
    while (!done.load()) {
        if (poll(&pfd, 1, 100) > 0 && (pfd.revents & POLLIN)) {
            signalfd_siginfo si;
            if (::read(sfd, &si, sizeof si) != sizeof si) continue;
            if (si.ssi_signo == SIGUSR1) { printStats(monitor.stats()); continue; }
            SG_INFO("received signal %u, shutting down", si.ssi_signo);
            break;
        }
    }

    const uint64_t one = 1;
    if (::write(stop_fd, &one, sizeof one) < 0) { /* best effort */ }
    prod.join();
    cons.join();

    const Stats st = monitor.stats();
    printStats(st);
    ::close(sfd);
    ::close(stop_fd);
    return st.state == State::Fault ? 2 : 0;
}
