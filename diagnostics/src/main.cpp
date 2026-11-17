#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include "diagnostics.hpp"
#include "landing_tap.hpp"
#include "tap_eligibility.hpp"
#include <chrono>
#include <cmath>
#include <string>
using namespace geode::prelude;
namespace {
inputalign::History history;
bool captureFailed = false;
inputalign::LandingTap landingTap;
bool sendingTap = false, assistFailed = false;
std::uint64_t nowMs() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
void sendTap(PlayLayer* pl, inputalign::TapAction action) {
    if (!pl || action == inputalign::TapAction::none) return;
    struct Guard { Guard() { sendingTap = true; } ~Guard() { sendingTap = false; } } guard;
    pl->handleButton(action == inputalign::TapAction::press, 1, true);
}

template <typename T>
bool read(Mod* mod, char const* key, T& value) {
    using Setting = typename SettingTypeForValueType<T>::SettingType;
    if (!cast::typeinfo_pointer_cast<Setting>(mod->getSetting(key))) return false;
    value = mod->getSettingValue<T>(key);
    return true;
}
// Explicitly conservative: never run two correctors on the same input stream.
inputalign::TapBlock tapBlocker(PlayLayer* pl, PlayerObject* p, bool ignorePause = false) {
    inputalign::TapEnvironment e;
    e.failed = assistFailed;
    e.enabled = Mod::get()->getSettingValue<bool>("landing-tap");
    e.alive = pl && p && !p->m_isDead;
    e.practice = pl && pl->m_isPracticeMode;
    e.paused = pl && pl->m_isPaused && !ignorePause;
    e.cbf = e.enabled && e.alive && e.practice && !e.paused &&
        Loader::get()->isModLoaded("syzzi.click_between_frames");
    auto basicBlock = inputalign::eligibility(e);
    if (basicBlock != inputalign::TapBlock::none) return basicBlock;
    if (auto* original = Loader::get()->getLoadedMod("ai.predictivebuffer")) {
        e.original_present = true;
        e.original_schema = read(original, "enabled", e.original_enabled);
        if (e.original_schema && e.original_enabled)
            e.original_schema = read(original, "buffer-normal", e.original_normal);
    }
    if (pl && p) {
        e.unsupported_mode = pl->m_gameState.m_isDualMode || p->m_isPlatformer ||
            p->m_isShip || p->m_isBird || p->m_isBall || p->m_isDart || p->m_isRobot ||
            p->m_isSpider || p->m_isSwing || p->m_isUpsideDown || p->m_isSideways || p->m_isDashing;
        e.unsafe_contact = p->m_isOnSlope || p->m_touchedRing || p->m_touchedCustomRing ||
            p->m_touchedPad || p->m_touchedGravityPortal;
    }
    return inputalign::eligibility(e);
}
inputalign::Config config() {
    inputalign::Config c;
    auto* mod = Loader::get()->getLoadedMod("ai.predictivebuffer");
    if (!mod) return c;
    c.available = read(mod, "enabled", c.enabled)
        && read(mod, "invisible-trajectory", c.trajectory)
        && read(mod, "buffer-practice-only", c.practice_only)
        && read(mod, "buffer-normal", c.normal)
        && read(mod, "buffer-wave", c.wave)
        && read(mod, "normal-ticks", c.normal_ticks)
        && read(mod, "wave-ticks", c.wave_ticks)
        && read(mod, "cbf-sync", c.cbf_sync)
        && read(mod, "spam-guard", c.spam_guard)
        && read(mod, "performance-mode", c.performance)
        && read(mod, "max-correction-ms", c.correction_ms);
    c.cbf_loaded = Loader::get()->isModLoaded("syzzi.click_between_frames");
    return c;
}
std::string describe(inputalign::Assessment const& a) {
    std::string text;
    for (std::size_t i = 0; i < a.size; ++i) {
        if (i) text += "; ";
        text += inputalign::message(a.reasons[i]);
    }
    return text;
}
}
class $modify(InputAlignDiagnosticsInput, GJBaseGameLayer) {
    int checkCollisions(PlayerObject* p, float dt, bool ignoreDamage) {
        int result = GJBaseGameLayer::checkCollisions(p, dt, ignoreDamage);
        auto* pl = PlayLayer::get();
        if (!pl || static_cast<GJBaseGameLayer*>(pl) != static_cast<GJBaseGameLayer*>(this)
            || p != pl->m_player1 || !p) return result;
        if (p->m_isDead) { landingTap.resetAttempt(); return result; }
        if (!std::isfinite(dt) || dt <= 0.f) return result;
        try {
            bool eligible = tapBlocker(pl, p) == inputalign::TapBlock::none &&
                (p->m_isOnGround || p->m_yVelocity < 0);
            sendTap(pl, landingTap.step(nowMs(), eligible, p->m_isOnGround));
        } catch (...) {
            assistFailed = true;
            sendTap(pl, landingTap.stop());
        }
        return result;
    }
    void handleButton(bool press, int button, bool player1) {
        if (sendingTap) {
            GJBaseGameLayer::handleButton(press, button, player1);
            return;
        }
        // The original call always passes through, with the same arguments.
        auto* pl = PlayLayer::get();
        if (!captureFailed && pl && static_cast<GJBaseGameLayer*>(pl) ==
                static_cast<GJBaseGameLayer*>(this) && button == 1) {
            try {
                if (Mod::get()->getSettingValue<bool>("capture")) {
                    auto* player = player1 ? m_player1 : m_player2;
                    // In single-player GD may send the second input channel.
                    // Without an identified player the mode is unknown; omit it.
                    if (player) {
                        history.record({inputalign::assess(config(), {pl->m_isPracticeMode,
                            player->m_isDart}), press, player1});
                    }
                }
            } catch (...) {
                // A diagnostics failure must not prevent the original input call.
                captureFailed = true;
            }
        }
        if (pl && static_cast<GJBaseGameLayer*>(pl) ==
            static_cast<GJBaseGameLayer*>(this) && button == 1 && player1) {
            try {
                auto* p = pl->m_player1;
                bool eligible = tapBlocker(pl, p) == inputalign::TapBlock::none;
                bool falling = p && !p->m_isOnGround && p->m_yVelocity < 0;
                auto ms = Mod::get()->getSettingValue<int64_t>("landing-window-ms");
                landingTap.input(press, eligible, falling, nowMs(),
                    static_cast<int>(std::clamp<int64_t>(ms, 10, 150)));
            } catch (...) {
                assistFailed = true;
                sendTap(pl, landingTap.stop());
            }
        }
        GJBaseGameLayer::handleButton(press, button, player1);
    }
};
class $modify(InputAlignLandingPlayer, PlayerObject) {
    void update(float dt) {
        PlayerObject::update(dt);
        auto* pl = PlayLayer::get();
        if (!pl || static_cast<PlayerObject*>(this) != pl->m_player1) return;
        if (m_isDead) { landingTap.resetAttempt(); return; }
        if (!std::isfinite(dt) || dt <= 0.f) return;
        try {
            sendTap(pl, landingTap.afterPhysics());
        } catch (...) {
            assistFailed = true;
            sendTap(pl, landingTap.stop());
        }
    }
};
class $modify(InputAlignDiagnosticsLevel, PlayLayer) {
    void resetLevel() {
        landingTap.resetAttempt(); assistFailed = false;
        history.clear();
        captureFailed = false;
        PlayLayer::resetLevel();
    }
    void destroyPlayer(PlayerObject* p, GameObject* object) {
        if (p == m_player1) landingTap.resetAttempt();
        PlayLayer::destroyPlayer(p, object);
    }
    void onQuit() {
        landingTap.clear(); assistFailed = false;
        history.clear();
        captureFailed = false;
        PlayLayer::onQuit();
    }
};
class $modify(InputAlignDiagnosticsPause, PauseLayer) {
    void customSetup() {
        sendTap(PlayLayer::get(), landingTap.stop());
        PauseLayer::customSetup();
        auto* menu = CCMenu::create();
        menu->setPosition({0, 0});
        auto* sprite = ButtonSprite::create("Diag InputAlign");
        sprite->setScale(0.55f);
        auto* item = CCMenuItemSpriteExtra::create(sprite, this,
            menu_selector(InputAlignDiagnosticsPause::showDiagnostics));
        auto size = CCDirector::get()->getWinSize();
        item->setPosition({size.width - 75.f, 28.f});
        menu->addChild(item);
        addChild(menu, 100);
    }
    void showDiagnostics(CCObject*) {
        auto c = config();
        std::string text = "<cy>Estado observable, no veredicto de simulacion.</c>\n";
        text += "Las llamadas pueden incluir entradas generadas por otros mods.\n\n";
        auto* pl = PlayLayer::get();
        auto blocker = tapBlocker(pl, pl ? pl->m_player1 : nullptr, true);
        text += fmt::format("<cy>Ayuda de salto:</c> {}\nUltimo resultado: {}\n\n",
            inputalign::blockMessage(blocker),
            inputalign::tapMessage(landingTap.status()));
        auto const& totals = landingTap.stats();
        text += fmt::format("Nivel: {} solicitudes, {} expirados, {} cancelados, {} mantenidos\nUltimo retraso solicitado: {} ms\n\n",
            totals.requested, totals.expired, totals.cancelled, totals.native_holds, totals.last_delay_ms);
        if (captureFailed) text += "<cr>Captura suspendida por error; se conserva el paso de entradas.</c>\n";
        if (!c.available) text += "InputAlign ausente o esquema de ajustes incompatible.\n";
        else {
            text += "<cy>Ajustes actuales (independientes del historial):</c>\n";
            text += fmt::format("Buffer: {} | Trayectoria: {} | Solo practica: {}\n",
                c.enabled ? "ON" : "OFF", c.trajectory ? "ON" : "OFF",
                c.practice_only ? "ON" : "OFF");
            text += fmt::format("Buffer Normal/Wave: {}/{} | CBF cargado/sync: {}/{}\n",
                c.normal ? "ON" : "OFF", c.wave ? "ON" : "OFF",
                c.cbf_loaded ? "SI" : "NO", c.cbf_sync ? "ON" : "OFF");
            text += fmt::format("Ticks Normal/Wave: {}/{} | Presupuesto: {} ms\n",
                c.normal_ticks, c.wave_ticks, c.correction_ms);
            text += fmt::format("Spam Guard: {} | Rendimiento: {}\n",
                c.spam_guard ? "ON" : "OFF", c.performance ? "ON" : "OFF");
            text += "Esas opciones no prueban spam ni tiempo agotado.\n";
        }
        text += "\n<cy>Ultimas llamadas (mas reciente primero):</c>\n";
        if (!history.size()) text += "Sin llamadas observadas en este intento.\n";
        for (std::size_t i = 0; i < history.size() && i < 8; ++i) {
            auto const& e = *history.recent(i);
            text += fmt::format("P{} {}: {}\n", e.player1 ? 1 : 2,
                e.press ? "pulsar" : "soltar", describe(e.assessment));
        }
        FLAlertLayer::create("Diagnostico InputAlign", text, "OK")->show();
    }
};
