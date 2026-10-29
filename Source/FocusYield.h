#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Hand keyboard focus from the plugin's web view back to the host window, so the host's own
// typing keyboard sees key-ups (a focused web view swallows them and the host's notes hang).
void yieldKeyboardFocusToHost (juce::Component&);
