#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

// The whole UI is ui/index.html in a WebView. This class is only the bridge:
// native functions the page calls, and a 30 Hz "state" event carrying param changes, meters and scope.
class WreckageEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit WreckageEditor (WreckageProcessor&);
    ~WreckageEditor() override;

    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colour (0xff141210)); }

    static constexpr int baseW = 1200, baseH = 750;

private:
    using Args = const juce::Array<juce::var>&;
    using Done = juce::WebBrowserComponent::NativeFunctionCompletion;

    void timerCallback() override;
    std::optional<juce::WebBrowserComponent::Resource> resource (const juce::String& url);
    juce::var initData();
    juce::var presetPayload();
    juce::RangedAudioParameter* param (const juce::var& id) { return proc.apvts.getParameter (id.toString()); }
    juce::WebBrowserComponent::Options options();

    WreckageProcessor& proc;
    std::vector<float> lastSent;
    juce::String lastPresetJson;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::WebBrowserComponent web;
};
