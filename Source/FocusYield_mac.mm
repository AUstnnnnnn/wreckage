#include "FocusYield.h"
#import <AppKit/AppKit.h>

void yieldKeyboardFocusToHost (juce::Component& c)
{
    auto* peer = c.getPeer();
    if (peer == nullptr) return;
    auto* view = (NSView*) peer->getNativeHandle();
    auto* window = [view window];
    if (window == nil) return;
    NSView* host = [view superview];
    [window makeFirstResponder: (host != nil && [host acceptsFirstResponder]) ? host : nil];
}
