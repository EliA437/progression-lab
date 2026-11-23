#pragma once
#include <string>
#include <vector>

namespace pl
{

    struct Chord
    {
        int degree = 0; // semitones above the tonic (0-11): 0 = I, 7 = V, 9 = vi, 5 = IV
        std::string quality = "maj"; // "maj", "min", "dim", "aug", "7", "maj7", "min7", ...
        int bassDegree = -1;
        double beats = 4.0; // how long it lasts (4 beats = one bar in 4/4)
    };

    // A progression is just a vector of chords
    using Progression = std::vector<Chord>;

    // The key
    struct Key
    {
        int tonic = 0; // 0 = c, 1 = C#, etc...
        bool minor = false;
    };
}
