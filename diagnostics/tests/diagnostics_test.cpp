#include "diagnostics.hpp"
#include <cstdlib>
#include <iostream>
using namespace inputalign;
void check(bool condition, char const* label) {
    if (!condition) { std::cerr << label << '\n'; std::exit(1); }
}
bool has(Assessment const& a, Reason r) {
    for (std::size_t i = 0; i < a.size; ++i) if (a.reasons[i] == r) return true;
    return false;
}
int main() {
    Config c;
    check(has(assess(c, {}), Reason::unavailable), "missing engine");
    c.available = true; c.normal_ticks = c.wave_ticks = 80;
    check(has(assess(c, {}), Reason::unknown), "never invent a simulation verdict");
    c.enabled = false; c.trajectory = false; c.practice_only = true;
    c.normal = false; c.normal_ticks = 0; c.cbf_loaded = true; c.cbf_sync = false;
    auto a = assess(c, {});
    check(a.size == 6, "report simultaneous blockers");
    for (auto r : {Reason::disabled, Reason::trajectory_off, Reason::practice_only,
                   Reason::mode_off, Reason::zero_window, Reason::cbf_sync_off})
        check(has(a, r), "all blockers present");
    check(!has(assess(c, {true, true}), Reason::practice_only), "practice bypass");
    check(!has(assess(c, {true, true}), Reason::mode_off), "independent wave setting");
    check(!has(assess(c, {true, true}), Reason::zero_window), "independent wave window");
    c.wave_ticks = 0; c.wave = false;
    check(has(assess(c, {true, true}), Reason::zero_window), "wave zero");
    check(has(assess(c, {true, true}), Reason::mode_off), "wave disabled");
    c = Config{}; c.available = true; c.normal_ticks = c.wave_ticks = 80;
    c.spam_guard = true; c.performance = true; c.correction_ms = 1;
    check(has(assess(c, {}), Reason::unknown), "budget and spam flags are not runtime outcomes");
    History h;
    check(!h.recent(0), "empty history");
    for (int i = 0; i < 100; ++i) h.record({assess(c, {}), i % 2 == 0, true});
    check(h.size() == 32 && !h.recent(32), "bounded storage");
    check(!h.recent(0)->press && h.recent(1)->press && h.recent(31)->press, "ring order");
    h.clear(); check(h.size() == 0 && !h.recent(0), "reset between attempts");
    for (std::size_t i = 0; i < static_cast<std::size_t>(Reason::count); ++i)
        check(!message(static_cast<Reason>(i)).empty(), "every reason has text");
    std::cout << "PASS: blockers, unknown outcomes, ring bounds and reset\n";
}
