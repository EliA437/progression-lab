# Progression Lab — Build Roadmap

Nov 22, 2026 · @Eli Allen

## Where things stand

No code exists yet. The design is settled, so the next step is Phase 0: a JUCE plugin that loads in your DAW. Plan on 12 to 16 weeks part-time to reach a shippable v1 (Phases 0 to 6). The neural model (Phase 7) is optional.

**Decisions already locked in from the planning chats:**

- **Python never ships to users.** It runs only on your machine to clean data and build `base.db`. The plugin is pure C++ (JUCE 8, C++20).
- **v1 engine is a Markov model**, not a neural net. "Training" means counting chord transitions. It works instantly with 10 to 50 user samples, and blending genres is a weighted average of their tables.
- **Two SQLite files per user.** `base.db` holds the stock genres, is read-only and is embedded in the plugin. `user.db` holds custom genres and accepted progressions, and updates never overwrite it.
- **Chords are stored relative to the key** (scale degree + quality), so one progression counts for all 12 keys.
- **Plugin type is an instrument** with a built-in preview synth and MIDI drag-out. Formats: VST3 and AU, plus Standalone for quick testing.
- **Data:** Chordonomicon for statistics (license unconfirmed), McGill Billboard (CC0) and ChoCo CC BY subsets to ship, plus your own seed progressions for trap, lo-fi and similar genres.

**How to use this roadmap:** do the phases in order. Each phase ends with a "Done when" test. Don't start the next phase until that test passes. Phase 1 (drag-out) comes before the data work on purpose: it's the riskiest part, and if it fails in a DAW, nothing else matters.

**One correction to an earlier chat:** JUCE 8's open-source license is AGPLv3, not GPLv3. License your repo AGPLv3 to match. Selling a closed-source version later needs a paid JUCE license.

## How the pieces fit

```
YOUR DEV MACHINE (Python, offline only)
  Chord datasets ──► Python pipeline ──► base.db
  (Chordonomicon,    (parse, detect key,  (stock genres,
   Billboard, ChoCo,  normalize to         SQLite, built by
   your seeds)        degrees)             build_db.py)
                                             │
                                             ▼
                          Build and release (CMake embeds base.db,
                          CI runs pluginval, GitHub Releases)
                                             │ installer
                                             ▼
USER'S COMPUTER (inside the DAW, no Python)
  Plugin UI  ◄──► DAW track (.mid drag-out, MIDI import)
     ▲▼
  Generator engine (Markov + blending, voicing, chord detection) ──► Preview synth (audio thread)
     ▲▼
  SQLite layer (base.db copy read-only + user.db, SQLiteCpp, WAL)
```

Data flows one way from your machine to the user's. After install, everything loops inside the plugin: UI asks the generator, the generator reads and writes SQLite, and MIDI goes to and from the DAW.

## Phase 0: Tools and repo (2 to 4 days)

Goal: an empty Progression Lab plugin that builds with one command and loads in your DAW.

### Step 0.1: Install the toolchain (macOS, Neovim, FL Studio)

```bash
xcode-select --install          # compilers, if not already installed
brew install cmake ninja python@3.12 git
```

- **Xcode app:** still install it from the App Store and open it once. You write code in Neovim, but AU builds and `auval` need the Xcode SDK.
- **Neovim (LazyVim):** in `:LazyExtras`, enable `lang.clangd`, `lang.cmake`, `lang.python` and `dap.core`. Then in `:Mason`, check that `clangd`, `codelldb`, `neocmakelsp` and `ruff` are installed.
- **DAWs:** FL Studio is your main DAW. Add **Reaper** (free 60-day trial) as a second one, because most of your users won't be on FL. Ableton and Logic are optional; without Logic, `auval` is your only AU check.

### Step 0.2: Start from the Pamplejuce template

Pamplejuce is a JUCE CMake template that already includes Catch2 tests, GitHub Actions CI, pluginval and macOS/Windows packaging. It saves you most of Phase 6.

1. Go to `github.com/sudara/pamplejuce` and click **Use this template**. Name the repo `progression-lab`.
2. Clone it with its submodules (JUCE is a submodule):

```bash
git clone --recurse-submodules git@github.com:<you>/progression-lab.git
cd progression-lab
```

3. Open `CMakeLists.txt` and set the plugin identity. Look for the `juce_add_plugin` call and set these fields:

```cmake
juce_add_plugin("${PROJECT_NAME}"
    COMPANY_NAME "Progression Lab"
    PRODUCT_NAME "Progression Lab"
    BUNDLE_ID com.progressionlab.plugin
    PLUGIN_MANUFACTURER_CODE Plab   # 4 chars, at least one uppercase
    PLUGIN_CODE Prlb                # 4 chars, unique to this plugin
    IS_SYNTH TRUE                   # instrument: works in every DAW
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    FORMATS AU VST3 Standalone
    COPY_PLUGIN_AFTER_BUILD TRUE)   # copies into ~/Library/Audio/Plug-Ins
```

Don't change the manufacturer or plugin code after people install it. DAWs use them to identify the plugin in saved projects.

### Step 0.3: Lay out the repo

Add these folders next to Pamplejuce's `source/`:

```
progression-lab/
├── source/            # C++ plugin (Pamplejuce's folder)
│   ├── PluginProcessor.cpp/.h   # audio thread, MIDI, state
│   ├── PluginEditor.cpp/.h      # UI
│   ├── engine/        # Markov, blending, voicing (pure C++, no JUCE UI)
│   ├── data/          # SQLite access
│   └── midi/          # MIDI file export, chord detection
├── tests/             # Catch2 tests
├── pipeline/          # Python: ingest, normalize, build_db, prototype
│   ├── requirements.txt
│   └── notebooks/
├── data/
│   ├── raw/           # downloaded datasets (gitignored)
│   ├── seeds/         # your hand-written progressions (committed)
│   └── build/base.db  # pipeline output, embedded in the plugin
├── docs/
├── CREDITS.md         # dataset attributions (CC BY requires it)
└── LICENSE            # AGPLv3
```

Keep `engine/` free of JUCE UI code. Then you can unit-test it and reuse it later.

### Step 0.4: Build and load it

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

