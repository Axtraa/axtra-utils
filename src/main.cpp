#ifdef GEODE_IS_WINDOWS
    #include <windows.h>
#endif
#ifdef GEODE_IS_MACOS
    #include <CoreGraphics/CoreGraphics.h>
#endif

#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCCircleWave.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/utils/Keyboard.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "eclipse_ui.hpp"

using namespace geode::prelude;

// The pause-menu buttons sit in a fixed-width column that shares its content
// size between every child, so our items must keep the same footprint as the
// vanilla ones. A wider entry (long key names like "PageDown") makes the
// layout squish the whole column and the buttons end up bunched at the top.
static constexpr float PAUSE_BUTTON_SIZE = 40.f;

#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
// ============================================================
//  Auto-click key  –  Geode keybind setting to simulated key
// ============================================================

// Geode's enumKeyCodes carries the Windows Virtual-Key values on every
// platform (KEY_F2 == 0x71, KEY_PageDown == 0x22, ...), so a Keybind's key can
// be fed straight into the platform key simulation below.
static constexpr int RK_SHIFT = 0x10, RK_CONTROL = 0x11, RK_MENU = 0x12, RK_LWIN = 0x5B;

// The setting is declared desktop-only in mod.json, so it does not exist at all
// on mobile (and neither do the call sites, guarded the same way).
static std::optional<geode::Keybind> autoClickKeybind() {
    auto mod = Mod::get();
    if (!mod->hasSetting("auto-click-key")) return std::nullopt;

    auto binds = mod->getSettingValue<std::vector<geode::Keybind>>("auto-click-key");
    if (binds.empty()) return std::nullopt;
    return binds.front();
}

// ============================================================
//  Key simulation (cross-platform)
// ============================================================

#ifdef GEODE_IS_MACOS
// Map Windows VK codes to Mac key codes
static int vkToMacKey(int vk) {
    switch (vk) {
        case 0x41: return 0x00; case 0x42: return 0x0B; case 0x43: return 0x08;
        case 0x44: return 0x02; case 0x45: return 0x0E; case 0x46: return 0x03;
        case 0x47: return 0x05; case 0x48: return 0x04; case 0x49: return 0x22;
        case 0x4A: return 0x26; case 0x4B: return 0x28; case 0x4C: return 0x25;
        case 0x4D: return 0x2E; case 0x4E: return 0x2D; case 0x4F: return 0x1F;
        case 0x50: return 0x23; case 0x51: return 0x0C; case 0x52: return 0x0F;
        case 0x53: return 0x01; case 0x54: return 0x11; case 0x55: return 0x20;
        case 0x56: return 0x09; case 0x57: return 0x0D; case 0x58: return 0x07;
        case 0x59: return 0x10; case 0x5A: return 0x06;
        case 0x30: return 0x1D; case 0x31: return 0x12; case 0x32: return 0x13;
        case 0x33: return 0x14; case 0x34: return 0x15; case 0x35: return 0x17;
        case 0x36: return 0x16; case 0x37: return 0x1A; case 0x38: return 0x1C;
        case 0x39: return 0x19;
        case 0x20: return 0x31; case 0x0D: return 0x24; case 0x09: return 0x30;
        case 0x08: return 0x33; case 0x2E: return 0x75; case 0x2D: return 0x72;
        case 0x1B: return 0x35; case 0x24: return 0x73; case 0x23: return 0x77;
        case 0x21: return 0x74; case 0x22: return 0x79;
        case 0x26: return 0x7E; case 0x28: return 0x7D;
        case 0x25: return 0x7B; case 0x27: return 0x7C;
        case 0xA0: return 0x38; case 0xA1: return 0x3C;
        case 0xA2: return 0x3B; case 0xA3: return 0x3E;
        case 0xA4: return 0x3A; case 0xA5: return 0x3D;
        // Generic modifier keys, as stored by Geode's keybind chords
        case RK_SHIFT: return 0x38; case RK_CONTROL: return 0x3B;
        case RK_MENU: return 0x3A; case RK_LWIN: return 0x37;
        case 0x14: return 0x39;
        case 0x70: return 0x7A; case 0x71: return 0x78; case 0x72: return 0x63;
        case 0x73: return 0x76; case 0x74: return 0x60; case 0x75: return 0x61;
        case 0x76: return 0x62; case 0x77: return 0x64; case 0x78: return 0x65;
        case 0x79: return 0x6D; case 0x7A: return 0x67; case 0x7B: return 0x6F;
        default: return -1;
    }
}
#endif

