# PolyRhythm Pro

PolyRhythm Pro is a free Windows desktop instrument for making, hearing, seeing, and learning polyrhythms. It should feel like a small musical instrument and visual playground: changing a rhythm changes both what the listener hears and what they see.

The app is for curious beginners, music producers, and people who already know music theory. Its interface should teach without getting in the way of experimentation.

## Product direction

Build a focused standalone instrument, not a DAW or plug-in host. Keep the rhythm visualization at the center, make the musical relationship easy to understand, and make every visible control work. Start with one polished scene and add capabilities only when they can use the same rhythm and transport model reliably.

The original product vision includes:

- Multiple rhythm voices, sound choices, and beginner-friendly presets.
- Editable polyrhythm ratios and tempo.
- Clear visual scenes such as orbiting beads, pendulums, or bouncing objects, with sound events tied to the same rhythm.
- Play/pause, reset, elapsed time, and simple project save/load.
- Later, richer sound customization, MIDI, and audio export.

The visual style is dark charcoal or midnight with restrained depth, clear typography, careful spacing, and distinct accents for the voices. Visuals should stay legible when resized and at high DPI. Use JUCE's normal 2D drawing APIs for the first scenes; do not add OpenGL as a setup requirement.

## First prototype

The first playable milestone is one shared-cycle orbit scene with:

- Voice A in cyan and Voice B in warm amber, each with its own synthesized sound.
- Play/pause, reset, cycle count, and elapsed time.
- Editable event counts from 2 through 9 and presets for 3:2, 4:3, and 5:4.
- A tempo control from 40 to 220 BPM. BPM is explicitly the tempo of Voice B.
- A master volume control and a visible message if audio output is unavailable.
- A timeline beneath the orbit tracks to show each voice's events and their shared boundary.
- A resizable dark interface with a 1180 × 680 minimum content size.

Voice A has `n` evenly spaced events and Voice B has `m` evenly spaced events during one repeating shared cycle. A displayed ratio `n:m` means “n hits in A during the time of m hits in B.” The cycle lasts `m × 60 / BPM` seconds. Both voices place their events evenly through that cycle. The first event is the shared boundary and is accented. The audio sample clock is the transport clock; the GUI timer only refreshes the display.

## Build on Windows

### Prerequisites

- Visual Studio 2022 (or another release supported by the selected JUCE version), with the **Desktop development with C++** workload. Include MSVC, a Windows SDK, and CMake tools for Windows.
- CMake 3.22 or newer. Visual Studio's CMake tools may provide it; otherwise install CMake from [cmake.org](https://cmake.org/download/).
- Git for Windows.
- VS Code's C/C++ and CMake Tools extensions are optional; the commands below work in PowerShell.

### Configure and build

Clone the project and open PowerShell in the repository folder. The first configure uses CMake FetchContent to download the pinned JUCE [9.0.6 release](https://github.com/juce-framework/JUCE/releases/tag/9.0.6), so it needs internet access. No JUCE checkout elsewhere on the machine is required.

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
```

The Debug executable is expected at:

```text
build\windows-debug\PolyRhythmPro_artefacts\Debug\PolyRhythm Pro.exe
```

Launch it from PowerShell:

```powershell
Start-Process ".\build\windows-debug\PolyRhythmPro_artefacts\Debug\PolyRhythm Pro.exe"
```

For an optimized Release build:

```powershell
cmake --preset windows-release
cmake --build --preset windows-release
Start-Process ".\build\windows-release\PolyRhythmPro_artefacts\Release\PolyRhythm Pro.exe"
```

The presets use the Visual Studio 2022 x64 generator. If using a different Visual Studio release, update the generator in `CMakePresets.json` to match the installed version.

The app uses the Windows default audio output. If no usable output is available, it should remain open and show an error message. Choose or reconnect an output in Windows Sound settings, then restart the app.

## Development handoff

The replacement project has not yet had a successful Windows configure, build, or launch. Treat that as the immediate next milestone:

1. Inspect `git status` and preserve local files. A local JUCE checkout may exist under the repository's `JUCE/` folder; it is ignored and is not needed by this FetchContent build.
2. Run the Debug configure and build commands above. Resolve real compiler, linker, or JUCE API errors before changing the product scope.
3. Launch the executable and inspect the actual window. Verify resizing, the stopped state, 3:2/4:3/5:4 presets, custom event counts, tempo changes, and Play/Pause/Reset.
4. Listen to both voices and verify that their attacks, timing, and visual beads stay aligned. Check the no-output-device path and show a real application screenshot after launch.
5. Only after the new project builds, retire the old Projucer scaffold in `JUCE-Assets/`.

Keep the rhythm model independent from drawing and audio. `RhythmModel` owns ratio semantics and cycle length; `TransportAudioEngine` schedules sound from sample time and publishes transport position; `RhythmScene` draws from that position; `MainComponent` coordinates the UI. Keep the audio callback free of UI work, avoid avoidable locks and allocations, and make UI/audio state exchange safe.

Future milestones, after validating the core instrument:

- Versioned JSON project save/load.
- Additional scenes such as pendulums and bouncing objects, driven by the same transport events.
- More sound choices and controls for pitch, attack, mix, and effects.
- MIDI and WAV/audio export.

Do not add controls for features that are not implemented. Update this README when the design or current feature set changes. Project instructions should live here and in the codebase, since a future Codex session in another IDE may not have this conversation's history.

## Project layout

- `CMakeLists.txt` and `CMakePresets.json`: pinned JUCE dependency and Windows Debug/Release builds.
- `Source/RhythmModel.*`: display-independent pattern and cycle semantics.
- `Source/TransportAudioEngine.*`: sample-clock transport and synthesized voices.
- `Source/RhythmScene.*`: 2D orbit tracks, beads, pulses, and event timeline.
- `Source/MainComponent.*`: controls and coordination.
- `JUCE-Assets/`: original Projucer scaffold. The root CMake project is active; the generated files are not used by it.
- `JUCE/`: optional local JUCE checkout. It is not part of the project build or repository contents.

## Licensing

The repository's existing `LICENSE` is GNU GPL version 3 and remains in place. JUCE 9 is dual-licensed under the JUCE licence and AGPLv3; the current build fetches JUCE 9.0.6. Before distributing binaries, provide the corresponding source and required notices, and review the current [JUCE 9 licence terms](https://juce.com/legal/juce-9-licence/) for exact obligations. This prototype includes no third-party sound samples, fonts, or image assets.

## Current limits

This is a focused instrument prototype, not a DAW or plug-in host. Save/load, alternate scenes, expanded synthesis/effects, MIDI, and audio export are not implemented yet.
