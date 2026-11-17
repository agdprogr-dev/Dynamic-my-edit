#include "landing_tap.hpp"
#include <cstdlib>
#include <iostream>
using namespace inputalign;
void check(bool c, char const* name) { if (!c) { std::cerr << name << '\n'; std::exit(1); } }
void tap(LandingTap& b, std::uint64_t t=0, int ms=80) {
    b.input(true,true,true,t,ms); b.input(false,true,true,t+1,ms);
}
int main() {
    LandingTap b;
    tap(b);
    check(b.step(20,true,false)==TapAction::none,"wait airborne");
    check(b.step(79,true,true)==TapAction::press,"released tap on landing");
    check(b.step(80,true,false)==TapAction::release,"release after physics even past deadline");
    check(b.step(81,true,true)==TapAction::none,"no repeated jumps");
    b.clear(); tap(b); check(b.step(80,true,true)==TapAction::none,"exact deadline expires");
    check(b.status()==TapStatus::expired,"timeout reason");
    b.clear(); tap(b); check(b.step(20,false,true)==TapAction::none,"context change cancels");
    check(b.step(21,true,true)==TapAction::none,"cancel cannot rearm");
    b.clear(); b.input(true,true,true,0,80);
    check(b.step(20,true,true)==TapAction::none,"held press remains native");
    check(b.status()==TapStatus::native_hold,"hold reason");
    b.clear(); tap(b); check(b.step(20,true,true)==TapAction::press,"new pulse");
    b.input(true,true,false,21,80);
    check(b.step(22,true,true)==TapAction::none,"do not release user's new hold");
    b.clear(); tap(b); b.step(20,true,true);
    check(b.stop()==TapAction::release,"pause flushes synthetic hold");
    check(b.status()==TapStatus::applied,"opening diagnosis preserves last result");
    check(b.stop()==TapAction::none,"flush is idempotent");
    b.clear(); tap(b); b.step(20,true,true); b.clear();
    check(b.step(21,true,true)==TapAction::none,"no stale events after reset/death");
    b.clear(); b.input(true,true,false,0,80); b.input(false,true,false,1,80);
    check(b.step(20,true,true)==TapAction::none,"grounded input not buffered");
    b.clear(); tap(b); b.input(false,true,true,5,80);
    check(b.step(6,true,true)==TapAction::press,"duplicate release does not extend/cancel tap");
    b.clear(); tap(b,50); check(b.step(49,true,true)==TapAction::none,"clock reversal fails closed");
    for (int ms : {-100,0,10,80,150,10000}) {
        b.clear(); tap(b,0,ms);
        auto cap=static_cast<std::uint64_t>(std::clamp(ms,10,150));
        check(b.step(cap,true,true)==TapAction::none,"bounded window");
    }
    b.clear(); tap(b); b.input(true,true,true,30,80); b.input(false,true,true,31,80);
    check(b.step(100,true,true)==TapAction::press,"new tap replaces previous one");
    std::cout << "PASS: landing taps, expiry, cancellation, physical ownership and lifecycle\n";
}
