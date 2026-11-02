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

#include <algorithm>

#include "eclipse_ui.hpp"

using namespace geode::prelude;

// The pause-menu buttons sit in a fixed-width column that shares its content
// size between every child, so our items must keep the same footprint as the
// vanilla ones. A wider entry (long key names like "PageDown") makes the
// layout squish the whole column and the buttons end up bunched at the top.
static constexpr float PAUSE_BUTTON_SIZE = 40.f;

#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
// ============================================================
//  Key name ↔ Windows Virtual-Key code conversion
// ============================================================

// VK codes as raw hex values (cross-platform, matches Windows VK_*)
static constexpr int RK_F1 = 0x70, RK_F12 = 0x7B;
static constexpr int RK_SPACE = 0x20, RK_RETURN = 0x0D, RK_TAB = 0x09, RK_BACK = 0x08;
static constexpr int RK_DELETE = 0x2E, RK_INSERT = 0x2D, RK_ESCAPE = 0x1B;
static constexpr int RK_HOME = 0x24, RK_END = 0x23, RK_PRIOR = 0x21, RK_NEXT = 0x22;
static constexpr int RK_UP = 0x26, RK_DOWN = 0x28, RK_LEFT = 0x25, RK_RIGHT = 0x27;
static constexpr int RK_LSHIFT = 0xA0, RK_RSHIFT = 0xA1;
static constexpr int RK_LCTRL = 0xA2, RK_RCTRL = 0xA3;
static constexpr int RK_LALT = 0xA4, RK_RALT = 0xA5;
static constexpr int RK_CAPITAL = 0x14;
static constexpr int RK_NUMPAD0 = 0x60, RK_NUMPAD9 = 0x69;
static constexpr int RK_MULTIPLY = 0x6A, RK_ADD = 0x6B;
static constexpr int RK_SUBTRACT = 0x6D, RK_DECIMAL = 0x6E, RK_DIVIDE = 0x6F;

static std::string keyNameFromVK(int vk) {
    if (vk >= RK_F1 && vk <= RK_F12)
        return "F" + std::to_string(vk - RK_F1 + 1);
    if (vk >= 'A' && vk <= 'Z')
        return std::string(1, static_cast<char>(vk));
    if (vk >= '0' && vk <= '9')
        return std::string(1, static_cast<char>(vk));

    switch (vk) {
        case RK_SPACE:    return "Space";
        case RK_RETURN:   return "Enter";
        case RK_TAB:      return "Tab";
        case RK_BACK:     return "Backspace";
        case RK_DELETE:   return "Delete";
        case RK_INSERT:   return "Insert";
        case RK_HOME:     return "Home";
        case RK_END:      return "End";
        case RK_PRIOR:    return "PageUp";
        case RK_NEXT:     return "PageDown";
        case RK_UP:       return "Up";
        case RK_DOWN:     return "Down";
        case RK_LEFT:     return "Left";
        case RK_RIGHT:    return "Right";
        case RK_LSHIFT:   return "LShift";
        case RK_RSHIFT:   return "RShift";
        case RK_LCTRL:    return "LCtrl";
        case RK_RCTRL:    return "RCtrl";
        case RK_LALT:     return "LAlt";
        case RK_RALT:     return "RAlt";
        case RK_CAPITAL:  return "CapsLock";
        case RK_NUMPAD0:  return "Num0";
        case 0x61: return "Num1";
        case 0x62: return "Num2";
        case 0x63: return "Num3";
        case 0x64: return "Num4";
        case 0x65: return "Num5";
        case 0x66: return "Num6";
        case 0x67: return "Num7";
        case 0x68: return "Num8";
        case RK_NUMPAD9:  return "Num9";
        case RK_MULTIPLY: return "Num*";
        case RK_ADD:      return "Num+";
        case RK_SUBTRACT: return "Num-";
        case RK_DECIMAL:  return "Num.";
        case RK_DIVIDE:   return "Num/";
        default:          return "";
    }
}

