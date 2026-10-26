// Root module of the package: `import mcpplibs.mwrap.boost;`.
//
// This file is the package's interface root ([lib] path). It is committed and
// feature-independent; the re-export set cannot be, because a root that names a
// module whose feature is off would not compile. build.mcpp therefore emits a
// partition of THIS module into MCPP_OUT_DIR, holding `export import` for every
// module of the currently active feature set, and the line below re-exports it.
//
// The partition lives in the build's own output directory, never in the package
// root: two consumers building the same checkout with different features must
// not overwrite each other's closure. See docs/architecture.md ("root module").
export module mcpplibs.mwrap.boost;

export import :closure;
