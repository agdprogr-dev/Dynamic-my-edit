#pragma once
#include <algorithm>
#include <cstdint>
#include <string_view>
namespace inputalign {
enum class TapAction { none, press, release };
enum class TapStatus { idle, armed, applied, expired, cancelled, native_hold };
constexpr std::string_view tapMessage(TapStatus s) {
    switch (s) {
        case TapStatus::armed: return "Tap pendiente de aterrizaje";
        case TapStatus::applied: return "Tap aplicado al aterrizar";
        case TapStatus::expired: return "Ventana agotada sin aterrizaje";
        case TapStatus::cancelled: return "Tap cancelado: contexto no compatible";
        case TapStatus::native_hold: return "Pulsacion mantenida: control del juego";
        default: return "Sin tap pendiente";
    }
}
// Original inputs are always forwarded by the host. Only a released tap is retried.
class LandingTap {
    bool down = false, armed = false, pulse = false;
    std::uint64_t started = 0, window = 80;
    TapStatus last = TapStatus::idle;
public:
    void clear() { *this = LandingTap{}; }
    TapStatus status() const { return last; }
    void input(bool press, bool eligible, bool falling, std::uint64_t now, int ms) {
        // An actual press takes ownership; an actual release ends our pulse too.
        pulse = false;
        if (!eligible) { armed = false; down = press; last = TapStatus::cancelled; return; }
        if (press && !down) {
            armed = falling;
            started = now;
            window = static_cast<std::uint64_t>(std::clamp(ms, 10, 150));
            last = armed ? TapStatus::armed : TapStatus::idle;
        }
        down = press;
    }
    TapAction stop() {
        bool release = pulse && !down;
        if (armed) last = TapStatus::cancelled;
        armed = pulse = false;
        return release ? TapAction::release : TapAction::none;
    }
    TapAction step(std::uint64_t now, bool eligible, bool grounded) {
        // Called AFTER one real player physics update. Never release a real hold.
        if (pulse) { pulse = false; return down ? TapAction::none : TapAction::release; }
        if (!eligible) { if (armed) last = TapStatus::cancelled; armed = false; return TapAction::none; }
        if (!armed) return TapAction::none;
        if (now < started || now - started >= window) {
            armed = false; last = TapStatus::expired; return TapAction::none;
        }
        if (!grounded) return TapAction::none;
        armed = false;
        if (down) { last = TapStatus::native_hold; return TapAction::none; }
        pulse = true; last = TapStatus::applied; return TapAction::press;
    }
};
}
