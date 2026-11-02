# PROJECT KNOWLEDGE BASE

**Generated:** 2026-06-28

## OVERVIEW

Project: **Axtra's Utils**
Stack: C++23, Geode SDK 5.7.1 (Geometry Dash modding framework), CMake 3.21+, MIT License

A cross-platform Geometry Dash mod (Geode) that disables visual effects, provides auto-click key simulation, and forces 16:9 aspect ratio during gameplay. Targets GD version 2.2081 on Windows, macOS, iOS, and Android.

## STRUCTURE

```
remove-effect/
├── .github/workflows/        # CI/CD — multi-platform builds (Win, Mac, iOS, Android)
├── resources/                # Static assets
│   └── button.png            # Pause menu button sprite
├── src/
│   ├── main.cpp              # Mod logic (features, hooks, pause menu, key recorder)
│   ├── eclipse_ui.cpp        # Registers the features in Eclipse Menu when present
│   ├── eclipse_ui.hpp        # Bridge API used by main.cpp
│   └── eclipse-api/          # Vendored Eclipse Menu public API (header-only)
├── CMakeLists.txt            # Build config (CMake 3.21+, C++23, Geode SDK bindings pinning)
├── mod.json                  # Geode mod manifest — settings, metadata, dependencies
├── changelog.md              # Version changelog
├── README.md                 # User-facing documentation
├── LICENSE                   # MIT License
└── logo.png                  # Mod logo
```

## COMMANDS

| Action    | Command                                                          |
|-----------|------------------------------------------------------------------|
| Build     | `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build` |
| Run       | Place `.geode` artifact in Geometry Dash `geode/mods/` folder    |

**Prerequisite:** `GEODE_SDK` environment variable pointing to Geode SDK installation. Local builds discouraged — CI handles all setup.

## CODING STANDARDS

- **Language**: C++23 (`CMAKE_CXX_STANDARD 23`)
- **Style**: Standard Geode patterns — `$modify` macro hooks, `m_snake_case` fields, `CamelCase` classes
- **Platform guards**: `#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)` for desktop-only features
- **VK codes**: Use cross-platform `RK_*` constants (raw hex) instead of `VK_*` from `windows.h` — required for macOS/iOS compilation
- **Cocos2d**: Uses cocos2d-x 2.x API (NOT 3.x). No `EventListenerTouchOneByOne`, `cocos2d::Touch`, etc.
- **Menu items**: Only `CCMenuItemSpriteExtra` works cross-platform. `CCMenuItemLabel`, `CCMenuItemFont` fail to link on iOS
- **Key storage**: `Mod::get()->getSavedValue<T>()` / `setSavedValue()` for persistent data (e.g., recorded key binding)
- **Mac APIs**: Use `CoreGraphics/CoreGraphics.h` (NOT `ApplicationServices.h`) to avoid `CommentType` conflict with Geode bindings

## KEY ARCHITECTURE

### `$modify(PlayLayer)` — Main gameplay hooks
- `setupHasCompleted()` — Level init (16:9 overlay setup, auto-click state reset)
- `resetLevel()` — Resets auto-click state on restart
- `destroyPlayer()` — Toggles key on death (if auto-click was triggered)
- `levelComplete()` — Toggles key on level completion (if auto-click was triggered)
- `updateVisibility(float dt)` — Per-frame hook for:
  - Orb pulse suppression (`no-orb-pulse` setting)
  - Auto-click key simulation at configurable percentage (desktop only)
  - 16:9 letterbox overlay toggle
- **Auto-click percentage**: Calculated as `(playerX / levelLength) * 100`. Do NOT use `m_level->m_normalPercent` — it returns best %, not real-time progress. `levelLength` must be > 1000 to avoid false triggers.

### `$modify(CCCircleWave)` — Circle effect suppression
- `draw()` override — skips rendering when `no-circles` enabled

### `$modify(PauseLayer)` — UI additions
- Settings button (gear icon) → opens Geode settings popup
- Record Key button (desktop only) → opens `KeyRecordLayer` for interactive key binding

### `KeyRecordLayer` — Interactive key recording (desktop only)
- Windows: Polls `GetAsyncKeyState()` in `update()` to detect new key presses
- macOS: Polls `CGEventSourceKeyState()` with Mac→VK key code mapping
- Snapshots key state on init to avoid triggering on already-held keys
- Saves key name via `Mod::get()->setSavedValue("auto-click-key", name)`
- Live-updates the key label in PauseLayer when recorded
- Cancel button to close without recording