#ifdef GEODE_IS_WINDOWS
// Check if a VK code is an extended key (needs KEYEVENTF_EXTENDEDKEY)
static bool isExtendedKey(int vk) {
    return (vk == 0x2D || vk == 0x2E || // Insert, Delete
            vk == 0x24 || vk == 0x23 || // Home, End
            vk == 0x21 || vk == 0x22 || // PageUp, PageDown
            (vk >= 0x25 && vk <= 0x28) || // Arrow keys
            vk == 0xA1 || vk == 0xA3 || vk == 0xA5 || // RShift, RCtrl, RAlt
            vk == 0x5B || vk == 0x5C || // Left/Right Windows
            (vk >= 0x60 && vk <= 0x6F)); // Numpad
}
#endif

static void simulateKeyState(int vkCode, bool down) {
    #ifdef GEODE_IS_WINDOWS
        INPUT input{};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = static_cast<WORD>(vkCode);
        if (!down) input.ki.dwFlags = KEYEVENTF_KEYUP;
        if (isExtendedKey(vkCode)) input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        SendInput(1, &input, sizeof(INPUT));
    #endif
    #ifdef GEODE_IS_MACOS
        int macKey = vkToMacKey(vkCode);
        if (macKey < 0) return;
        CGEventRef event = CGEventCreateKeyboardEvent(NULL, macKey, down);
        CGEventPost(kCGHIDEventTap, event);
        CFRelease(event);
    #endif
}

// A keybind can be a chord: hold the modifiers while the key is pressed, then
// unwind them in reverse order.
static constexpr int MODIFIER_KEYS[] = {RK_SHIFT, RK_CONTROL, RK_MENU, RK_LWIN};
static constexpr uint8_t MODIFIER_BITS[] = {
    geode::KeyboardModifier::Shift,
    geode::KeyboardModifier::Control,
    geode::KeyboardModifier::Alt,
    geode::KeyboardModifier::Super,
};
static constexpr size_t MODIFIER_COUNT = sizeof(MODIFIER_KEYS) / sizeof(MODIFIER_KEYS[0]);

static void pressModifiers(geode::KeyboardModifier modifiers) {
    for (size_t i = 0; i < MODIFIER_COUNT; ++i) {
        if (modifiers & MODIFIER_BITS[i]) simulateKeyState(MODIFIER_KEYS[i], true);
    }
}

static void releaseModifiers(geode::KeyboardModifier modifiers) {
    for (size_t i = MODIFIER_COUNT; i > 0; --i) {
        if (modifiers & MODIFIER_BITS[i - 1]) simulateKeyState(MODIFIER_KEYS[i - 1], false);
    }
}

static void pressKeybind(geode::Keybind const& bind) {
    int vk = static_cast<int>(bind.key);
    if (vk <= 0) return;
    pressModifiers(bind.modifiers);
    simulateKeyState(vk, true);
}

static void releaseKeybind(geode::Keybind const& bind) {
    int vk = static_cast<int>(bind.key);
    if (vk <= 0) return;
    simulateKeyState(vk, false);
    releaseModifiers(bind.modifiers);
}

// Tap the key once (used on death / level completion)
static void tapKeybind(geode::Keybind const& bind) {
    pressKeybind(bind);
    releaseKeybind(bind);
}

#endif // GEODE_IS_WINDOWS || GEODE_IS_MACOS

// ============================================================
//  PlayLayer  –  auto-click + orb pulse + 16:9 overlay
// ============================================================

