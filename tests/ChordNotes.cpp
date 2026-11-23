#include <catch2/catch_test_macros.hpp>
#include <engine/ChordNotes.h>
#include <vector>

using pl::Chord;
using pl::Key;
using Notes = std::vector<int>;

// chordToNotes() returns the bass note first, then the chord tones from the root up.

TEST_CASE ("I-V-vi-IV in C major", "[engine][chord]")
{
    const Key cMajor { 0, false };

    CHECK (pl::chordToNotes (Chord { 0, "maj" }, cMajor) == Notes { 48, 60, 64, 67 }); // C
    CHECK (pl::chordToNotes (Chord { 7, "maj" }, cMajor) == Notes { 55, 67, 71, 74 }); // G
    CHECK (pl::chordToNotes (Chord { 9, "min" }, cMajor) == Notes { 57, 69, 72, 76 }); // Am
    CHECK (pl::chordToNotes (Chord { 5, "maj" }, cMajor) == Notes { 53, 65, 69, 72 }); // F
}

TEST_CASE ("The key's tonic transposes the chord", "[engine][chord]")
{
    // I in D major is a D major chord: D F# A
    CHECK (pl::chordToNotes (Chord { 0, "maj" }, Key { 2, false }) == Notes { 50, 62, 66, 69 });
}

TEST_CASE ("Root pitch wraps around the octave", "[engine][chord]")
{
    // tonic A# (10) + degree 7 = 17, which wraps to 5 (F)
    CHECK (pl::chordToNotes (Chord { 7, "maj" }, Key { 10, false }) == Notes { 53, 65, 69, 72 });
}

TEST_CASE ("Slash chords use the bass degree for the bass note", "[engine][chord]")
{
    // C/E: C major chord with E in the bass
    Chord cOverE { 0, "maj" };
    cOverE.bassDegree = 4;

    CHECK (pl::chordToNotes (cOverE, Key { 0, false }) == Notes { 52, 60, 64, 67 });
}

TEST_CASE ("Seventh chords add a fourth chord tone", "[engine][chord]")
{
    // G7: G B D F
    CHECK (pl::chordToNotes (Chord { 7, "7" }, Key { 0, false }) == Notes { 55, 67, 71, 74, 77 });
}

TEST_CASE ("Chord quality interval recipes", "[engine][chord]")
{
    CHECK (pl::intervalsFor ("maj") == Notes { 0, 4, 7 });
    CHECK (pl::intervalsFor ("min") == Notes { 0, 3, 7 });
    CHECK (pl::intervalsFor ("dim") == Notes { 0, 3, 6 });
    CHECK (pl::intervalsFor ("maj7") == Notes { 0, 4, 7, 11 });
}

TEST_CASE ("Unknown quality falls back to major", "[engine][chord]")
{
    CHECK (pl::intervalsFor ("not-a-quality") == pl::intervalsFor ("maj"));
}
