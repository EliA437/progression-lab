/*
 What's going on: A Chord says "the V chord, major" but doesn't say which piano keys to press.
 This step converts it into MIDI note numbers, which is what goes into the MIDI file.

 MIDI note numbers: every piano key has a number. 
 60 is middle C, and each step up is one semitone (61 = C#, 62 = D, ... 72 = the next C).
 So a C major chord from middle C is 60, 64, 67.

 How the conversion works:

 Find the root note. Add the key's tonic and the chord's degree,
 then wrap into 0–11 with % 12. In C, the V chord is 0 + 7 = 7, which is G.
 Place it near middle C. 60 + 7 = 67, the G above middle C.
 Stack the chord's intervals on top.
 Each quality is a recipe of semitone gaps from the root:
 major = root, +4, +7 (root, major 3rd, 5th)
 minor = root, +3, +7 (the 3rd is one semitone lower, which is what makes it sound sad)
 Add a bass note an octave below the root, so it sounds full in a piano roll. For a slash chord,
 the bass is the bassDegree note instead.
*/

#pragma once
#include <map>
#include <string>
#include <vector>
#include "Chord.h"

namespace pl {

  // Semitone gaps from the root for each chord quality.
  inline const std::vector<int>& intervalsFor(const std::string& quality)
  {
      static const std::map<std::string, std::vector<int>> table = {
          { "maj",  { 0, 4, 7 } },
          { "min",  { 0, 3, 7 } },
          { "dim",  { 0, 3, 6 } },
          { "aug",  { 0, 4, 8 } },
          { "sus2", { 0, 2, 7 } },
          { "sus4", { 0, 5, 7 } },
          { "7",    { 0, 4, 7, 10 } },
          { "maj7", { 0, 4, 7, 11 } },
          { "min7", { 0, 3, 7, 10 } },
          { "m7b5", { 0, 3, 6, 10 } },
          { "dim7", { 0, 3, 6, 9 } },
          { "9",    { 0, 4, 7, 10, 14 } },
      };
      auto it = table.find(quality);
      return it != table.end() ? it->second : table.at("maj"); // unknown quality → major
  }

    // Turn one chord into midi note numbers (60 = middle c)
  inline std::vector<int> chordToNotes(const Chord& chord, const Key& key)
  {
      const int rootPitch = (key.tonic + chord.degree) % 12; // 0-11, e.g. 7 = G
      const int root = 60 + rootPitch;

      std::vector<int> notes;

      // Bass note an octave below: the root, or the slash-chord bass if one is set
      const int bassPitch = chord.bassDegree >= 0 ? (key.tonic + chord.bassDegree) % 12 : rootPitch;
      notes.push_back(48 + bassPitch);

      for (int gap : intervalsFor(chord.quality))
          notes.push_back(root + gap);

      return notes;
  }

}
