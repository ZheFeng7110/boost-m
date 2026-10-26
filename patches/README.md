# patches

Replayable adaptations. `tools/reapply_hand_edits.py` applies them in two
rounds — vendored upstream first, regenerated module sources second — and is
idempotent in both directions (an applied patch is skipped by the reverse check).

| Set | Applies to | Reason |
|---|---|---|
| `*.patch` listed in `VENDORED_PATCHES` | `upstream/boost/**` | Minimal upstream adaptations: TU-local objects promoted to `inline constexpr` so a CMI and a consumer GMF cannot emit the same mangled symbol twice; internal-linkage/static helpers given external linkage for the export surface; platform guards for the vendored 1.92.0 tree; the `system` split that moves function-local statics out of headers. |
| `*.patch` listed in `SRC_PATCHES` | `src/**`, `generated/exports/**` | Module-side adaptations the generator cannot know: extra GMF includes, platform guards on exported entities, export-ownership moves, explanatory comments kept in the generated lists. |
| `final/<path>` | the same paths | The hand-maintained final form of files whose content intentionally diverges from generator output. Stored as whole files, not diffs, so replay needs no Git history. |

Every upstream patch is minimal and is reviewed against the 1.92.0 tag it was
written for; `upstream/boost/SOURCE.md` records the archive and checksum the tree
comes from. A patch whose anchor is missing fails the replay unless it is listed
as tolerant in `tools/reapply_hand_edits.py` with a written reason (`json`,
`core`, `io` — transitional no-ops superseded by stored final forms).

Replacing a `final/` entry is an interface change: apply the edit, then copy the
edited file over the stored form so the replay keeps reproducing it.
