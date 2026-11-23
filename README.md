# PolyRhythm Pro

**Compose a rhythm you can watch come back together.** Give a felt piano three pulses and a bass two in the same span of time. Press Play: their lights leave the same line, drift apart, and strike at different moments. Add a rolled chord and an airy echo. When every point finally lands together, the music resolves and the sky answers.

PolyRhythm Pro is a standalone generative instrument for **Windows x64**. Use it as a music-theory playground, a synth for building evolving ambient pieces, or a fullscreen stage for recording a performance. The notes are synthesized live; the visuals follow the notes that actually sound.

![Aurora Loom stage study: green, amber, violet and blue notes trace separate paths toward a common point beneath a curtain of light.](docs/aurora-loom.png)

**[Build and launch on Windows](#build-and-launch-on-windows)** · **[Play your first piece](#play-your-first-piece)** · **[Create a video performance](#create-a-video-performance)**

## Play your first piece

1. Open **Presets → Nocturne Atlas** and press **Play**. Turn off Chords, Atmosphere and Synth for a moment: the piano plays three pulses against two bass pulses. Turn the layers back on without losing their settings.
2. Choose **Aurora Loom** above the stage. Under **Nodes**, raise **Fan spread** to separate the paths. The five-lane note track shows what is about to sound; a layer name lights when its note lands.
3. Choose **Every point**, then **Next alignment**. Playback starts just *before* the meeting so you hear the approach. A root and fifth support the impact, a ninth settles to the octave, and a crest of light travels through the aurora.

**Want a different starting point?** Copper Canopy, Glass Tides and Polar Drift are complete arrangements with their own voices, effects, rhythm and scenery. **Start with a pattern** changes only the ratio. **Reset** rewinds the same piece rather than rolling a new one.

## Shape the sound

| Open this control | What you can do |
| --- | --- |
| **Pattern & tempo** | Pick from 24 ratios or set the melody and bass event counts yourself. Play at 1–180 BPM. Master pitch transposes all five layers together by up to an octave without changing their individual tuning. |
| **Music → Score** | Choose a layer and one of 14 synthesized instruments, from felt piano and hollow flute to Solar brass, Prism lead and Titan strings. The fifth **Synth** layer starts enabled in a fresh performance; switch it off in the mixer or save a different startup default. Set gain, pitch, octave, event multiplier, note chance and release. Note chance starts at 100%; written rests still count as rests. |
| **Music → Pitches** | Give each node a distinct note or let several share a pitch while keeping independent timing. Try ascending, descending or seeded order. On Chords, compare a triad, first inversion, suspended second, stacked fourths or seventh; hear it one tone at a time, as a block, or rolled upward. |
| **Music → EQ / Space / Acoustics** | Filter a layer, shape its low and high shelves, or send it into reverb, tempo-synced echo and chorus. Try **Velvet chamber** against **Glacial hall**, then adjust room size, damping, width and feedback. |
| **Music → Cloud** | Turn up Cloud mix to hear grains cut from the layer’s own recent sound. Change grain density, length and pitch range; add octave shimmer or reverse the grains. |

**Try it by ear:** solo Chords, choose 16 nodes, and set **Nodes per pitch** to 1, then 4. The second version shares four distinct pitches across the same 16 players. Raise the Chords multiplier to make the arpeggio faster without speeding up the whole piece.

## Direct the stage

- **Eclipse Garden** traces paths around an etched moon. A full reunion sends a solar flare over its rim. **Aurora Loom** weaves light beneath moving curtains. In **Lunar Tide**, moonlight brightens on the water and birds lift from the mountains.
- **Environment → Sky / Motion** changes the scenery, not the mix. Adjust up to 6,000 stars, twinkle, note sparks, hue cycling, parallax and scene-specific details such as curtain lift or moon path.
- **Nodes** gives each active layer 1–32 points (160 across five layers). Choose a convergence line or point; change fan spread, travel offset and the number of passes before reunion. Shared hits briefly draw glowing threads. **Layer attacks** finds a shared layer hit; **Every point** waits for every active node.
- **Nodes → Node motion** switches between the original eased paths and **Equal speed / variable distance**. A light with a longer wait takes a longer loop, so lights separate naturally but still reach the gate at their scheduled notes. Choose a mode in **Custom rhythm**, or flip it live from **View → Equal-speed nodes**. In **Nodes**, **Travel tempo** changes playback speed (the same BPM as the transport). In equal-speed mode, **Orbit distance** scales all loops from 25% to 800% without changing when notes land. Try 250% with a long reunion; large loops may extend beyond the stage. On a point target, **Point coverage** spreads the loops through a full circle at 100%. Travel offset applies only to the original mode.
- **Nodes → Auto-balance active paths** distributes active layers across the stage even if only Bass and Atmosphere are playing. Turn it off to keep fixed layer slots, or use **Place layer** directly below it to select a layer and change its **Line offset** or **Point rotation**. The View menu still offers the layout shortcuts.
- **View** controls fullscreen, the note track, renderer and stage FPS readout. Drag the dividers to resize the inspector and playing controls. Adjust track height in **Settings**. Keep **Cache scenery for smoother nodes** on to prioritize motion; turn it off for full-resolution scenery on each refresh. Playback follows display refresh, but actual FPS depends on the scene and hardware.

## Explore without losing a performance

| Action | What changes |
| --- | --- |
| **Random rhythm / Custom rhythm** | Roll a new ratio, groove and seed, or pick the event counts and groove yourself. Your instruments, effects and tempo stay put. |
| **Randomize all** | Explore a different arrangement and scene while preserving tempo, master level, layer switches and pitch/octave offsets. A dismissible warning gives you a chance to save first. |
| **Custom roll** | Select exactly which settings can change. Give numbers limits and tick allowed instruments or switch states. Unchecked settings stay untouched. |
| **File / Presets** | Save and load whole performances. Under **File → Manage saved presets**, rename, update or remove your own presets. |
| **Settings → Save startup defaults / Load defaults** | Remember only the controls you select. Saving does **not** alter the live piece. Load defaults to apply them now, or let them take effect next launch. **Use preset as startup defaults** does the same for a complete factory or saved performance. |

**Shortcuts:** F11 toggles fullscreen; −5 s / +5 s seek; Ctrl+Z / Ctrl+Y undo or redo performance edits. Scroll over a dial, slider or main selector to adjust it. Left/right arrows step the last clicked visible slider. User presets, sequences and defaults live under `%APPDATA%\PolyRhythm Pro`.

## Create a video performance

Press **F11**, then **Create video**. Choose a sequence and press **Preview** to hide the studio controls. The three factory scores build from two voices to five, adding up to 80 moving points along the way. Overlapping changes in tempo, pitch, effects and fan spread carry each arrangement into its finale. All three use 6,000 stars, full twinkle and drifting parallax. Lines of narration fade in at the top without a panel over the scenery.

- **Infinity bloom:** Follow a felt piano under the eclipse as glass and Solar brass rise out of the widening fan.
- **Silk in motion:** Hear seven against four beneath the aurora, with rolled chords and a swelling Titan strings layer.
- **Tidal echoes:** Listen for celesta echoes answered by flute and Prism lead under the moon.

Open **Edit timeline** to customize a factory sequence, or choose **New from current** to start from your own performance. Set **Timeline length** by typing a duration in seconds; the start and transition sliders scale to that span. Click a cue marker to edit it, or click empty space to place the next cue. Choose a category and setting, then set the target and optionally write a caption. Numeric settings ease over the transition; switches and instruments change on the cue. The editor stores frame-accurate times on a 30 fps grid, independent of playback refresh.

Use **Use current as opening** to replace the starting sound and scene without touching the cues. **Save as preset** keeps your edit without replacing a factory sequence. For saved sequences, use **Save** to update or rename, **Save copy** to branch, or **Remove saved…** to remove it.

**Space** pauses or resumes preview; **Esc** restores your prior instrument and playhead. **Finish at next full reunion** waits for the real musical event and its afterglow. Capture the stage with your screen recorder: this build does **not** encode video or export audio/MIDI. It is an instrument, not a DAW or plug-in host.

## If the app crashes

After restarting, choose **Settings → Open diagnostics folder**. Each launch writes its own local session log, including completed setting edits and environment switches. An unhandled Windows exception can also leave a matching `.dmp` file. Keep the pair from the affected session; nothing is uploaded automatically.

## Build and launch on Windows

These instructions are for the source tree containing the [launcher](Run-PolyRhythmPro.cmd) and [video editor](Source/VideoEditor.h). If your checkout only contains the initial JUCE scaffold, it is not this playable version.

1. Install **Visual Studio 2026** with **Desktop development with C++**, the Windows SDK and **C++ CMake tools for Windows**. Install Git. The supplied presets target the Visual Studio 18 2026 x64 generator.
2. Open the project folder in Visual Studio. From **Developer PowerShell**, at the folder containing `CMakePresets.json`, run:

   ```powershell
   cmake --preset windows-release
   cmake --build --preset windows-release --parallel 4
   .\Run-PolyRhythmPro.cmd
   ```

   The first configure downloads the pinned JUCE 9.0.6 source. If you already have JUCE locally, replace the first command with `cmake --preset windows-release "-DFETCHCONTENT_SOURCE_DIR_JUCE=C:\path\to\JUCE"`. Point it to the JUCE folder containing `modules` and `CMakeLists.txt`.

3. After building, double-click **Run-PolyRhythmPro.cmd**, or invoke it from any PowerShell directory with `& "C:\path\to\PolyRhythm-Pro\Run-PolyRhythmPro.cmd"`. It prefers Release for playback and falls back to Debug. To develop, build with the `windows-debug` preset and pass `--debug` to the launcher.

**If CMake is not on PATH:** in Visual Studio Community 2026, use `$cmake = Join-Path $env:ProgramFiles 'Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'` and replace `cmake` in the commands above with `& $cmake`. **If Play reports no output:** choose or reconnect a default audio output in Windows Sound settings, then restart the app.

## Behind the instrument

The project uses original procedural visuals and synthesized sounds. [Lucid Rhythms](https://www.youtube.com/@LucidRhythms/videos) inspired the idea of making musical phase visible; its software and recordings are not used here.

[Project license](LICENSE) · [JUCE dependency license](https://github.com/juce-framework/JUCE/blob/9.0.6/LICENSE.md)
