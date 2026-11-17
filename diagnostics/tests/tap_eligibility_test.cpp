#include "tap_eligibility.hpp"
#include <cstdlib>
#include <iostream>
using namespace inputalign;
void check(bool c, char const* name) { if (!c) { std::cerr << name << '\n'; std::exit(1); } }
TapEnvironment ready() { TapEnvironment e; e.enabled = e.alive = e.practice = true; return e; }
int main() {
    auto e = ready();
    check(eligibility(e)==TapBlock::none,"standalone allowed");
    e.original_present = e.original_enabled = true;
    check(eligibility(e)==TapBlock::original_normal,"no double cube correction");
    e.original_normal = false;
    check(eligibility(e)==TapBlock::none,"original wave can stay active when cube is off");
    e.original_schema = false;
    check(eligibility(e)==TapBlock::schema,"unknown enabled mod schema blocks");
    e = ready(); e.original_present = true;
    check(eligibility(e)==TapBlock::none,"disabled original allowed");
    struct Case { bool TapEnvironment::*field; TapBlock reason; };
    for (auto c : {Case{&TapEnvironment::failed,TapBlock::failure},
                   Case{&TapEnvironment::paused,TapBlock::practice},
                   Case{&TapEnvironment::cbf,TapBlock::cbf},
                   Case{&TapEnvironment::unsupported_mode,TapBlock::mode},
                   Case{&TapEnvironment::unsafe_contact,TapBlock::contact}}) {
        e=ready(); e.*(c.field)=true;
        check(eligibility(e)==c.reason,"negative gate");
        check(!blockMessage(c.reason).empty(),"reason text");
    }
    e=ready(); e.enabled=false; check(eligibility(e)==TapBlock::disabled,"opt in");
    e=ready(); e.alive=false; check(eligibility(e)==TapBlock::no_player,"dead/missing player");
    e=ready(); e.practice=false; check(eligibility(e)==TapBlock::practice,"normal runs unchanged");
    // Exhaustive safety boundary over gate flags: none never means a conflicting engine.
    for (unsigned bits=0; bits<4096; ++bits) {
        e={bool(bits&1),bool(bits&2),bool(bits&4),bool(bits&8),bool(bits&16),
           bool(bits&32),bool(bits&64),bool(bits&128),bool(bits&256),bool(bits&512),
           bool(bits&1024),bool(bits&2048)};
        if (eligibility(e)==TapBlock::none)
            check(!e.failed && e.enabled && e.alive && e.practice && !e.paused && !e.cbf &&
                !e.unsupported_mode && !e.unsafe_contact && (!e.original_present ||
                (e.original_schema && !(e.original_enabled && e.original_normal))),"gate invariant");
    }
    std::cout << "PASS: compatibility, schema and gate safety combinations\n";
}