- **CMake** reads `CMakeLists.txt` and plans the build; rerun the first command only when that file changes. **Ninja** runs the plan and recompiles only changed files; run the second command after every code change.
- `compile_commands.json` lands in `build/`, and clangd finds it there. Open `source/PluginProcessor.cpp` in Neovim and check there are no red JUCE include errors. The first index takes a minute or two.
- Add a build keymap in `lua/config/keymaps.lua`: `vim.keymap.set("n", "<leader>cb", "<cmd>!cmake --build build<cr>", { desc = "Build plugin" })`.
- **Load it in FL Studio:** Options → Manage plugins → **Find installed plugins**, then add Progression Lab (VST3) from the Channel rack as a generator. Do the same in Reaper.
- **Before rebuilding,** remove the plugin from FL's Channel rack or close FL. Overwriting it while it's loaded can crash FL or leave the old version running.

On Mac, check the AU too:

```bash
auval -a | grep -i plab               # should list your plugin
auval -v aumu Prlb Plab               # full validation; must end in PASS
```

### Step 0.5: Install pluginval

Download it from `github.com/Tracktion/pluginval/releases` and run it on your build:

```bash
pluginval --strictness-level 5 --validate ~/Library/Audio/Plug-Ins/VST3/Progression\ Lab.vst3
```

Run it after every meaningful change. Raise the strictness to 10 before release.

### Step 0.6: Set up debugging

Don't attach a debugger to FL Studio. macOS's hardened runtime blocks it, from Neovim or Xcode. Debug in one of these instead:

1. **Standalone app (default).** Launch it from Neovim with codelldb's "Launch file" config. The binary is in `build/` under `…_artefacts/Debug/Standalone/Progression Lab.app/Contents/MacOS/Progression Lab`; check the exact path. Toggle breakpoints with `<leader>db`, continue with `<leader>dc`.
2. **JUCE AudioPluginHost** for host behavior (playhead, tempo, MIDI in and out). Build it from `JUCE/extras/AudioPluginHost` and run it under codelldb. It's unsigned, so the debugger can attach.
3. **FL Studio** for real-world testing only. For FL-only bugs, log to a file with `juce::FileLogger` and watch it with `tail -f`.

Also add a `.clang-format` at the repo root (`BasedOnStyle: LLVM`, `IndentWidth: 4`) and a `.clangd` file that hides diagnostics from `JUCE/`.

**Done when:** the empty plugin builds with one command, opens in FL Studio and Reaper, clangd works in Neovim, the Standalone app runs under codelldb, and passes `auval` and pluginval.

## Phase 1: Prove MIDI drag-out (about 1 week)

Goal: a hard-coded I–V–vi–IV in C that you can drag from the plugin onto a DAW track as a MIDI clip.

### Step 1.1: Define the core types

Create `source/engine/Chord.h`. Every later phase uses these types, so get them right now.

```cpp
#pragma once
#include <vector>
#include <string>

namespace pl {

// A chord relative to the key. degree = semitones above the tonic (0-11).
struct Chord {
    int degree = 0;              // 0 = I, 7 = V, 9 = vi, 5 = IV
    std::string quality = "maj"; // maj, min, dim, aug, sus2, sus4, 7, maj7, min7, m7b5, dim7, 9, ...
    int bassDegree = -1;         // -1 = root position; otherwise slash-chord bass
    double beats = 4.0;          // duration
};

using Progression = std::vector<Chord>;

struct Key { int tonic = 0; bool minor = false; }; // tonic 0 = C, 1 = C#, ...

}
```

Storing the degree as semitones (0 to 11) instead of Roman numeral strings keeps the maths simple. Convert to Roman numerals only for display.

### Step 1.2: Turn chords into notes

Create `source/engine/ChordNotes.h`. This is a deliberately basic voicing. Phase 5 replaces it.

```cpp
inline std::vector<int> intervalsFor(const std::string& q) {
    if (q == "min")  return {0, 3, 7};
    if (q == "dim")  return {0, 3, 6};
    if (q == "aug")  return {0, 4, 8};
    if (q == "sus2") return {0, 2, 7};
    if (q == "sus4") return {0, 5, 7};
    if (q == "7")    return {0, 4, 7, 10};
    if (q == "maj7") return {0, 4, 7, 11};
    if (q == "min7") return {0, 3, 7, 10};
    if (q == "m7b5") return {0, 3, 6, 10};
    return {0, 4, 7}; // maj
}

inline std::vector<int> midiNotes(const pl::Chord& c, const pl::Key& k, int octave = 4) {
    int root = 12 * (octave + 1) + (k.tonic + c.degree) % 12; // C4 = 60
    std::vector<int> notes;
    for (int i : intervalsFor(c.quality)) notes.push_back(root + i);
    return notes;
}
```

### Step 1.3: Write the progression to a .mid file

Create `source/midi/MidiExport.cpp`. Use 960 ticks per quarter note; every DAW handles it.

```cpp
juce::File writeProgressionToMidi(const pl::Progression& prog, const pl::Key& key) {
    constexpr int ppq = 960;
    juce::MidiMessageSequence seq;
    double tick = 0.0;

    for (const auto& chord : prog) {
        double len = chord.beats * ppq;
        for (int note : midiNotes(chord, key)) {
            seq.addEvent(juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), tick);
            seq.addEvent(juce::MidiMessage::noteOff(1, note), tick + len - 1); // -1 avoids overlap
        }
        tick += len;
    }
    seq.updateMatchedPairs();

    juce::MidiFile midi;
    midi.setTicksPerQuarterNote(ppq);
    midi.addTrack(seq);

    auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getChildFile("ProgressionLab")
                    .getChildFile("Progression_" + juce::String(juce::Time::currentTimeMillis()) + ".mid");
    file.getParentDirectory().createDirectory();

    juce::FileOutputStream out(file);
    if (out.openedOk()) midi.writeTo(out);
    return file;
}
```

The timestamp in the name matters. Some DAWs cache a dropped file by path, so reusing one name can drop an old clip.

### Step 1.4: Make a draggable handle

Create a small `DragHandle` component. The drag must start inside `mouseDrag`, not on a button click.

