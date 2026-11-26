# Eclipse Menu public API (vendored)

Verbatim copy of the public headers from EclipseMenu's `include/` directory
(<https://github.com/EclipseMenu/EclipseMenu>, EPL-2.0).

They are header-only: every call goes through a vtable fetched from the Eclipse
mod at runtime with a Geode event (`eclipse::__internal__::FetchVTableEvent`),
so this mod does not link against Eclipse and keeps working without it.

Do not edit these files here. To update, copy the whole `include/` directory
again and check that `eclipse::__internal__::API_VERSION` (in `events.hpp`)
did not change in a way that breaks the `VTable` layout.

A stale copy is not just missing functions: Eclipse writes the *whole* current
`VTable` into the object this mod passes to `FetchVTableEvent`, so every field
appended upstream is written past the end of our smaller struct (static
storage, silent memory corruption). Keep this directory in sync with the
Eclipse release users actually run; do not add fields of our own.

Synced with Eclipse v1.10.1 (2026-11-26).
