# Progression Lab

Open-source chord progression plugin (VST3 / AU / CLAP / Standalone, built with JUCE).
The user picks a genre, key and length; the plugin generates a chord progression they can
preview and drag into their DAW as MIDI. Users can keep/discard results to train a genre,
create their own genres from sample chords, and blend genres.

**The full build plan is in `docs/ROADMAP.md`. Read it before starting any phase.**
Where this file and the roadmap disagree, this file wins (see "Overrides" below).

## About the developer

- Eli, CS student. Solid on general programming and C++ basics, new to JUCE and audio plugins.
- Explain JUCE-specific concepts briefly the first time they come up (threads, processBlock,
  editors, BinaryData). Don't over-explain general C++.
- Prefers direct, concise answers and honest assessments.
- Editor: Neovim (LazyVim, clangd). Do not suggest Xcode as an editor.
- Primary DAW: FL Studio (VST3). Secondary test DAW: Reaper.
- Machine: Apple Silicon Mac (arm64), zsh.

## Current status

- Phase 0 (tools and repo): done. Plugin builds, loads in FL Studio as a generator,
  VST3 passes pluginval at strictness 5, local Catch2 tests pass.
- CI: fixed for Windows (LLVM upgrade) and unsigned builds; confirm all three jobs are green.
- **Next: Phase 1** — hard-coded I–V–vi–IV progression with MIDI drag-out, tested in FL Studio first.

Update this section as phases complete.

## Build, test, validate

```bash
# configure (rerun only when CMakeLists.txt changes)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
# build (copies plugins into ~/Library/Audio/Plug-Ins automatically)
cmake --build build
# tests
ctest --test-dir build --output-on-failure
# validate (alias set in ~/.zshrc)
pluginval --strictness-level 5 --validate ~/Library/Audio/Plug-Ins/VST3/Progression\ Lab.vst3
```

- Before rebuilding, the plugin must be removed from FL Studio's Channel rack or FL closed.
- Debug with the Standalone app or JUCE's AudioPluginHost under codelldb. A debugger cannot
  attach to FL Studio (hardened runtime).
- `auval` is broken on this Mac for **all** third-party AUs (it doesn't list FabFilter etc.
  either). Don't try to fix local AU validation; CI validates AU on a clean runner.
- The libc++ "selected platform is no longer supported" warnings are harmless.

## Repo layout

```
source/                 C++ plugin
  PluginProcessor.*     audio thread, MIDI, state
  PluginEditor.*        UI
  engine/               Markov generator, blending, voicing — pure C++, NO JUCE UI code
  data/                 SQLite access (base.db + user.db)
  midi/                 MIDI export, chord parsing/detection
tests/                  Catch2 tests (PluginBasics.cpp is from the template)
benchmarks/             Catch2 benchmarks
assets/                 files here are embedded into BinaryData automatically
pipeline/               offline Python: ingest, normalize, build_db, notebooks
data/raw/               downloaded datasets (gitignored)
data/seeds/             hand-written seed progressions per genre (committed)
data/build/             pipeline output (base.db)
docs/ROADMAP.md         full phase-by-phase plan
JUCE/, modules/, cmake/ git submodules — never edit these
```

## Fixed decisions (don't change without asking)

- Python never ships to users. It runs offline to build `base.db`. The plugin is pure C++.
- v1 generator is a variable-order Markov model with backoff, not a neural net.
  Blending = weighted mix of genres' next-chord probabilities.
- Chords are stored relative to the key: `degree` = semitones above tonic (0–11), plus
  `quality` and optional `bass_degree`. Convert to Roman numerals only for display.
- Two SQLite files: `base.db` (read-only, embedded, replaced on update) and `user.db`
  (user data, never overwritten, migrated via `PRAGMA user_version`).
- Genre IDs are string slugs (`lofi`) / UUIDs for user genres, never integer IDs across DBs.
- Plugin is an instrument (`IS_SYNTH TRUE`) with MIDI in/out and a built-in preview synth.
- Plugin codes are permanent: manufacturer `Plab`, plugin `Prlb`. Never change them.

## Overrides to docs/ROADMAP.md

The roadmap was written before the repo existed. Actual setup differs:

- **JUCE version is 9.0.1**, not 8. The roadmap's license note (AGPLv3) was for JUCE 8.
  Check `JUCE/LICENSE.md` before choosing the repo license. LICENSE is still the template's.
- **C++23**, set by `pamplejuce_shared_code_defaults()` (roadmap says C++20).
- **Embedding data (Step 3.1):** don't use `juce_add_binary_data`. Put `base.db` and
  `schema.sql` in `assets/`; `pamplejuce_add_assets()` embeds them and the `Assets` target
  is already linked. Access via `BinaryData::base_db`, `BinaryData::schema_sql`.
- **Adding dependencies:** use CPM (already included), not FetchContent:
  `CPMAddPackage("gh:SRombauts/SQLiteCpp#3.3.2")`, then add `SQLiteCpp` to
  `target_link_libraries(SharedCode INTERFACE ...)` in CMakeLists.txt.
- **All plugin code links through the `SharedCode` INTERFACE target**, so tests can use it.
  New `.cpp`/`.h` files under `source/` are picked up automatically by the glob.

## Code rules

- Audio thread (`processBlock`): no allocation, no locks, no file/DB access, no strings.
  Pass data to it via fixed-size structs through a lock-free FIFO (roadmap Step 5.2).
- DB access and model building happen on background threads; generation (sub-ms) can run
  on the message thread.
- Keep `source/engine/` free of JUCE UI dependencies so it's unit-testable.
- Every engine feature gets a Catch2 test in `tests/`.
- Run pluginval after changes that touch the processor, state or parameters.

## Git

- Commit with conventional-commit messages and a detailed body (this repo is a portfolio
  piece recruiters may read). Format:
  ```
  feat: short summary in imperative mood

  - What changed and why, one bullet per meaningful change
  ```
  Types: feat, fix, refactor, test, docs, ci, chore.
- Only commit or push when Eli asks.
- Never commit `build/`, `data/raw/`, `.venv/`, or anything with secrets.
- Git identity must be the EliA437 GitHub account.
