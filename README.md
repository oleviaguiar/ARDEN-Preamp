# ARDEN Dual Circuit Preamplifier — prototype v0.1

Windows 11 x64 / JUCE 8 / C++17 / VST3. Source prototype, **not a compiled or hardware-validated plugin**.

## Build on Windows
1. Install Visual Studio 2022 with Desktop development with C++ and CMake, plus Git.
2. Open this folder in Visual Studio as a CMake project (internet required on first configuration to fetch JUCE 8.0.10).
3. Choose x64 Release and build target `Arden_VST3`.
4. Locate `ARDEN Dual Circuit Preamplifier.vst3` in the CMake build artifacts and copy the VST3 bundle to `C:\Program Files\Common Files\VST3`.
5. Rescan plugins in Reaper or Ableton Live.

## Current limitations / validation required
- This is a **first code prototype**, not yet compiled or tested on Windows. Compiler adjustments may be needed.
- Saturation models are custom approximations; they are **not** measured or circuit-accurate Neve/valve models.
- Drive compensation is an approximate fixed curve, not a calibrated loudness-matching system.
- EQ is a digital musical approximation; it uses the requested mid-frequency choices, but is not a 1073 circuit EQ model.
- GUI has vintage colors, gradient and glass meter, but realistic knob artwork, numbered scales, value popups and switch animation are pending.
- Stereo oversampling currently assumes a stereo bus; mono DAW support should be validated and adapted.
- Crossfade and parameter smoothing should be audited for real-time and click-free operation.
- The clip indicator lights at >= 0 dBFS sample peak; no limiter is included.
- CPU/latency benchmarking and listening tests are pending.

## Parameters
Circuit: 1073/Valve; Input -24..+24 dB; Drive 0..10; Low/Mid/High +/-6 dB; Mid Freq 360/700/1600/3200/4800/7200 Hz; Output -24..+12 dB; Power; EQ; meter IN/OUT.

## Engineering note
For a production release, avoid allocating temporary audio buffers in `processBlock`, preallocate dry storage in `prepareToPlay`, and move filter-coefficient updates out of the audio callback when unchanged. These are currently simple prototype implementations.