static int vkFromKeyName(const std::string& name) {
    if (name.empty()) return 0;

    if (name[0] == 'F' && name.size() > 1) {
        try {
            int n = std::stoi(name.substr(1));
            if (n >= 1 && n <= 12) return RK_F1 + n - 1;
        } catch (...) {}
        return 0;
    }

    if (name.size() == 1) {
        char c = name[0];
        if (c >= '0' && c <= '9') return c;
        if (c >= 'A' && c <= 'Z') return c;
        if (c >= 'a' && c <= 'z') return c - 32;
        return 0;
    }

    if (name == "Space")      return RK_SPACE;
    if (name == "Enter")      return RK_RETURN;
    if (name == "Tab")        return RK_TAB;
    if (name == "Backspace")  return RK_BACK;
    if (name == "Delete")     return RK_DELETE;
    if (name == "Insert")     return RK_INSERT;
    if (name == "Home")       return RK_HOME;
    if (name == "End")        return RK_END;
    if (name == "PageUp")     return RK_PRIOR;
    if (name == "PageDown")   return RK_NEXT;
    if (name == "Up")         return RK_UP;
    if (name == "Down")       return RK_DOWN;
    if (name == "Left")       return RK_LEFT;
    if (name == "Right")      return RK_RIGHT;
    if (name == "LShift")     return RK_LSHIFT;
    if (name == "RShift")     return RK_RSHIFT;
    if (name == "LCtrl")      return RK_LCTRL;
    if (name == "RCtrl")      return RK_RCTRL;
    if (name == "LAlt")       return RK_LALT;
    if (name == "RAlt")       return RK_RALT;
    if (name == "CapsLock")   return RK_CAPITAL;
    if (name == "Num0")       return RK_NUMPAD0;
    if (name == "Num1")       return 0x61;
    if (name == "Num2")       return 0x62;
    if (name == "Num3")       return 0x63;
    if (name == "Num4")       return 0x64;
    if (name == "Num5")       return 0x65;
    if (name == "Num6")       return 0x66;
    if (name == "Num7")       return 0x67;
    if (name == "Num8")       return 0x68;
    if (name == "Num9")       return RK_NUMPAD9;

    return 0;
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
            (vk >= 0x60 && vk <= 0x6F)); // Numpad
}
#endif

static void simulateKeyPress(int vkCode) {
    #ifdef GEODE_IS_WINDOWS
        INPUT input{};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = static_cast<WORD>(vkCode);
        if (isExtendedKey(vkCode)) input.ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
        SendInput(1, &input, sizeof(INPUT));
    #endif
    #ifdef GEODE_IS_MACOS
        int macKey = vkToMacKey(vkCode);
        if (macKey < 0) return;
        CGEventRef event = CGEventCreateKeyboardEvent(NULL, macKey, true);
        CGEventPost(kCGHIDEventTap, event);
        CFRelease(event);
    #endif
}

static void simulateKeyRelease(int vkCode) {
    #ifdef GEODE_IS_WINDOWS
        INPUT input{};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = static_cast<WORD>(vkCode);
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        if (isExtendedKey(vkCode)) input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        SendInput(1, &input, sizeof(INPUT));
    #endif
    #ifdef GEODE_IS_MACOS
        int macKey = vkToMacKey(vkCode);
        if (macKey < 0) return;
        CGEventRef event = CGEventCreateKeyboardEvent(NULL, macKey, false);
        CGEventPost(kCGHIDEventTap, event);
        CFRelease(event);
    #endif
}

// ============================================================
//  Key Record Layer  –  press a key to bind it
// ============================================================

// Re-fit the label every time the binding changes, otherwise the text spills
// out of the pause menu column.
static void setKeyText(CCLabelBMFont* label, std::string const& name) {
    if (!label) return;
    label->setString(name.c_str());

    constexpr float maxScale = 0.55f;
    constexpr float minScale = 0.12f;

    float maxWidth = PAUSE_BUTTON_SIZE - 6.f;
    if (auto parent = label->getParent()) {
        float boxWidth = parent->getContentWidth();
        if (boxWidth > 0.f) maxWidth = boxWidth - 6.f;
    }

    float width = label->getContentWidth();
    float scale = maxScale;
    if (width > 0.f && width * scale > maxWidth) {
        scale = std::clamp(maxWidth / width, minScale, maxScale);
    }
    label->setScale(scale);
}

// Store the new binding and reflect it in every UI showing it
static void applyAutoClickKey(std::string const& name, CCLabelBMFont* keyLabel) {
    Mod::get()->setSavedValue("auto-click-key", name);
    setKeyText(keyLabel, name);
    axtra::eclipse_ui::updateKeyLabel(name);
}

class KeyRecordLayer : public CCLayerColor {
protected:
    CCLabelBMFont* m_keyLabel = nullptr;
    #ifdef GEODE_IS_WINDOWS
        bool m_prevState[256] = {};
    #endif
    #ifdef GEODE_IS_MACOS
        bool m_prevState[128] = {};
    #endif

