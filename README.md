# Atelier Debussy  (VST3 / AU / Standalone)

Impressionist MIDI generator. Pick a key, mode, style (Clair de Lune, Une barque sur l'ocean,
Jeux d'eau, La cathedrale engloutie), a part (Arp Up / Down / Wave, Main Chords, Motif, Bass,
Full Texture), a meter (4/4, 3/4, 6/8, 9/8, 12/8) and a rhythm grid (1/4, 1/8, 1/8 triplet,
1/16, 1/16 triplet). Press "Nouvelle esquisse" for a new idea.

Get MIDI into your DAW:
* Drag the "Drag MIDI to DAW" pad onto any MIDI/instrument track (most reliable), or "Export .mid".
* Or enable "Audition loop" / press play: the plugin also streams the clip as MIDI output
  (needs a DAW that routes VST3 MIDI out) and has a small built-in piano to hear it.

## Build without installing anything: GitHub Actions
1. Create a GitHub repo and push this folder.
2. Actions tab > "Build plugin" > download the artifact for Windows / macOS.
3. Copy the .vst3 to
   * Windows: C:\Program Files\Common Files\VST3
   * macOS:   ~/Library/Audio/Plug-Ins/VST3   (then: xattr -dr com.apple.quarantine "Atelier Debussy.vst3")

## Build locally
Needs CMake 3.22+ and a C++17 compiler (Visual Studio 2022 / Xcode / gcc). Internet needed once (fetches JUCE 7.0.12).

    cmake -B build -DAD_COPY=ON        # AD_COPY=ON copies the plugin to your system folder (may need admin on Windows)
    cmake --build build --config Release

Output: build/AtelierDebussy_artefacts/Release/{VST3,AU,Standalone}

JUCE is AGPL/commercial licensed: fine for personal use; check JUCE's terms before distributing.
