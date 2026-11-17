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
    check(b.step(79,true,true)==TapAction::none,"repeated collision does not release pulse");
    check(b.afterPhysics()==TapAction::release,"release after next physics update");
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
    check(b.afterPhysics()==TapAction::none,"do not release user's new hold");
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
    b.clear(); tap(b); b.step(20,true,true); b.step(21,true,true);
    check(b.stats().requested==1 && b.stats().last_delay_ms==20,"requests count once with actual delay");
    b.resetAttempt();
    check(b.stats().requested==1 && b.status()==TapStatus::applied,"retry preserves level summary");
    check(b.afterPhysics()==TapAction::none,"retry discards stale pulse");
    tap(b,100); b.step(180,true,false); b.step(181,true,false);
    check(b.stats().expired==1,"expiry counted once");
    tap(b,200); b.step(210,false,false); b.step(211,false,false);
    check(b.stats().cancelled==1,"context cancellation counted once");
    tap(b,300); b.resetAttempt(); b.resetAttempt();
    check(b.stats().cancelled==2,"death and retry do not double count cancellation");
    b.input(true,true,true,400,80); b.step(410,true,true); b.step(411,true,true);
    check(b.stats().native_holds==1,"native hold counted once");
    b.input(false,true,false,412,80); tap(b,500); b.input(true,true,true,510,80);
    check(b.stats().cancelled==3,"replacing a pending tap counts one cancellation");
    b.clear(); check(b.stats().requested==0 && b.stats().cancelled==0,"new level clears stats");
    std::uint32_t counter = std::numeric_limits<std::uint32_t>::max()-1;
    incrementCounter(counter); incrementCounter(counter);
    check(counter==std::numeric_limits<std::uint32_t>::max(),"counters saturate");
    b.clear(); check(b.recent(0)==nullptr,"empty outcome history");
    tap(b); b.step(20,true,true); b.afterPhysics();
    check(b.recent(0)->status==TapStatus::applied && b.recent(0)->delay_ms==20,"requested delay history");
    b.resetAttempt(); check(b.historySize()==1,"attempt reset preserves history");
    tap(b,100); b.stop();
    check(b.recent(0)->cause==TapCancel::pause,"pause cancellation cause");
    b.resetAttempt(); tap(b,200); b.resetAttempt();
    check(b.recent(0)->cause==TapCancel::reset,"death/reset cause");
    tap(b,300); b.step(310,false,false);
    check(b.recent(0)->cause==TapCancel::context,"eligibility cause");
    b.resetAttempt(); tap(b,400); b.stop(TapCancel::error);
    check(b.recent(0)->cause==TapCancel::error,"error cause");
    b.resetAttempt(); tap(b,500); b.input(true,true,true,510,80);
    check(b.recent(0)->cause==TapCancel::replaced,"replacement cause");
    b.clear();
    for (unsigned i=0; i<40; ++i) { tap(b,i*100); b.step(i*100+80,true,false); }
    check(b.historySize()==16 && !b.recent(16),"bounded history");
    check(b.recent(15)->status==TapStatus::expired,"history retains valid oldest entry");
    b.resetAttempt(); tap(b,5000); b.step(5012,true,true); b.afterPhysics();
    check(b.recent(0)->delay_ms==12 && b.recent(1)->status==TapStatus::expired,"newest-first ring order");
    b.clear(); check(b.historySize()==0 && !b.recent(0),"level exit clears history");
    check(nextTapWindow(10)==40 && nextTapWindow(40)==80 && nextTapWindow(75)==80 &&
          nextTapWindow(80)==120 && nextTapWindow(120)==150 && nextTapWindow(150)==40 &&
          nextTapWindow(std::numeric_limits<std::int64_t>::max())==40,"window presets and custom setting");
    std::cout << "PASS: landing taps, expiry, cancellation, physical ownership and lifecycle\n";
}