    bool init(CCLabelBMFont* keyLabel) {
        m_keyLabel = keyLabel;
        if (!CCLayerColor::initWithColor({0, 0, 0, 180})) return false;

        auto win = CCDirector::sharedDirector()->getWinSize();
        setContentSize(win);

        // --- panel background ---
        auto panel = CCLayerColor::create({25, 25, 25, 230}, 340, 220);
        panel->setPosition({(win.width - 340) / 2.f, (win.height - 220) / 2.f});
        addChild(panel);

        // title
        auto title = CCLabelBMFont::create("RECORD KEY", "goldFont.fnt");
        title->setScale(0.7f);
        title->setPosition({win.width / 2, win.height / 2 + 70});
        addChild(title);

        // instruction
        auto instr = CCLabelBMFont::create("Press any key...", "bigFont.fnt");
        instr->setScale(0.55f);
        instr->setPosition({win.width / 2, win.height / 2 + 15});
        addChild(instr);

        // current binding
        std::string cur = Mod::get()->getSavedValue<std::string>("auto-click-key", "F2");
        auto curLabel = CCLabelBMFont::create(("Current: " + cur).c_str(), "goldFont.fnt");
        curLabel->setScale(0.45f);
        curLabel->setPosition({win.width / 2, win.height / 2 - 20});
        curLabel->setOpacity(140);
        addChild(curLabel);

        // cancel button
        auto cancelLabel = CCLabelBMFont::create("Cancel", "goldFont.fnt");
        cancelLabel->setScale(0.5f);
        auto cancelBtn = CCMenuItemSpriteExtra::create(
            cancelLabel, this,
            menu_selector(KeyRecordLayer::onCancel)
        );
        auto menu = CCMenu::create(cancelBtn, nullptr);
        menu->setPosition({win.width / 2, win.height / 2 - 65});
        addChild(menu);

        // snapshot current key state so we only detect NEW presses
        #ifdef GEODE_IS_WINDOWS
            for (int i = 0; i < 256; i++)
                m_prevState[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
        #endif
        #ifdef GEODE_IS_MACOS
            for (int i = 0; i < 128; i++)
                m_prevState[i] = CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, i);
        #endif

        scheduleUpdate();
        return true;
    }

    void update(float dt) override {
        CCLayerColor::update(dt);
        #ifdef GEODE_IS_WINDOWS
            for (int k = 1; k < 256; k++) {
                bool down = (GetAsyncKeyState(k) & 0x8000) != 0;
                if (down && !m_prevState[k]) {
                    std::string name = keyNameFromVK(k);
                    if (!name.empty()) {
                        applyAutoClickKey(name, m_keyLabel);
                        removeFromParentAndCleanup(true);
                        return;
                    }
                }
                m_prevState[k] = down;
            }
        #endif
        #ifdef GEODE_IS_MACOS
            // Map Mac key codes back to Windows VK codes for keyNameFromVK
            static const int macToVk[128] = {
                0x41,0x53,0x44,0x46,0x48,0x47,0x5A,0x58, // 0x00-0x07: A,S,D,F,H,G,Z,X
                0x43,0x56,0x00,0x42,0x51,0x57,0x45,0x52, // 0x08-0x0F: C,V,_,B,Q,W,E,R
                0x59,0x54,0x31,0x32,0x33,0x34,0x36,0x35, // 0x10-0x17: Y,T,1,2,3,4,6,5
                0x39,0x37,0x2D,0x30,0x50,0x4C,0x4A,0x4B, // 0x18-0x1F: =,9,_,0,P,L,J,K
                0x4F,0x49,0x00,0x4E,0x4D,0x4F,0x4E,0x00, // 0x20-0x27: O,I,_,N,M,_,_,_
                0x20,0x08,0x00,0x1B,0x00,0x00,0x00,0x00, // 0x28-0x2F: Space,Delete,_,Esc,_,_,_,_
                0x00,0x31,0x00,0x32,0x00,0x33,0x34,0x36, // 0x30-0x37: _,1,_,2,_,3,4,6
                0x35,0x00,0x2D,0x00,0x37,0x00,0x00,0x00, // 0x38-0x3F: 5,_,=,_,7,_,_,_
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0x40-0x47: _,_,_,_,_,_,_,_
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0x48-0x4F: _,_,_,_,_,_,_,_
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0x50-0x57: _,_,_,_,_,_,_,_
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0x58-0x5F: _,_,_,_,_,_,_,_
                0x74,0x73,0x75,0x77,0x7B,0x7C,0x7D,0x7E, // 0x60-0x67: F5,F3,F6,F4,F11,F12,F13,F14
                0x00,0x79,0x00,0x76,0x60,0x61,0x62,0x63, // 0x68-0x6F: _,F10,_,F8,F1,F2,F3,F4
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0x70-0x77: _,_,_,_,_,_,_,_
                0x7E,0x00,0x77,0x00,0x3F,0x7F,0x00,0x00, // 0x78-0x7F: Up,_,Down,_,F7,Right,_,_
            };
            for (int k = 0; k < 128; k++) {
                bool down = CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, k);
                if (down && !m_prevState[k]) {
                    int vk = macToVk[k];
                    if (vk != 0) {
                        std::string name = keyNameFromVK(vk);
                        if (!name.empty()) {
                            applyAutoClickKey(name, m_keyLabel);
                            removeFromParentAndCleanup(true);
                            return;
                        }
                    }
                }
                m_prevState[k] = down;
            }
        #endif
    }

