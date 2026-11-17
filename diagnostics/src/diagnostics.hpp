#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace inputalign {
// These describe observable configuration, never an inferred simulation result.
enum class Reason : std::size_t {
    unavailable, disabled, trajectory_off, practice_only, mode_off, zero_window,
    cbf_sync_off, unknown, count
};
constexpr std::string_view message(Reason r) {
    switch (r) {
        case Reason::unavailable: return "InputAlign ausente o ajustes incompatibles";
        case Reason::disabled: return "Buffer desactivado";
        case Reason::trajectory_off: return "Trayectoria invisible desactivada";
        case Reason::practice_only: return "Solo practica activo fuera de practica";
        case Reason::mode_off: return "Buffer desactivado para este modo";
        case Reason::zero_window: return "Limite de ticks igual a cero";
        case Reason::cbf_sync_off: return "CBF cargado sin sincronizacion";
        case Reason::unknown: return "Sin bloqueo de configuracion; motivo interno no disponible";
        default: return "Estado desconocido";
    }
}
struct Config {
    bool available = false;
    bool enabled = true, trajectory = true, practice_only = false;
    bool normal = true, wave = true, cbf_loaded = false, cbf_sync = true;
    bool spam_guard = false, performance = false;
    std::int64_t normal_ticks = 0, wave_ticks = 0, correction_ms = 0;
};
struct Context { bool practice = false, wave = false; };
// Collect every known blocker, rather than guessing which branch the DLL took.
struct Assessment {
    std::array<Reason, static_cast<std::size_t>(Reason::count)> reasons{};
    std::size_t size = 0;
    void add(Reason r) { if (size < reasons.size()) reasons[size++] = r; }
};
inline Assessment assess(Config const& c, Context const& x) {
    Assessment a;
    if (!c.available) { a.add(Reason::unavailable); return a; }
    if (!c.enabled) a.add(Reason::disabled);
    if (!c.trajectory) a.add(Reason::trajectory_off);
    if (c.practice_only && !x.practice) a.add(Reason::practice_only);
    if (x.wave ? !c.wave : !c.normal) a.add(Reason::mode_off);
    if ((x.wave ? c.wave_ticks : c.normal_ticks) <= 0) a.add(Reason::zero_window);
    if (c.cbf_loaded && !c.cbf_sync) a.add(Reason::cbf_sync_off);
    if (!a.size) a.add(Reason::unknown);
    return a;
}
struct Observation { Assessment assessment; bool press = false, player1 = true; };
class History {
    std::array<Observation, 32> entries{};
    std::size_t next = 0, used = 0;
public:
    void clear() { next = used = 0; }
    void record(Observation value) {
        entries[next] = value;
        next = (next + 1) % entries.size();
        if (used < entries.size()) ++used;
    }
    std::size_t size() const { return used; }
    Observation const* recent(std::size_t index) const {
        if (index >= used) return nullptr;
        return &entries[(next + entries.size() - 1 - index) % entries.size()];
    }
};
}
