// mcpplibs.mwrap.boost.version — thin entry that re-exports the mapped version
// constants. The mapping itself is owned by mcpplibs.mwrap.boost.macros: one
// entity, one export owner. `version` implies `macros` in mcpp.toml.
export module mcpplibs.mwrap.boost.version;

export import mcpplibs.mwrap.boost.macros;