### Key Simulation (desktop only)
- **Windows**: `SendInput()` with `KEYEVENTF_EXTENDEDKEY` for extended keys
- **macOS**: `CGEventCreateKeyboardEvent()` with Mac key code mapping via `vkToMacKey()`
- Key is held for 100ms (time-based via `dt`, not frame-based)

## ECLIPSE INTEGRATION (`src/eclipse_ui.cpp`)

Optional, runtime-only integration: nothing links against Eclipse, and all code paths stay no-ops when it is absent.

- **API**: `src/eclipse-api/` is a verbatim copy of EclipseMenu's public `include/` headers (EPL-2.0, header-only, dispatch through a Geode event). Refresh the copy from the EclipseMenu repository when its `API_VERSION` bumps — never edit it in place.
- **Detection**: `Loader::get()->isModLoaded("eclipse.eclipse-menu")`. Do NOT call the API at mod load: Eclipse may load after this mod. Registration is retried from a `MenuLayer::init` hook until the vtable answers (`getVTable()` is retried internally until a listener returns `Stop`).
- **Registration**: one `MenuTab` named `Axtra's Utils`, filled with `addModSettingToggle()` (bools) and `addInputFloat()` (floats). Geode settings stay the single source of truth; Eclipse only gets a session copy seeded with `eclipse::config::set()` and refreshed via `listenForSettingChanges`.
- **Pause menu**: the gear button is skipped when `eclipse_ui::available()` (Eclipse already exposes the same toggles). The Record Key button stays on desktop — Eclipse's public API has no key-recording component.
- **Key label**: `KeyRecordLayer` calls `axtra::eclipse_ui::updateKeyLabel()` so the Eclipse label follows the recorded key.

## SETTINGS (mod.json)

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `auto-click` | bool | false | Enable auto key press at percentage (desktop only) |
| `auto-click-percent` | float | 50.0 | Trigger percentage (0-100) |
| `no-orb-pulse` | bool | false | Disable orb pulsing |
| `orb-pulse-size` | float | 0.6 | Orb pulse size (0-1) |
| `no-circles` | bool | false | Disable circle wave effects |
| `force-16-9` | bool | false | Force 16:9 letterbox bars |

**Key binding**: Stored via `getSavedValue("auto-click-key", "F2")`, not in mod.json settings.

## CI/CD

- **Workflow**: `.github/workflows/multi-platform.yml`
- **Trigger**: Push to any branch
- **Platforms**: Windows, macOS, iOS, Android32, Android64
- **Build tool**: `geode-sdk/build-geode-mod@main`
- **Post-change**: Must push to trigger build. Use `git commit --allow-empty -m "ci: trigger build" && git push` if needed.

## GOTCHAS

- `PlayLayer::update` hook does NOT work reliably — use `updateVisibility` for per-frame logic
- `m_level->m_normalPercent` is the "best percentage", NOT real-time progress. Calculate from `m_player->getPositionX()` instead
- `m_level->m_levelLength` is 0.0 at level start — use `m_levelLength` (PlayLayer member) and require > 1000
- `updateVisibility` is called before player/level are initialized — wait for `playerX > 0 && levelLen > 0`
- cocos2d-x 2.x (GD 2.2) lacks `EventListenerTouchOneByOne`, `cocos2d::Touch`, `cocos2d::Event` — use manual touch delegates or avoid touch listeners
- `CCMenuItemLabel` / `CCMenuItemFont` cause linker errors on iOS — use `CCMenuItemSpriteExtra` for all buttons
- VK_* constants from `windows.h` only exist on Windows — define cross-platform `RK_*` equivalents for macOS/iOS builds
- `ApplicationServices.h` on macOS causes `CommentType` conflict with Geode bindings — use `CoreGraphics/CoreGraphics.h` instead
- `destroyPlayer` requires `(PlayerObject*, GameObject*)` arguments — not zero-arg
- Auto-click is disabled on mobile (iOS/Android) — no keyboard API available
- Eclipse's public API swaps the `id` and `title` arguments of `addInputFloat()` — pass the same string for both, or the label shows the config key
- Eclipse's `addModSettingToggle()` dereferences the setting — guard with `Mod::hasSetting(key)` (desktop-only settings are missing on mobile)