class $modify (PlayLayer) {
    struct Fields {
        CCNode* m_169overlay = nullptr;
        #if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
            bool m_autoClickFired = false;
            bool m_autoClickHeld = false;
            geode::Keybind m_autoClickBind{};
            float m_autoClickTimer = 0.f;
            float m_startPosX = 0.f;
        #endif
    };

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        #if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
            m_fields->m_autoClickFired = false;
            m_fields->m_autoClickHeld = false;
            m_fields->m_autoClickTimer = 0.f;
            m_fields->m_startPosX = m_player1 ? m_player1->getPositionX() : 0.f;
        #endif
        update169Overlay();
    }

    void resetLevel() {
        #if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
            // Don't leave a simulated key stuck down if the level restarts
            if (m_fields->m_autoClickHeld) releaseKeybind(m_fields->m_autoClickBind);
        #endif
        PlayLayer::resetLevel();
        #if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
            m_fields->m_autoClickFired = false;
            m_fields->m_autoClickHeld = false;
            m_fields->m_autoClickTimer = 0.f;
            m_fields->m_startPosX = m_player1 ? m_player1->getPositionX() : 0.f;
        #endif
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        PlayLayer::destroyPlayer(player, object);
        #if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
            if (m_fields->m_autoClickFired && Mod::get()->getSettingValue<bool>("auto-click")) {
                if (auto bind = autoClickKeybind()) tapKeybind(*bind);
            }
        #endif
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        #if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
            if (m_fields->m_autoClickFired && Mod::get()->getSettingValue<bool>("auto-click")) {
                if (auto bind = autoClickKeybind()) tapKeybind(*bind);
            }
        #endif
    }

    void remove169Overlay() {
        if (m_fields->m_169overlay) {
            m_fields->m_169overlay->removeFromParent();
            m_fields->m_169overlay = nullptr;
        }
    }

    void update169Overlay() {
        remove169Overlay();
        if (!Mod::get()->getSettingValue<bool>("force-16-9")) return;

        auto ui = m_uiLayer;
        if (!ui) return;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        float target = 16.f / 9.f;
        float current = winSize.width / winSize.height;
        if (std::abs(current - target) < 0.005f) return;

        auto overlay = CCNode::create();
        overlay->setID("169-overlay"_spr);
        overlay->setPosition({0, 0});

        if (current > target) {
            float barW = (winSize.width - winSize.height * target) / 2.f;
            auto left  = CCLayerColor::create({0,0,0,255}, barW, winSize.height);
            left->setPosition({0, 0});
            overlay->addChild(left);
            auto right = CCLayerColor::create({0,0,0,255}, barW, winSize.height);
            right->setPosition({winSize.width - barW, 0});
            overlay->addChild(right);
        } else {
            float barH = (winSize.height - winSize.width / target) / 2.f;
            auto top    = CCLayerColor::create({0,0,0,255}, winSize.width, barH);
            top->setPosition({0, winSize.height - barH});
            overlay->addChild(top);
            auto bottom = CCLayerColor::create({0,0,0,255}, winSize.width, barH);
            bottom->setPosition({0, 0});
            overlay->addChild(bottom);
        }

        ui->addChild(overlay, -999999);
        m_fields->m_169overlay = overlay;
    }

    virtual void updateVisibility(float dt) {
        auto* fmod = FMODAudioEngine::get();
        auto pulse1 = fmod->m_pulse1;
        auto audioScale = m_audioEffectsLayer->m_audioScale;

        if (Mod::get()->getSettingValue<bool>("no-orb-pulse")) {
            auto s = Mod::get()->getSettingValue<float>("orb-pulse-size");
            fmod->m_pulse1 = s;
            m_audioEffectsLayer->m_audioScale = s;
        }

        #if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
            // --- Auto Click ---
            // Phase 1: release key after timer expires
            if (m_fields->m_autoClickHeld) {
                m_fields->m_autoClickTimer -= dt;
                if (m_fields->m_autoClickTimer <= 0.f) {
                    releaseKeybind(m_fields->m_autoClickBind);
                    m_fields->m_autoClickHeld = false;
                }
            }
            // Phase 2: detect percentage and press key
            // Skip if using start position (startPosX > 0 means not starting from beginning)
            if (!m_fields->m_autoClickFired && m_fields->m_startPosX <= 0.f && Mod::get()->getSettingValue<bool>("auto-click") && m_level && m_player1) {
                float playerX = m_player1->getPositionX();
                float levelLen = m_levelLength;
                if (playerX > 0.f && levelLen > 1000.f) {
                    float triggerPct = Mod::get()->getSettingValue<float>("auto-click-percent");
                    float currentPct = (playerX / levelLen) * 100.f;
                    if (currentPct >= triggerPct) {
                        if (auto bind = autoClickKeybind()) {
                            pressKeybind(*bind);
                            m_fields->m_autoClickBind = *bind;
                            m_fields->m_autoClickHeld = true;
                            m_fields->m_autoClickTimer = 0.1f;
                        }
                        m_fields->m_autoClickFired = true;
                    }
                }
            }
        #endif

        PlayLayer::updateVisibility(dt);

        fmod->m_pulse1 = pulse1;
        m_audioEffectsLayer->m_audioScale = audioScale;

        bool want169 = Mod::get()->getSettingValue<bool>("force-16-9");
        bool has169  = m_fields->m_169overlay != nullptr;
        if (want169 != has169) {
            want169 ? update169Overlay() : remove169Overlay();
        }
    }
};

