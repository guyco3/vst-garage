Do this bruh

```sh
(base) guycohen@Guys-MacBook-Air vst % xcodebuild -scheme "NewProject - Standalone Plugin"
(base) guycohen@Guys-MacBook-Air vst % open /Users/guycohen/dev/vst-garage/plugins/vst/NewProject/Builds/MacOSX/build/Debug/NewProject.app
(base) guycohen@Guys-MacBook-Air vst % 
```


to push release:

```sh
git tag v1.1.0 && git push origin v1.1.0
```

Plans for future plugins:

Proposed 10-VST Development Roadmap for VST Garage
Based on the exhaustive synthesis of user requests, technical gaps, and market trends, the following ten plugins are recommended for development. Each project is designed to address a specific, verified pain point in the modern production landscape.

1. The "Ghost Tape" Always-On Sampler

The Need: Producers frequently lose "happy accidents" because they were not actively recording. While some DAWs have retroactive MIDI record, few have reliable, lightweight retroactive audio capture that doesn't bloat the project folder.
Technical Implementation:   

Implement a high-efficiency circular buffer (32-bit float) that stores the last 20 minutes of audio.

The GUI should feature a waveform display that allows for instantaneous drag-and-drop of selected regions directly into the DAW timeline.

Include a "Ghost Tagging" system that uses transient detection to mark potential loops or hits automatically.   

2. The MSEG-FM "Soul" Synth

The Need: Users find modern FM synths too "utilitarian" or limited by ADSR envelopes. There is a desire for the "grit" and "bite" of legacy Yamaha hardware combined with modern modulation.
Technical Implementation:   

A 6-operator FM engine with switchable "Classic" and "Modern" DAC emulations.

Integrate a sophisticated MSEG for every operator, allowing for complex, multi-point modulation curves.

Include the "Yamaha Symphonic Chorus" as a built-in effect module, as this is a highly requested legacy feature.   

Full MPE support for per-note modulation of FM indices.

3. The "VST Rescan Guardian" Utility

The Need: DAW rescans are notorious for breaking project compatibility by deleting hidden or beta plugins and losing user-defined categories.
Technical Implementation:   

A standalone utility and VST bridge that mirrors the DAW’s plugin database.

Maintains a persistent "State Map" that caches plugin paths, versions, and OS compatibility data.

Provides an "at-a-glance" dashboard showing which plugins are officially supported for the current OS, solving the information gap identified on KVR.   

4. The "Dry AI" Bridge (De-Mixing & Stem Isolator)

The Need: Professional producers want to use sounds from generative AI (Suno/Udio) but find them too processed. They need to "dry out" the signal.
Technical Implementation:   

Use a neural network-based stem separator optimized for separating "Dry Vocal," "Dry Drums," and "Ambient Texture."

Include a "De-Reverb" module specifically tuned to the artifacts produced by generative AI models.

Allow for "Spectral Matching" where the user can feed a dry reference to the AI-generated stem to re-align the tonal balance.

5. The "Extended Woodwind" Physical Modeler

The Need: Composers lack high-quality, non-pitched woodwind effects like flutter-tongue and breath-noise.
Technical Implementation:   

Instead of samples, use physical modeling of the air column and reed dynamics.

Provide a "Tension" and "Pressure" control that can be modulated by a breath controller or expression pedal.

Focus on the "noise" components of the sound: the hiss of the air, the click of the keys, and the harmonic overtones of overblowing.

6. The "Logic-comp" Take Manager for FL Studio

The Need: FL Studio users struggle with cluttered playlists when recording multiple takes of audio or MIDI.
Technical Implementation:   

A VST plugin hosted on the mixer track that acts as the recording destination.

Internally manages a "Take Stack" similar to Logic Pro’s UI.

Allows the user to "swipe-comp" between takes within the plugin window and then "commit" the final comp to the FL Studio playlist as a single audio clip.

7. The "Boutique Pedal" Logic Sequencer

The Need: Users want the rhythmic complexity of pedals like the Chase Bliss Mood or Boss Slicer in the box.
Technical Implementation:   

A delay/reverb combo with a "Micro-Looping" engine.

Features a "Logic Sequencer" where the state of the delay (reverse, pitch-shift, stutter) is determined by a user-defined sequence that responds to the input transients.

Include a "Chaos" knob that introduces probability-based changes to the sequence, emulating the "happy accidents" of boutique hardware.

8. The "Phase-Lock" Drum De-Bleeder

The Need: Isolating drum bleed (e.g., snare in the kick mic) without introducing phase issues or unnatural gating artifacts.
Technical Implementation:   

A multiband expander driven by a sidechain from the "target" drum hit.

Uses a "Phase-Alignment" algorithm to ensure that the bleed is reduced without affecting the time-relationship between the various microphones in the kit.

Provides a "Visual Kit" map where users can point to where the bleed is coming from to refine the suppression algorithm.

9. The "Bitwig Utility" Pitch & Comp Suite

The Need: Bitwig is missing native, high-quality comping and pitch correction.
Technical Implementation:   

A focused utility plugin that provides a high-resolution "Note Editor" for manual pitch and time manipulation.

Integrates a "Crossfade Architect" that allows for more precise control over audio clip boundaries than Bitwig’s native inspector.

Designed to be "The Missing Inspector" for Bitwig users, focusing on the specific UI/UX gaps identified in the Sound on Sound review.   

10. The "Schrödinger" Texture Synth

The Need: There is a desire for non-traditional, "quantum" sound design tools like "Entanglement".
Technical Implementation:   

A wavetable synthesizer where the wavetable positions and morphing are determined by a real-time simulation of the Schrödinger equation.

Focus on "evolving textures" and "evolving pads" that don't sound like standard oscillators.

Include a "Probability Map" GUI where users can "paint" the likelihood of certain timbral shifts occurring over time.

