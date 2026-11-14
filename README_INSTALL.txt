ARDEN v0.2.1 — IMAGE LOAD FIX

IMPORTANT: Upload ALL these files to the repository ROOT, preserving the directories:

CMakeLists.txt
Source/PluginEditor.cpp
Source/PluginEditor.h
Source/PluginProcessor.cpp
Source/PluginProcessor.h
Assets/ArdenPanel.png

The Assets/ArdenPanel.png file is REQUIRED. If it is absent the build will fail.
The PNG is embedded into the VST3 at compile time, so no external file is needed after installation.

Do not remove the existing .github/workflows build file.
After uploading, wait for GitHub Actions, download the new VST3 and replace the old version.
Close and reopen REAPER to ensure the new VST3 binary is loaded.

This version adds explicit PNG decoding and a visible fallback panel if decoding fails.
