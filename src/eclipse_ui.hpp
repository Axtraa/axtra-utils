#pragma once

#include <string>

// Bridge between this mod's Geode settings and the Eclipse Menu interface.
// The Eclipse public API is header-only and delivered through an event, so
// these functions are no-ops when Eclipse is not installed.
namespace axtra::eclipse_ui {
    // True once this mod's components are actually registered inside Eclipse.
    // The Geode settings dialog stays the fallback UI when this is false.
    bool available();

    // Adds this mod's toggles and inputs to the Eclipse menu. Idempotent and
    // safe to call before Eclipse finished loading (it will retry).
    void registerComponents();

    // Mirrors the recorded auto-click key into the Eclipse menu label.
    void updateKeyLabel(std::string const& key);
}
