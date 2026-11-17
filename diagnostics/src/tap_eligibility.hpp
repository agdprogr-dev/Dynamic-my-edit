#pragma once
#include <string_view>
namespace inputalign {
enum class TapBlock { none, failure, disabled, no_player, practice, cbf, schema, original_normal, mode, contact };
struct TapEnvironment {
    bool failed = false, enabled = false, alive = false, practice = false, paused = false;
    bool cbf = false, original_present = false, original_schema = true;
    bool original_enabled = false, original_normal = true;
    bool unsupported_mode = false, unsafe_contact = false;
};
// Pure policy shared by the adapter and tests. Wave alone never touches cube input.
constexpr TapBlock eligibility(TapEnvironment const& e) {
    if (e.failed) return TapBlock::failure;
    if (!e.enabled) return TapBlock::disabled;
    if (!e.alive) return TapBlock::no_player;
    if (!e.practice || e.paused) return TapBlock::practice;
    if (e.cbf) return TapBlock::cbf;
    if (e.original_present) {
        if (!e.original_schema) return TapBlock::schema;
        if (e.original_enabled && e.original_normal) return TapBlock::original_normal;
    }
    if (e.unsupported_mode) return TapBlock::mode;
    if (e.unsafe_contact) return TapBlock::contact;
    return TapBlock::none;
}
constexpr std::string_view blockMessage(TapBlock b) {
    switch (b) {
        case TapBlock::failure: return "Ayuda suspendida por error";
        case TapBlock::disabled: return "Ayuda de salto apagada";
        case TapBlock::no_player: return "Sin jugador vivo";
        case TapBlock::practice: return "Requiere practica sin pausa";
        case TapBlock::cbf: return "CBF cargado: ayuda suspendida";
        case TapBlock::schema: return "Ajustes del corrector original incompatibles";
        case TapBlock::original_normal: return "Apaga Buffer in cube and others del original";
        case TapBlock::mode: return "Solo cubo normal sin dual, plataforma, giro ni dash";
        case TapBlock::contact: return "Pendiente, portal, pad u orbe detectado";
        default: return "Disponible (solo taps del jugador 1)";
    }
}
}
