#pragma once
#include <algorithm>
#include <cstdint>
#include <string_view>
#include <limits>
#include <array>
#include <cstddef>
namespace inputalign {
enum class TapAction { none, press, release };
enum class TapCancel { none, context, replaced, pause, reset, error };
constexpr std::string_view cancelMessage(TapCancel c) {
    switch (c) {
        case TapCancel::context: return "contexto incompatible";
        case TapCancel::replaced: return "reemplazado por otro tap";
        case TapCancel::pause: return "pausa";
        case TapCancel::reset: return "muerte o reinicio";
        case TapCancel::error: return "error de asistencia";
        default: return "sin cancelacion";
    }
}
constexpr int nextTapWindow(std::int64_t current) {
    for (int value : {40, 80, 120, 150}) if (current < value) return value;
    return 40;
}
enum class TapStatus { idle, armed, applied, expired, cancelled, native_hold };
constexpr std::string_view tapMessage(TapStatus s) {
    switch (s) {
        case TapStatus::armed: return "Tap pendiente de aterrizaje";
        case TapStatus::applied: return "Tap aplicado al aterrizar";
        case TapStatus::expired: return "Ventana agotada sin aterrizaje";
        case TapStatus::cancelled: return "Tap cancelado antes de aplicarse";
        case TapStatus::native_hold: return "Pulsacion mantenida: control del juego";
        default: return "Sin tap pendiente";
    }
}
struct TapEvent {
    TapStatus status = TapStatus::idle;
    TapCancel cause = TapCancel::none;
    std::uint64_t delay_ms = 0; // Present only for a requested press, not a claimed save.
};
struct TapStats {
    std::uint32_t requested = 0, expired = 0, cancelled = 0, native_holds = 0;
    std::uint64_t last_delay_ms = 0;
};
inline void incrementCounter(std::uint32_t& n) {
    if (n < std::numeric_limits<std::uint32_t>::max()) ++n;
}
// Original inputs are always forwarded by the host. Only a released tap is retried.
class LandingTap {
    bool down = false, armed = false, pulse = false;
    std::uint64_t started = 0, window = 80;
    TapStatus last = TapStatus::idle;
    TapStats totals;
    std::array<TapEvent, 16> outcomes{};
    std::size_t next = 0, used = 0;
    void record(TapStatus state, TapCancel cause = TapCancel::none, std::uint64_t delay = 0) {
        last = state;
        outcomes[next] = {state, cause, delay};
        next = (next + 1) % outcomes.size();
        if (used < outcomes.size()) ++used;
    }
    void cancelPending(TapCancel cause = TapCancel::context) {
        if (armed) { incrementCounter(totals.cancelled); record(TapStatus::cancelled, cause); }
        armed = false;
    }
public:
    void clear() { *this = LandingTap{}; }
    TapStatus status() const { return last; }
    TapStats const& stats() const { return totals; }
    std::size_t historySize() const { return used; }
    TapEvent const* recent(std::size_t index) const {
        if (index >= used) return nullptr;
        return &outcomes[(next + outcomes.size() - 1 - index) % outcomes.size()];
    }
    // Called only where the game resets the player's own input state (death/retry).
    void resetAttempt() {
        cancelPending(TapCancel::reset);
        down = pulse = false;
        started = 0;
    }
    void input(bool press, bool eligible, bool falling, std::uint64_t now, int ms) {
        // An actual press takes ownership; an actual release ends our pulse too.
        pulse = false;
        if (!eligible) { cancelPending(); down = press; return; }
        if (press && !down) {
            cancelPending(TapCancel::replaced);
            armed = falling;
            started = now;
            window = static_cast<std::uint64_t>(std::clamp(ms, 10, 150));
            last = armed ? TapStatus::armed : TapStatus::idle;
        }
        down = press;
    }
    TapAction stop(TapCancel cause = TapCancel::pause) {
        bool release = pulse && !down;
        cancelPending(cause);
        pulse = false;
        return release ? TapAction::release : TapAction::none;
    }
    TapAction afterPhysics() {
        if (!pulse) return TapAction::none;
        pulse = false;
        return down ? TapAction::none : TapAction::release;
    }
    TapAction step(std::uint64_t now, bool eligible, bool grounded) {
        // Collision observations may repeat before the next physics step.
        if (pulse) return TapAction::none;
        if (!eligible) { cancelPending(); return TapAction::none; }
        if (!armed) return TapAction::none;
        if (now < started || now - started >= window) {
            armed = false; record(TapStatus::expired);
            incrementCounter(totals.expired); return TapAction::none;
        }
        if (!grounded) return TapAction::none;
        armed = false;
        if (down) {
            record(TapStatus::native_hold); incrementCounter(totals.native_holds);
            return TapAction::none;
        }
        pulse = true; record(TapStatus::applied, TapCancel::none, now - started);
        incrementCounter(totals.requested); totals.last_delay_ms = now - started;
        return TapAction::press;
    }
};
}