```cpp
class DragHandle : public juce::Component {
public:
    std::function<juce::File()> makeFile; // set by the editor

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colours::darkslategrey);
        g.setColour(juce::Colours::white);
        g.drawFittedText("Drag MIDI", getLocalBounds(), juce::Justification::centred, 1);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (dragging || e.getDistanceFromDragStart() < 6) return;
        dragging = true;
        auto f = makeFile();
        juce::DragAndDropContainer::performExternalDragDropOfFiles(
            { f.getFullPathName() }, false, this, [this] { dragging = false; });
    }

    void mouseUp(const juce::MouseEvent&) override { dragging = false; }

private:
    bool dragging = false;
};
```

In `PluginEditor`, add a `DragHandle`, call `addAndMakeVisible`, and set `makeFile` to return `writeProgressionToMidi({{0,"maj"},{7,"maj"},{9,"min"},{5,"maj"}}, {0,false})`.

### Step 1.5: Test the drag in FL Studio, then Reaper

Test FL Studio first. It's the DAW where drag-out is most likely to need work.

| Where you drop in FL Studio | Expected result |
| --- | --- |
| Playlist | A new pattern clip holding the progression |
| Piano roll of a channel | The notes land in that pattern |
| Channel rack | A new channel with the MIDI (FL may show an import dialog) |

Then repeat in Reaper (drop onto an empty track).

- If FL rejects the drop or imports it wrong, build a **Save MIDI…** button with `juce::FileChooser` now. It's required, not optional: dragging a saved `.mid` from Finder into FL always works.
- Check the clip length in FL. A wrong ticks-per-quarter value shows up there as stretched or squashed notes.

**Done when:** the clip lands with four correct chords at the correct length in FL Studio (Playlist and Piano roll) and Reaper, and pluginval still passes.

## Phase 2: Python data pipeline (1 to 2 weeks, can overlap Phase 1)

Goal: a `base.db` with 8 stock genres, plus a Python Markov prototype whose output you've listened to and judged genre-like.

### Step 2.1: Set up the Python environment

```bash
cd pipeline
python3.12 -m venv .venv && source .venv/bin/activate
pip install pandas pyarrow datasets mido pytest jupyter
pip freeze > requirements.txt
```

Add `data/raw/` and `pipeline/.venv/` to `.gitignore`. Never commit raw datasets.

### Step 2.2: Pick the v1 genres

Start with 8. More genres means more seed-writing.

| Genre (slug) | Main source | Seeds you write |
| --- | --- | --- |
| pop | Chordonomicon, Billboard | 0 to 10 |
| rock | Chordonomicon, Billboard | 0 to 10 |
| rnb\_soul | Chordonomicon, Billboard | 10 to 20 |
| jazz | Chordonomicon, ChoCo | 10 to 20 |
| edm | Chordonomicon | 20 to 30 |
| neo\_soul | Your seeds | 30 to 50 |
| lofi | Your seeds | 30 to 50 |
| trap | Your seeds | 30 to 50 |

Use the slug as the genre's permanent ID in both databases. Integer IDs would clash between `base.db` and `user.db`.

### Step 2.3: Download the datasets

- **Chordonomicon:** `from datasets import load_dataset; ds = load_dataset("ailsntua/Chordonomicon")`. Run `df.columns` and look at a few rows first. Expect a `chords` string with section tags like `<verse_1>`, plus genre columns.
- **McGill Billboard:** download the chord annotations from the DDMAL Billboard page into `data/raw/billboard/`.
- **ChoCo:** clone `github.com/smashub/choco`. Read its `LICENSE.md` and keep only the CC BY subsets.
- **Email the Chordonomicon authors** this week to confirm the license. Until they reply, use it only to compute statistics, not as rows you ship.

### Step 2.4: Write your seed files

One text file per genre in `data/seeds/`. One progression per line, key first:

```
# data/seeds/lofi.txt
key=Dm | Dm9 G13 Cmaj9 A7b9
key=F  | Fmaj7 Em7 Dm7 Cmaj7
key=Am | Am9 Fmaj7 Cmaj7 E7#9
```

This is the part that makes lo-fi sound like lo-fi. Pull from songs you know and from your own playing. Seeds are yours, so there's no license question.

### Step 2.5: Parse chord symbols

Create `pipeline/chords.py`. Its job: turn any symbol into a root pitch class, a canonical quality, and a bass note.

```python
import re

NOTE = {'C':0,'C#':1,'Db':1,'D':2,'D#':3,'Eb':3,'E':4,'F':5,'F#':6,'Gb':6,
        'G':7,'G#':8,'Ab':8,'A':9,'A#':10,'Bb':10,'B':11,'Cb':11}

# Order matters: longest match first. Map every spelling to ~15 canonical qualities.
QUALITY = [('m7b5','m7b5'),('maj9','maj9'),('maj7','maj7'),('maj','maj'),('min9','min9'),('m9','min9'),
           ('min7','min7'),('m7','min7'),('dim7','dim7'),('dim','dim'),('aug','aug'),
           ('sus2','sus2'),('sus4','sus4'),('min','min'),('m','min'),('13','13'),
           ('11','11'),('9','9'),('7','7'),('','maj')]

SYM = re.compile(r'^([A-G][#b]?)([^/]*)(?:/([A-G][#b]?))?$')

def parse(symbol: str):
    m = SYM.match(symbol.strip())
    if not m: return None
    root, rest, bass = m.groups()
    quality = next(q for spelling, q in QUALITY if rest.startswith(spelling))
    return NOTE[root], quality, (NOTE[bass] if bass else None)
```

Write a pytest file with 30+ symbols from each dataset. Each dataset spells chords differently (Billboard and ChoCo use Harte syntax like `C:maj7`), so add a small adapter per source that converts to this format first.

### Step 2.6: Detect the key

Chordonomicon has no key labels, so estimate one. Score all 24 keys by how many chord tones fall in each scale, with extra weight for the first and last chord:

