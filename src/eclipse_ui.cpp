#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include <optional>
#include <string>
#include <string_view>

#include "eclipse-api/components.hpp"
#include "eclipse_ui.hpp"

using namespace geode::prelude;

namespace axtra::eclipse_ui {
    namespace {
        constexpr std::string_view ECLIPSE_MOD_ID = "eclipse.eclipse-menu";
        constexpr std::string_view TAB_NAME = "Axtra's Utils";

        // Must match the key used by the recorder in main.cpp
        #if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
            constexpr std::string_view KEY_SAVED_VALUE = "auto-click-key";
        #endif

        bool s_registered = false;
        bool s_available = false;
        std::optional<eclipse::components::Label> s_keyLabel;

        bool apiReady() {
            // Returns an all-null table (and is retried) until Eclipse answers
            return eclipse::__internal__::getVTable().CreateMenuTab != nullptr;
        }

        void addSettingToggle(eclipse::MenuTab const& tab, std::string_view settingKey) {
            auto mod = Mod::get();
            if (!mod->hasSetting(settingKey)) return;

            if (auto setting = mod->getSetting(settingKey)) {
                tab.addModSettingToggle(setting);
            }
        }

        void addSettingFloat(
            eclipse::MenuTab const& tab, std::string_view settingKey,
            float min, float max, std::string format
        ) {
            auto mod = Mod::get();
            if (!mod->hasSetting(settingKey)) return;

            auto setting = mod->getSetting(settingKey);
            if (!setting) return;

            // The API swaps the id and the title of input floats, so both are
            // set to the setting name to keep the label readable.
            std::string id = setting->getDisplayName();
            std::string geodeKey(settingKey);

            auto input = tab.addInputFloat(id, id, [geodeKey](float value) {
                Mod::get()->setSettingValue<float>(geodeKey, value);
            });
            input.setMinValue(min).setMaxValue(max).setFormat(std::move(format));

            if (auto description = setting->getDescription()) {
                input.setDescription(*description);
            }

            // Push the Geode value into Eclipse's (session-only) storage
            eclipse::config::set(id, static_cast<double>(mod->getSettingValue<float>(geodeKey)));

            // And keep it in sync when changed from the Geode settings dialog
            geode::listenForSettingChanges<float>(geodeKey, [id](float value) {
                eclipse::config::set(id, static_cast<double>(value));
            }, mod);
        }

        void buildMenu() {
            s_available = true;

            auto tab = eclipse::MenuTab::find(std::string(TAB_NAME));

            addSettingToggle(tab, "auto-click");
            addSettingFloat(tab, "auto-click-percent", 0.f, 100.f, "%.1f");

            // Key recording is a desktop-only feature and lives in the pause menu
            #if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
                s_keyLabel = tab.addLabel(
                    "Key: " + Mod::get()->getSavedValue<std::string>(std::string(KEY_SAVED_VALUE), "F2")
                );
            #endif

            addSettingToggle(tab, "no-orb-pulse");
            addSettingFloat(tab, "orb-pulse-size", 0.f, 1.f, "%.2f");

            addSettingToggle(tab, "no-circles");
            addSettingToggle(tab, "force-16-9");
        }
    }

    bool available() {
        return s_available;
    }

    void registerComponents() {
        if (s_registered) return;

        auto loader = Loader::get();
        if (!loader->isModLoaded(ECLIPSE_MOD_ID)) {
            // Eclipse may simply not be loaded yet; only give up once loading is over
            if (loader->getLoadingState() == Loader::LoadingState::Done) {
                s_registered = true;
            }
            return;
        }

        if (!apiReady()) return; // Eclipse is loaded but did not answer yet

        buildMenu();
        s_registered = true;
    }

    void updateKeyLabel(std::string const& key) {
        if (!s_available || !s_keyLabel) return;
        s_keyLabel->setText("Key: " + key);
    }
}

// Mods are loaded before the first scene, so this is the safest moment to look
// for Eclipse (which may load after this mod).
class $modify(AxtraUtilsMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        axtra::eclipse_ui::registerComponents();
        return true;
    }
};
