#pragma once
// Tiny dependency-free test framework (so the project builds anywhere, incl. CI).
#include <iostream>
#include <vector>

namespace tf {
struct Case { const char* name; void (*fn)(); };
inline std::vector<Case>& registry() { static std::vector<Case> r; return r; }
inline int& failures() { static int f = 0; return f; }
struct Reg { Reg(const char* n, void (*f)()) { registry().push_back({n, f}); } };
}  // namespace tf

#define TEST(name) \
    static void name(); \
    static tf::Reg reg_##name(#name, name); \
    static void name()

#define CHECK(cond) \
    do { if (!(cond)) { ++tf::failures(); \
        std::cerr << "  " << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond "\n"; } } while (0)