```python
MAJOR = {0,2,4,5,7,9,11}; MINOR = {0,2,3,5,7,8,10}

def detect_key(chords):  # chords = list of (root, quality, bass)
    best = None
    for tonic in range(12):
        for minor, scale in ((False, MAJOR), (True, MINOR)):
            s = 0.0
            for i, (root, q, _) in enumerate(chords):
                w = 2.0 if i in (0, len(chords) - 1) else 1.0
                tones = [(root + iv) % 12 for iv in (0, 3 if q.startswith(('min','m7b5','dim')) else 4, 7)]
                s += w * sum(((t - tonic) % 12) in scale for t in tones)
                if (root - tonic) % 12 == 0: s += w  # chord on the tonic
            if best is None or s > best[0]: best = (s, tonic, minor)
    return best[1], best[2]
```

Spot-check 50 songs by hand. If it's wrong more than about 15% of the time, switch to Krumhansl key profiles.

### Step 2.7: Normalize and clean

Create `pipeline/normalize.py`. For every song:

1. Split on section tags (`<verse_1>`, `<chorus_1>`). Each section is a candidate progression.
2. Detect the key for the whole song, then convert each chord to `degree = (root - tonic) % 12`.
3. Collapse immediate repeats (`C C G G` becomes `C G`).
4. Cut into loops of 4 to 8 chords. Drop sections with fewer than 3 distinct chords.
5. Drop exact duplicates within a genre.
6. Map the dataset's genre labels onto your 8 slugs with a dictionary. Drop songs that don't map.

Aim for at least 2,000 progressions per data-backed genre. Fewer than 500 means the Markov output will repeat itself.

### Step 2.8: Build base.db

Create `pipeline/build_db.py`. The plugin uses this same schema for `user.db`, so keep both in one file you can share: `data/schema.sql`.

```sql
-- data/schema.sql
PRAGMA user_version = 1;

CREATE TABLE genres (
  slug        TEXT PRIMARY KEY,          -- 'lofi', or a UUID for user genres
  name        TEXT NOT NULL,
  source      TEXT NOT NULL CHECK (source IN ('base','user','blend')),
  created_at  INTEGER NOT NULL
);

CREATE TABLE progressions (
  id          INTEGER PRIMARY KEY,
  genre_slug  TEXT NOT NULL,
  mode        TEXT NOT NULL CHECK (mode IN ('major','minor')),
  origin      TEXT NOT NULL CHECK (origin IN ('base','seed','sample','accepted')),
  created_at  INTEGER NOT NULL
);

CREATE TABLE chords (
  progression_id INTEGER NOT NULL REFERENCES progressions(id) ON DELETE CASCADE,
  position    INTEGER NOT NULL,
  degree      INTEGER NOT NULL,          -- 0-11, semitones above tonic
  quality     TEXT NOT NULL,
  bass_degree INTEGER,                   -- NULL = root position
  beats       REAL NOT NULL DEFAULT 4,
  PRIMARY KEY (progression_id, position)
);

CREATE TABLE blends (
  genre_slug  TEXT NOT NULL,
  parent_slug TEXT NOT NULL,
  weight      REAL NOT NULL
);

CREATE INDEX idx_prog_genre ON progressions(genre_slug);
```

The script reads `schema.sql`, inserts the genres, then inserts every progression and its chords in one transaction. Run `VACUUM` at the end to keep the file small. Check the size; under 20 MB is fine to embed.

### Step 2.9: Prototype the Markov generator in Python

Create `pipeline/notebooks/markov.ipynb`. A token is `"degree:quality"` (plus `/bass` if present). Each progression starts with a mode token, so the first chord depends on major or minor.

```python
import random
from collections import defaultdict, Counter

def train(progressions):  # list of (mode, [tokens])
    c2, c1 = defaultdict(Counter), defaultdict(Counter)
    for mode, toks in progressions:
        seq = [f'<{mode}>'] + toks
        for i in range(1, len(seq)):
            c1[seq[i-1]][seq[i]] += 1
            if i >= 2: c2[(seq[i-2], seq[i-1])][seq[i]] += 1
    return c2, c1

def sample(counter, temperature=1.0):
    toks, counts = zip(*counter.items())
    weights = [c ** (1.0 / temperature) for c in counts]
    return random.choices(toks, weights)[0]

def generate(model, mode='minor', length=4, temperature=1.0, min_count=3):
    c2, c1 = model
    seq = [f'<{mode}>']
    while len(seq) < length + 1:
        ctx = tuple(seq[-2:])
        if len(seq) >= 2 and sum(c2[ctx].values()) >= min_count:
            seq.append(sample(c2[ctx], temperature))   # 2nd order
        else:
            seq.append(sample(c1[seq[-1]], temperature))  # back off to 1st order
    return seq[1:]
```

Add a helper that writes the result to a `.mid` file with `mido`, then listen in your DAW. **Blending** in the prototype: merge the genres' counters after scaling each by its weight divided by its total count.

### Step 2.10: The listening test

Generate 20 progressions per genre and listen to them all. Then mix 10 from two genres, shuffle them, and guess which genre each came from. If you can't tell lo-fi from pop, fix the data (more seeds, better genre mapping) before writing any C++.

**Done when:** `base.db` builds from one command (`python build_db.py`), its tests pass, and you can tell the genres apart by ear.

## Phase 3: SQLite and the Markov engine in C++ (1 to 2 weeks)

Goal: pick a genre, key and length, hit Generate, and drag out a real generated progression.

### Step 3.1: Add SQLiteCpp and embed the data

In `CMakeLists.txt`, after JUCE is added:

```cmake
include(FetchContent)
FetchContent_Declare(SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG        3.3.2)          # check for the latest release tag
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
set(SQLITECPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SQLiteCpp)   # bundles sqlite3 itself

juce_add_binary_data(PLData SOURCES
    ${CMAKE_SOURCE_DIR}/data/build/base.db
    ${CMAKE_SOURCE_DIR}/data/schema.sql)

# Pamplejuce links everything through its shared target (SharedCode in recent versions)
target_link_libraries(SharedCode INTERFACE SQLiteCpp PLData)
```

In C++ you now have `BinaryData::base_db`, `BinaryData::base_dbSize` and `BinaryData::schema_sql`.

### Step 3.2: Set up the database files on first launch

Create `source/data/Database.cpp`. This runs once when the plugin loads.

