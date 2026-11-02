# Eclipse Menu public API (vendored)

Verbatim copy of the public headers from EclipseMenu's `include/` directory
(<https://github.com/EclipseMenu/EclipseMenu>, EPL-2.0).

They are header-only: every call goes through a vtable fetched from the Eclipse
mod at runtime with a Geode event (`eclipse::__internal__::FetchVTableEvent`),
so this mod does not link against Eclipse and keeps working without it.

Do not edit these files here. To update, copy the whole `include/` directory
again and check that `eclipse::__internal__::API_VERSION` (in `events.hpp`)
did not change in a way that breaks the `VTable` layout.