public:
    void onCancel(CCObject*) {
        removeFromParentAndCleanup(true);
    }

    static KeyRecordLayer* create(CCLabelBMFont* keyLabel = nullptr) {
        auto* ret = new KeyRecordLayer();
        if (ret && ret->init(keyLabel)) { ret->autorelease(); return ret; }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};
#endif // GEODE_IS_WINDOWS || GEODE_IS_MACOS

// ============================================================
//  PlayLayer  –  auto-click + orb pulse + 16:9 overlay
// ============================================================

class $modify (PlayLayer) {
    struct Fields {
        CCNode* m_169overlay = nullptr;
        #if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
            bool m_autoClickFired = false;
            int m_autoClickVK = 0;
            float m_autoClickTimer = 0.f;
            float m_startPosX = 0.f;
        #endif
    };

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        #if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
            m_fields->m_autoClickFired = false;
            m_fields->m_autoClickVK = 0;
            m_fields->m_autoClickTimer = 0.f;
            m_fields->m_startPosX = m_player1 ? m_player1->getPositionX() : 0.f;
        #endif
        update169Overlay();
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        #if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
            m_fields->m_autoClickFired = false;
            m_fields->m_autoClickVK = 0;
            m_fields->m_autoClickTimer = 0.f;
            m_fields->m_startPosX = m_player1 ? m_player1->getPositionX() : 0.f;
        #endif
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        PlayLayer::destroyPlayer(player, object);
        #if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
            if (m_fields->m_autoClickFired && Mod::get()->getSettingValue<bool>("auto-click")) {
                std::string keyName = Mod::get()->getSavedValue<std::string>("auto-click-key", "F2");
                int vk = vkFromKeyName(keyName);
                if (vk != 0) {
                    simulateKeyPress(vk);
                    simulateKeyRelease(vk);
                }
            }
        #endif
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        #if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
            if (m_fields->m_autoClickFired && Mod::get()->getSettingValue<bool>("auto-click")) {
                std::string keyName = Mod::get()->getSavedValue<std::string>("auto-click-key", "F2");
                int vk = vkFromKeyName(keyName);
                if (vk != 0) {
                    simulateKeyPress(vk);
                    simulateKeyRelease(vk);
                }
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
            if (m_fields->m_autoClickVK != 0) {
                m_fields->m_autoClickTimer -= dt;
                if (m_fields->m_autoClickTimer <= 0.f) {
                    simulateKeyRelease(m_fields->m_autoClickVK);
                    m_fields->m_autoClickVK = 0;
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
                        std::string keyName = Mod::get()->getSavedValue<std::string>("auto-click-key", "F2");
                        int vk = vkFromKeyName(keyName);
                        if (vk != 0) {
                            simulateKeyPress(vk);
                            m_fields->m_autoClickVK = vk;
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
//  PauseLayer  –  settings button + key-record button
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
    struct Fields {
        #if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
            CCLabelBMFont* m_keyLabel = nullptr;
        #endif
    };

    void customSetup() {
        PauseLayer::customSetup();

        // Make sure the Eclipse tab exists before deciding which button to show
        axtra::eclipse_ui::registerComponents();

        auto menu = this->getChildByID("right-button-menu");
        if (!menu) return;

        // Match the vanilla buttons so the column keeps its layout
        float unit = pauseButtonSize(menu);

        // --- Settings button ---
        // Eclipse exposes the same toggles, so no need for a second entry point
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

        // --- Record Key button ---
        #if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
            std::string curKey = Mod::get()->getSavedValue<std::string>("auto-click-key", "F2");
            auto keyLabel = CCLabelBMFont::create(curKey.c_str(), "goldFont.fnt");
            m_fields->m_keyLabel = keyLabel;
            auto keyBtn = CCMenuItemSpriteExtra::create(
                boxedPauseItem(keyLabel, unit), this,
                menu_selector(RemoveEffectPauseLayer::onRecordKey)
            );
            setKeyText(keyLabel, curKey);
            keyBtn->setID("remove-effect-record-key-btn"_spr);
            menu->addChild(keyBtn);
        #endif

        menu->updateLayout();
    }

    void onOpenSettings(CCObject*) {
        openSettingsPopup(Mod::get());
    }

    #if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
        void onRecordKey(CCObject*) {
            auto scene = CCDirector::sharedDirector()->getRunningScene();
            if (scene) scene->addChild(KeyRecordLayer::create(m_fields->m_keyLabel), 999);
        }
    #endif
};