```cpp
juce::File appDataDir() {
    auto base = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    base = base.getChildFile("Application Support");   // macOS returns ~/Library
   #endif
    auto dir = base.getChildFile("ProgressionLab");
    dir.createDirectory();
    return dir;
}

void Database::open() {
    auto dir = appDataDir();

    // 1. base.db: copy out of the binary if missing or out of date
    auto baseFile = dir.getChildFile("base.db");
    juce::MD5 embedded(BinaryData::base_db, (size_t) BinaryData::base_dbSize);
    if (!baseFile.existsAsFile() || juce::MD5(baseFile) != embedded)
        baseFile.replaceWithData(BinaryData::base_db, (size_t) BinaryData::base_dbSize);

    // 2. user.db: create once, never overwrite
    auto userFile = dir.getChildFile("user.db");
    bool fresh = !userFile.existsAsFile();
    db = std::make_unique<SQLite::Database>(userFile.getFullPathName().toStdString(),
                                            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db->exec("PRAGMA journal_mode=WAL");   // several plugin instances can share it
    db->exec("PRAGMA busy_timeout=2000");
    db->exec("PRAGMA foreign_keys=ON");
    if (fresh) db->exec(std::string(BinaryData::schema_sql, (size_t) BinaryData::schema_sqlSize));
    else       migrate();                  // compare PRAGMA user_version, run upgrade steps

    // 3. Query both files through one connection
    SQLite::Statement attach(*db, "ATTACH DATABASE ? AS base");
    attach.bind(1, baseFile.getFullPathName().toStdString());
    attach.exec();
}
```

Now one query can read both: `SELECT ... FROM base.progressions UNION ALL SELECT ... FROM main.progressions`. Treat `base.*` as read-only in your code.

### Step 3.3: Load progressions into memory

At startup, read every progression once into `std::vector<StoredProgression>` (genre slug, mode, origin, chords). Even 200,000 progressions fit comfortably in memory. Do the load on a background thread (`juce::Thread` or a `juce::ThreadPool` job), then hand the result to the message thread with `juce::MessageManager::callAsync`. Show "Loading…" until it arrives.

### Step 3.4: Build the Markov model

Create `source/engine/Markov.h`. One model per genre. Tokens are strings like `"9:min7"` (degree, quality, optional `/bass`).

```cpp
class MarkovModel {
public:
    using Dist = std::unordered_map<std::string, double>;

    void add(const std::string& mode, const std::vector<std::string>& toks, double weight) {
        std::vector<std::string> seq { "<" + mode + ">" };
        seq.insert(seq.end(), toks.begin(), toks.end());
        for (size_t i = 1; i < seq.size(); ++i) {
            o1[seq[i-1]][seq[i]] += weight;
            if (i >= 2) o2[seq[i-2] + "|" + seq[i-1]][seq[i]] += weight;
        }
    }

    // Probabilities for the next token. Backs off to 1st order when the 2-chord context is rare.
    Dist next(const std::vector<std::string>& seq, double minCount = 3.0) const {
        if (seq.size() >= 2)
            if (auto it = o2.find(seq[seq.size()-2] + "|" + seq.back()); it != o2.end() && total(it->second) >= minCount)
                return normalise(it->second);
        if (auto it = o1.find(seq.back()); it != o1.end()) return normalise(it->second);
        return {};
    }

private:
    std::unordered_map<std::string, Dist> o1, o2;
    static double total(const Dist& d) { double t = 0; for (auto& [k, v] : d) t += v; return t; }
    static Dist normalise(const Dist& d) { Dist r; double t = total(d); for (auto& [k, v] : d) r[k] = v / t; return r; }
};
```

### Step 3.5: Generate, with blending built in

The generator takes a list of (model, weight) pairs. One genre is just a list of one. This makes blending free.

```cpp
struct GenParams { std::string mode = "minor"; int length = 4; double temperature = 1.0; };

std::vector<std::string> generate(const std::vector<std::pair<const MarkovModel*, double>>& mix,
                                  const GenParams& p, std::mt19937& rng) {
    std::vector<std::string> seq { "<" + p.mode + ">" };
    while ((int) seq.size() < p.length + 1) {
        MarkovModel::Dist blended;
        double wSum = 0;
        for (auto& [model, w] : mix) {
            auto d = model->next(seq);
            if (d.empty()) continue;           // this genre has never seen the context
            for (auto& [tok, prob] : d) blended[tok] += w * prob;
            wSum += w;
        }
        if (blended.empty()) break;            // dead end: caller rerolls

        std::vector<std::string> toks; std::vector<double> weights;
        for (auto& [tok, prob] : blended) {
            toks.push_back(tok);
            weights.push_back(std::pow(prob / wSum, 1.0 / p.temperature));
        }
        std::discrete_distribution<size_t> pick(weights.begin(), weights.end());
        seq.push_back(toks[pick(rng)]);
    }
    return { seq.begin() + 1, seq.end() };
}
```

Two improvements once the basic version works:

- **Loop bonus:** at the last chord, multiply each candidate's weight by how likely it is to lead back to the first chord. Progressions then loop cleanly, which producers need.
- **Novelty check:** if a result exactly matches a training progression, reroll up to 5 times. Users want new ideas, not copies.

### Step 3.6: Add the controls

- **Key and mode:** two `juce::ComboBox`es. Mode picks the start token.
- **Length:** 4 or 8 chords. **Chords per bar:** 1 or 2, which sets `beats`.
- **Temperature ("Weirdness"):** slider from 0.5 to 1.5. Lower picks the most common moves.
- **Complexity:** at low settings, simplify qualities after generating (`maj9` to `maj7` to `maj`, `min9` to `min7` to `min`). Generate first, simplify second.

Store all of these in an `AudioProcessorValueTreeState`, so the DAW saves them with the project. Also save the current progression in `getStateInformation`, so reopening a project shows the same chords.

### Step 3.7: Get the threading right

| Work | Thread | Why |
| --- | --- | --- |
| Loading the DB, building models | Background | Can take a second or more |
| Generating a progression | Message thread | Takes well under 1 ms |
| Writing to user.db | Background | Disk can stall |
| Playing chords (Phase 5) | Audio thread | Gets a copy of the progression only, never touches the DB or models |