// ============================================================
//  CCCircleWave  –  suppress circle effects
// ============================================================

class $modify (CCCircleWave) {
    virtual void draw() {
        if (Mod::get()->getSettingValue<bool>("no-circles")) return;
        CCCircleWave::draw();
    }
};

// ============================================================
//  PauseLayer  –  settings button
// ============================================================

// Wraps a sprite in a fixed-size node so the pause menu column sees the same
// footprint as the vanilla buttons regardless of the sprite's own size.
static CCNode* boxedPauseItem(CCNode* content, float boxSize) {
    auto box = CCNode::create();
    box->setContentSize({boxSize, boxSize});
    content->setPosition({boxSize / 2.f, boxSize / 2.f});
    box->addChild(content);
    return box;
}

// Actual size of the vanilla entries in the menu; our items must match it or
// the fixed-width column squishes everything. Called before ours are added.
static float pauseButtonSize(CCNode* menu) {
    if (auto children = menu->getChildren()) {
        for (auto child : CCArrayExt<CCNode*>(children)) {
            if (auto item = typeinfo_cast<CCMenuItem*>(child)) {
                auto size = item->getScaledContentSize();
                // min: wide entries (toggle labels) must not blow up the box
                float unit = std::min(size.width, size.height);
                if (unit > 0.f) return unit;
            }
        }
    }
    return PAUSE_BUTTON_SIZE;
}

class $modify (RemoveEffectPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        // Make sure the Eclipse tab exists before deciding which button to show
        axtra::eclipse_ui::registerComponents();

        auto menu = this->getChildByID("right-button-menu");
        if (!menu) return;

        // Match the vanilla buttons so the column keeps its layout
        float unit = pauseButtonSize(menu);

        // --- Settings button ---
        // Eclipse exposes the same toggles, so no need for a second entry point.
        // The keybind lives in the Geode settings dialog opened from here.
        if (!axtra::eclipse_ui::available()) {
            auto btnSprite = CircleButtonSprite::create(
                CCSprite::create("button.png"_spr)
            );
            btnSprite->setScale(0.675f);
            auto settingsBtn = CCMenuItemSpriteExtra::create(
                boxedPauseItem(btnSprite, unit), this,
                menu_selector(RemoveEffectPauseLayer::onOpenSettings)
            );
            settingsBtn->setID("remove-effect-settings-btn"_spr);
            menu->addChild(settingsBtn);
        }

        menu->updateLayout();
    }

    void onOpenSettings(CCObject*) {
        openSettingsPopup(Mod::get());
    }
};
