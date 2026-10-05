#include "test_framework.hpp"

int main() {
    int failed_cases = 0;
    for (const auto& c : tf::registry()) {
        const int before = tf::failures();
        c.fn();
        const bool ok = tf::failures() == before;
        std::cout << (ok ? "[ PASS ] " : "[ FAIL ] ") << c.name << "\n";
        if (!ok) ++failed_cases;
    }
    std::cout << "\n" << (tf::registry().size() - failed_cases) << "/" << tf::registry().size() << " tests passed\n";
    return failed_cases ? 1 : 0;
}