Never lock a mutex or allocate memory on the audio thread. Phase 5 covers how to pass the progression across safely.

### Step 3.8: Test the engine

Add Catch2 tests in `tests/`: the token parser, a model trained on 3 progressions returning only tokens it saw, blending with weight 1.0/0.0 matching a single genre, and generated length always equal to the requested length.

**Done when:** Generate produces genre-like progressions in any key, blending two genres audibly mixes them, and reopening a saved DAW project restores the last progression.

## Phase 4: Learning loop and custom genres (2 weeks)

Goal: users can keep or discard generations, create their own genres from samples, and save blends.

### Step 4.1: Keep and discard

Add two buttons under the chord row.

- **Keep:** on a background thread, insert the progression into `main.progressions` with `origin = 'accepted'` and the current `genre_slug`, then its rows into `main.chords`. On the message thread, call `model.add(mode, tokens, acceptedWeight)` so the next generation reflects it right away.
- **Discard:** do nothing except generate a new one. Optional later: log discards to a separate table, opt-in only, as data for a future preference model.

This works for stock genres too. Accepted lo-fi progressions are saved under `lofi` in `user.db`, and `base.db` stays untouched.

### Step 4.2: Stop the model from feeding on itself

If accepted generations count as much as real data, a genre drifts toward its own outputs and gets repetitive. Weight each origin differently when building a model:

| Origin | Weight | Reason |
| --- | --- | --- |
| base | 1.0 | Large, real data |
| seed / sample (user's own) | 3.0 | Small but intentional, so it should count more |
| accepted | 0.5 | Generated by the model itself |

Also cap accepted progressions at 30% of a genre's total weight. If they go over, scale all accepted weights down to fit. Keep these numbers as constants in one place so you can tune them.

### Step 4.3: Create a custom genre

**New Genre** opens a panel: a name field and three ways to add samples. On save, insert a `genres` row with `source = 'user'` and a UUID slug (`juce::Uuid().toString()`), then the samples as `origin = 'sample'`.

Build the three input methods in this order, easiest first.

**1. Typed progressions.** Port `chords.py` to C++ (`source/midi/ChordParser.cpp`), reusing the same test cases. One line per progression, e.g. `Am7 Fmaj7 C G/B`. If the user doesn't give a key, run the C++ port of `detect_key`. Show the result as Roman numerals so the user can confirm.

**2. Pick from the library.** A searchable list of base progressions with checkboxes, filtered by genre. Ticked ones are copied in as samples.

**3. Drop in MIDI.** Make the panel a `juce::FileDragAndDropTarget` (`isInterestedInFileDrag` accepts `.mid`, `filesDropped` reads them). Then detect chords:

1. Read the file with `juce::MidiFile::readFrom`, merge tracks, and call `updateMatchedPairs()`.
2. Group notes whose start times fall within a 16th note of each other. Each group is one chord.
3. Convert the group to pitch classes. For each of the 12 roots and each quality template, score `matched notes - 0.5 × missing - 1.0 × extra`. Pick the best.
4. The lowest note sets the bass. If it isn't the root, it's a slash chord.
5. Detect the key over the whole file, then convert to degrees.

Prototype steps 2 to 5 in Python with `mido` against 10 MIDI files you know the chords for. Port to C++ only once it gets at least 9 of 10 right.

**Always show what was detected** ("Found 14 progressions, mostly minor"), with each chord editable before saving. A wrong chord saved silently poisons the genre.

### Step 4.4: Show genre strength

Count the weighted samples in a genre and show a badge next to it:

| Weighted samples | Badge |
| --- | --- |
| Under 10 | Learning: add more samples |
| 10 to 29 | Usable |
| 30 or more | Solid |

For stock genres, show the accepted count instead: "Lo-fi · 23 learned".

### Step 4.5: Reset to stock

A per-genre menu item. It runs `DELETE FROM main.progressions WHERE genre_slug = ? AND origin = 'accepted'` (chords go too, through `ON DELETE CASCADE`), then rebuilds that genre's model. For a custom genre, the same query resets it to the user's own samples. Ask for confirmation first; this can't be undone.

### Step 4.6: Blend mode and Save as Genre

Blend mode shows 2 or 3 genre pickers with weight sliders. Normalize the weights so they sum to 1, then pass them straight to `generate()` from Phase 3.

**Save as Genre** inserts a genre with `source = 'blend'` and one `blends` row per parent. When generating from a saved blend, mix the parents with the blend's own model (its accepted progressions). Give the blend's own model a weight that grows with its accepted count, e.g. `own = min(0.5, accepted / 100)`, so it slowly develops a style of its own.

### Step 4.7: Export and import genres

A `.plgenre` file is plain JSON, so it's easy to share on GitHub:

```json
{
  "format": 1,
  "name": "Dark Ambient Soul",
  "progressions": [
    { "mode": "minor", "chords": [ {"degree": 0, "quality": "min9", "beats": 4},
                                   {"degree": 8, "quality": "maj7", "beats": 4} ] }
  ]
}
```

Use `juce::JSON::toString` and `juce::JSON::parse`. On import, give the genre a new UUID slug so it can't overwrite an existing one, and reject files whose `format` is newer than you support.

**Done when:** you can create a genre from typed chords, MIDI and library picks; keeping progressions visibly changes later output; and reset, blend-save, export and import all work after a DAW restart.

## Phase 5: Voicing, preview synth and UI polish (2 weeks)

Goal: progressions sound like a player voiced them, play back in sync with the DAW, and the UI is something you'd want to use daily.

### Step 5.1: Build the voicing engine

This step does more for sound quality than anything else in the project. The Markov model picks *which* chords; the voicing engine picks *which notes*. Create `source/engine/Voicing.cpp`.

First extend the interval table with extensions: 9th = 14, 11th = 17, 13th = 21 semitones above the root. Then, for each chord:

1. **Generate candidates.** Every inversion of the upper notes inside a range (about E3 to G5), plus spread variants for the chosen style.
2. **Score each candidate** against the previous chord's voicing:
   - Voice-leading cost: sort both voicings and sum how far each voice moves. Smaller is better.
   - Penalty for intervals under 5 semitones below about E3, which sound muddy.
   - Penalty for doubling the 3rd, and for the top note jumping more than 5 semitones.
3. **Pick the cheapest.** The first chord uses a fixed, centred voicing.
4. **Add the bass separately**, one or two octaves below, playing the root or the slash-chord bass.

Offer four styles, with a default per genre:

| Style | What it does | Default for |
| --- | --- | --- |
| Close | All upper notes within an octave | pop, rock, edm |
| Open | Drop-2: second-highest note dropped an octave | rnb\_soul, gospel |
| Spread | Root low, 3rd and 7th in the middle, extensions on top | lofi, neo\_soul |
| Rootless | Leave out the root, voice 3-7-9 or 7-3-13 | jazz |

The same voicing function feeds the MIDI export, so dragged clips sound exactly like the preview.

### Step 5.2: Build the preview synth

Use `juce::Synthesiser` with one simple voice class: a saw or triangle oscillator, a low-pass filter (`juce::dsp::StateVariableTPTFilter`) and a `juce::ADSR` envelope. Eight voices is enough. Call `synth.renderNextBlock` in `processBlock`.

**Getting the progression to the audio thread safely:**

1. On the message thread, turn the voiced progression into a fixed-size `Schedule` struct: `std::array` of up to 64 note events (start beat, length, pitch), plus a count. No `std::vector`, no strings.
2. Push it through a `juce::AbstractFifo`-backed queue of `Schedule`s.
3. At the start of each `processBlock`, the audio thread pops the newest one and keeps it as its current schedule.

The audio thread never allocates, never locks and never touches the database.

**Syncing to the DAW:** in `processBlock`, read `getPlayHead()->getPosition()`. When `getIsPlaying()` is true, use `getPpqPosition()` (in beats) modulo the progression length to decide which notes sound. When the DAW is stopped, a **Preview** button runs an internal clock at the host's tempo (`getBpm()`), or 90 BPM if none.

Also write the notes into `processBlock`'s `midiMessages`. Users whose DAW supports MIDI routing can then play the chords through any other instrument.

In FL Studio, users route this by setting the plugin wrapper's **MIDI output port** and matching it on the target instrument's input port. Test that routing in FL.

### Step 5.3: Per-chord editing and locks

- **Reroll one chord:** click a chord block, regenerate only that position with the same context, and keep candidates whose transition into the next chord has non-zero probability.
- **Lock:** a padlock on each block. During generation, a locked position appends its locked token instead of sampling. Reroll fills only the unlocked ones.
- **Inversion nudge:** up/down arrows on a block shift its voicing one inversion. Store the offset on the chord.

### Step 5.4: Lay out the main screen

One screen, top to bottom:

1. **Top bar:** genre selector (with strength badge), Blend toggle, key, mode, length.
2. **Chord row:** 4 or 8 large blocks. Each shows the chord name (`Fm9`) and the Roman numeral under it (`i9`). Spell names with flats or sharps to match the key (`B♭` in F, `A♯` in B).
3. **Controls:** Weirdness, Complexity, Voicing style, Chords per bar.
4. **Action bar:** Generate, Keep, Discard, Preview, and the Drag MIDI handle.

Write a custom `juce::LookAndFeel_V4` subclass for colours and fonts, and make the editor resizable (`setResizable(true, true)` with limits). Generate a progression the moment the plugin opens, so there's never an empty screen.

### Step 5.5: Help users pick a key

Most DAWs don't send the project key to plugins, so a fully automatic "Match my project" isn't possible. Offer two things instead: a **Detect key** drop zone where users drop any MIDI clip (reuse the Phase 4 key detection), and a remembered default key per user in `user.db`.

**Done when:** a friend who produces can open it cold, get a progression they like into their DAW in under a minute, and says it sounds played, not programmed.

## Phase 6: CI, testing and release (1 to 2 weeks)

Goal: tagging `v0.1.0` on GitHub automatically builds, tests and publishes Mac and Windows installers.

### Step 6.1: Turn on CI

Pamplejuce's GitHub Actions workflow already builds on macOS and Windows, runs Catch2 tests and runs pluginval. Push to GitHub and make sure it goes green. Then add two things:

- **base.db in CI:** commit `data/build/base.db` to the repo (fine if it's under about 20 MB), so CI doesn't need the raw datasets. Rebuild it locally whenever the pipeline changes.
- **A Python job:** run `pytest pipeline/` so the chord parser and key detection can't silently break.

### Step 6.2: Check the data license before the first public build

Anything in a public repo or installer is redistribution. Until the Chordonomicon authors confirm the license in writing, build the shipped `base.db` from McGill Billboard, ChoCo (CC BY subsets) and your own seeds only. Add a `--ship` flag to `build_db.py` that excludes Chordonomicon. List every source and its license in `CREDITS.md`.

### Step 6.3: macOS signing and notarization

Without this, macOS blocks the plugin and users see a malware warning.

1. Join the Apple Developer Program (paid yearly membership).
2. Create a **Developer ID Application** certificate and a **Developer ID Installer** certificate.
3. Add the certificates and an app-specific password to the repo's GitHub Secrets. Pamplejuce's README lists the exact secret names its workflow expects.
4. The workflow signs the plugins, builds a `.pkg`, sends it to Apple for notarization and staples the ticket.

For a first beta to friends, you can skip this. Tell testers to run `xattr -dr com.apple.quarantine` on the plugin after installing.

### Step 6.4: Windows installer

Pamplejuce builds a Windows installer with Inno Setup, which puts the VST3 in `C:\Program Files\Common Files\VST3`. Unsigned installers trigger a SmartScreen warning ("More info → Run anyway"). That's acceptable for an open-source beta; buy a code-signing certificate later if it costs you users.

### Step 6.5: Pre-release checklist

Run through this on both operating systems before every release:

1. pluginval at `--strictness-level 10` passes.
2. `auval -v aumu Prlb Plab` passes (Mac).
3. **Fresh install:** delete the `ProgressionLab` app-data folder, install, open. Stock genres appear and `user.db` is created.
4. **Upgrade:** install the old version, keep a progression and create a genre, install the new version. Everything the user made is still there.
5. Drag-out works in FL Studio (Playlist and Piano roll) and Reaper. Test Ableton or Logic too if you can get a trial.
6. Two instances open in one project at the same time, both keeping progressions, without errors.
7. Save a project, close the DAW, reopen: same progression and settings. In FL Studio, also install the new build before reopening: the plugin must load and keep its progression.

### Step 6.6: Publish

Bump the version in `CMakeLists.txt`, then:

```bash
git tag v0.1.0 && git push origin v0.1.0
```

Pamplejuce's workflow attaches the installers to a GitHub Release. Write a README with a GIF of the drag-out, install steps for each OS, and a "Share your genre" section. Add a `genre-packs/` folder where people can submit `.plgenre` files by pull request.

**Done when:** someone who has never seen the project installs it from GitHub Releases and gets a progression into their DAW without asking you anything.

## Phase 7 (optional): A neural model

Only start this after v1 ships and only keep it if it wins a blind listening test against Markov. It's also a strong ML portfolio piece, which is a fair reason to do it.

1. **Train in Python with PyTorch.** A small decoder-only transformer: 4 layers, 256-wide, on the same tokens as the Markov model. Start every sequence with a genre token and a mode token, e.g. `<lofi> <minor> 0:min9 8:maj7 …`.
2. **Blending** becomes averaging the genre-token embeddings by weight, instead of mixing tables.
3. **Export** with `torch.onnx.export` to `model.onnx`. A model this size is a few MB.
4. **Run it in the plugin** with ONNX Runtime: link its prebuilt library in CMake, load the model once on a background thread, and sample token by token exactly like the Markov loop.
5. **Keep personalization in Markov.** You can't retrain a network on each user's machine. Multiply the network's next-token probabilities by the user's Markov probabilities for that genre (both weighted), so Keep still changes the output instantly.
6. **Blind test:** generate 30 from each engine per genre, shuffle, rate each 1 to 5 without knowing the source. Ship the network only if it scores clearly higher.

**Done when:** the blind test is decided, either way.

## Milestone checklist

Tick these off as you go. Week numbers assume about 10 to 15 hours a week.

**Weeks 1–2: Foundations (Phases 0–1)**

- [ ] Toolchain installed, Pamplejuce repo created, plugin codes set
- [ ] Empty plugin loads in FL Studio and Reaper; auval and pluginval pass; clangd and codelldb work in Neovim
- [ ] Hard-coded I–V–vi–IV drags into FL Studio (Playlist and Piano roll) and Reaper as a correct 4-bar clip
- [ ] Email sent to Chordonomicon authors about the license

**Weeks 2–4: Data (Phase 2)**

- [ ] Datasets downloaded; chord parser passes 30+ test symbols per source
- [ ] Key detection spot-checked on 50 songs
- [ ] Seed files written: 30+ each for lofi, trap, neo\_soul
- [ ] `base.db` builds from one command
- [ ] Python Markov prototype passes the blind genre test

**Weeks 5–6: Engine (Phase 3)**

- [ ] SQLiteCpp linked; `base.db` embedded; `user.db` created on first launch
- [ ] Markov model with backoff and temperature in C++, with Catch2 tests
- [ ] Generate button, key, mode, length and Weirdness work in the plugin
- [ ] Blending two genres works
- [ ] Project save and reload restores the progression

**Weeks 7–8: Learning (Phase 4)**

- [ ] Keep writes to `user.db` and changes output right away
- [ ] Origin weights and the 30% cap in place
- [ ] New Genre from typed chords and from library picks
- [ ] MIDI import with chord detection, editable before saving
- [ ] Strength badge, Reset to stock, Save blend as genre
- [ ] `.plgenre` export and import

**Weeks 9–10: Sound and UI (Phase 5)**

- [ ] Voicing engine with 4 styles and voice leading
- [ ] Preview synth synced to the DAW transport
- [ ] Reroll one chord, locks, inversion nudge
- [ ] Final layout, custom look, opens with a progression already generated
- [ ] A producer friend tests it cold

**Weeks 11–12: Ship (Phase 6)**

- [ ] CI green on Mac and Windows, including pytest
- [ ] Shipped `base.db` uses only confirmed-license data
- [ ] Pre-release checklist passes on both operating systems
- [ ] README, CREDITS.md, genre-packs folder
- [ ] `v0.1.0` tagged and released

## Gotchas to keep in mind

| Trap | What happens | Prevention |
| --- | --- | --- |
| DB or allocation on the audio thread | Clicks, dropouts, DAW crash | Audio thread only reads a fixed-size `Schedule` (Step 5.2) |
| Changing plugin or manufacturer codes | Saved projects lose the plugin | Set once in Phase 0, never change |
| Overwriting `user.db` on update | Users lose every genre they built | Only `base.db` is replaced; `user.db` gets migrations |
| Accepted progressions weighted like real data | Genres get repetitive over time | Origin weights + 30% cap (Step 4.2) |
| Silent chord-detection errors | Custom genres learn wrong chords | Always show detected chords for editing |
| Shipping unconfirmed-license data | Takedown or forced rewrite of the dataset | `--ship` build excludes Chordonomicon until confirmed |
| Reusing one temp `.mid` filename | DAW drops a stale clip | Timestamp every exported file |
| Two instances writing `user.db` at once | "database is locked" errors | WAL mode + `busy_timeout` (Step 3.2) |
| Testing in only one DAW | Drag-out broken for most users | Test FL Studio and Reaper every release; trial Ableton and Logic before big releases |
| Building the neural model first | Months spent, worse results with small data | Markov first; neural only if it wins a blind test |

## Reference links

- Pamplejuce template: `github.com/sudara/pamplejuce`
- JUCE docs and tutorials: `docs.juce.com`
- pluginval: `github.com/Tracktion/pluginval`
- SQLiteCpp: `github.com/SRombauts/SQLiteCpp`
- Chordonomicon: `huggingface.co/datasets/ailsntua/Chordonomicon`
- ChoCo: `github.com/smashub/choco`
- ONNX Runtime (Phase 7): `onnxruntime.ai`
