# AGENTS.md

Geode mod for Geometry Dash 2.2081 (Geode 5.7.1, C++23, MIT). Suppresses orb-pulse/circle VFX, simulates a
keypress at a level percentage (desktop), forces a 16:9 letterbox, and mirrors all of it into Eclipse Menu's UI
when Eclipse is installed.

## Build & verify

There are no tests, linters or formatters. **The only real verification is the CI build.**

- Push to any branch → `.github/workflows/multi-platform.yml` builds Windows, macOS, iOS, Android32/64 via
  `geode-sdk/build-geode-mod@main`. Artifacts only; no release/publish step.
- Local build needs `GEODE_SDK` set (CMake `FATAL_ERROR`s without it):
  `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`
- `CMakeLists.txt` GLOBs `src/*.cpp`; new files are picked up, but re-run configure if a build seems to ignore one.
- Bindings are cloned by Geode from `geode-sdk/bindings@main` at configure time, so builds are **not reproducible**: an
  upstream bindings/codegen change can break CI with no local diff. Pinning by bare SHA does not work (GitHub rejects
  unadvertised objects; the fetch just falls back to the default).
- iOS only: codegen emits definitions for the standalone FMOD C API into `GeneratedSource.cpp` without the FMOD
  headers. `CMakeLists.txt` force-includes `fmod.hpp` on the `GeodeBindings` target — keep it (or an equivalent
  patch) or iOS dies on `unknown type name 'FMOD_RESULT'`.

## Layout

| Path | Role |
|---|---|
| `src/main.cpp` | All gameplay: `$modify` of `PlayLayer`, `CCCircleWave`, `PauseLayer`, plus `KeyRecordLayer` |
| `src/eclipse_ui.cpp/.hpp` | Optional Eclipse integration; own `MenuLayer::init` hook |
| `src/eclipse-api/` | Verbatim vendored Eclipse public headers (EPL-2.0). Never edit; re-copy the whole `include/` dir |

Settings live in `mod.json` and stay the single source of truth. The recorded auto-click key is **not** a setting:
it is stored with `getSavedValue("auto-click-key", "F2")`.

## GD / Geode gotchas (bitten before)

- Per-frame logic goes in `PlayLayer::updateVisibility`, **not** `update` (unreliable). It runs before player/level
  exist — wait for `playerX > 0 && levelLength > 0`.
- Progress = `m_player1->getPositionX() / m_levelLength`. `m_level->m_normalPercent` is the *best* percent, and
  `m_level->m_levelLength` is 0 at level start. Require `m_levelLength > 1000` before firing anything.
- This is cocos2d-x 2.x: no `Touch`, `cocos2d::Event`, `EventListenerTouchOneByOne`.
- `CCMenuItemLabel`/`CCMenuItemFont` fail to link on iOS — use `CCMenuItemSpriteExtra`.
- No `VK_*` outside Windows: use the local cross-platform `RK_*` constants. Mac key simulation must include
  `CoreGraphics/CoreGraphics.h` (`ApplicationServices.h` clashes with Geode's `CommentType`).
- `destroyPlayer` takes `(PlayerObject*, GameObject*)`.
- Every OS-level keyboard/input include **and** call must be inside `#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)`,
  otherwise Android/iOS CI fails. `auto-click*` settings are declared `platforms: ["win","mac"]` in mod.json, so
  `Mod::hasSetting()` is false on mobile — never dereference a setting without that guard.
- `_spr` resolves to `axtra.axtra-utils`; new resources must be listed in `mod.json` `resources`.

## Eclipse integration (`src/eclipse_ui.cpp`)

Runtime-only: nothing links against Eclipse, and every path is a no-op when it is absent.

- Check `Loader::get()->isModLoaded("eclipse.eclipse-menu")`. Do **not** call the API during mod load — Eclipse may
  load later. Registration is retried from `MenuLayer::init`; `getVTable()` retries itself until Eclipse answers.
- One `MenuTab` named `Axtra's Utils`. Bools via `addModSettingToggle()`, floats via `addInputFloat()`; both are
  session storage (`eclipse::config::set` == temp storage, matching the API's `disableSaving()`), seeded from the
  Geode setting and refreshed by `geode::listenForSettingChanges`.
- Public API bug: `MenuTab::addInputFloat(id, title, …)` swaps id/title — pass the same string for both or the label
  shows the config key.
- `addModSettingToggle()` dereferences the setting; guard with `Mod::hasSetting(key)`.
- The pause-menu gear button is skipped when `eclipse_ui::available()`; the Record Key button stays (Eclipse has no
  key-recording component). Recording calls `applyAutoClickKey()`, which updates both labels.
