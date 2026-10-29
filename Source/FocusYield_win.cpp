#include "FocusYield.h"
#define NOMINMAX
#include <windows.h>

void yieldKeyboardFocusToHost (juce::Component& c)
{
    auto* peer = c.getPeer();
    if (peer == nullptr) return;
    auto hwnd = (HWND) peer->getNativeHandle();
    if (auto parent = GetParent (hwnd)) SetFocus (parent);
}
