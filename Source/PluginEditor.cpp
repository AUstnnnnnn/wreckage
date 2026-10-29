#include "PluginEditor.h"
#include "FocusYield.h"
#include <BinaryData.h>

WreckageEditor::WreckageEditor (WreckageProcessor& p)
    : AudioProcessorEditor (p), proc (p), web (options())
{
    lastSent.assign (params::all().size(), -1.0f);
    addAndMakeVisible (web);
    web.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setResizable (true, true);
    setResizeLimits (baseW * 3 / 4, baseH * 3 / 4, baseW * 3 / 2, baseH * 3 / 2);
    getConstrainer()->setFixedAspectRatio ((double) baseW / baseH);
    setSize (baseW, baseH);
    startTimerHz (30);
}

WreckageEditor::~WreckageEditor() { stopTimer(); }

void WreckageEditor::resized() { web.setBounds (getLocalBounds()); }

juce::WebBrowserComponent::Options WreckageEditor::options()
{
    using O = juce::WebBrowserComponent::Options;
    auto userData = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("WRECKAGE-webview");

    return O {}
        .withBackend (O::Backend::webview2)
        .withWinWebView2Options (O::WinWebView2 {}.withUserDataFolder (userData).withBackgroundColour (juce::Colour (0xff141210)))
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([this] (const auto& url) { return resource (url); })

        .withNativeFunction ("init", [this] (Args, Done done) { done (initData()); })
        .withNativeFunction ("set", [this] (Args a, Done done)
        {
            if (auto* p = param (a[0])) { p->setValueNotifyingHost ((float) a[1]); proc.presets.markDirty(); }
            done ({});
        })
        .withNativeFunction ("gesture", [this] (Args a, Done done)
        {
            if (auto* p = param (a[0])) (bool) a[1] ? p->beginChangeGesture() : p->endChangeGesture();
            done ({});
        })
        .withNativeFunction ("yieldFocus", [this] (Args, Done done)
        {
            if (proc.wrapperType != juce::AudioProcessor::wrapperType_Standalone) yieldKeyboardFocusToHost (*this);
            done ({});
        })
        .withNativeFunction ("note", [this] (Args a, Done done)
        {
            const int n = (int) a[0];
            const float v = (float) a[1];
            auto m = v > 0 ? juce::MidiMessage::noteOn (1, n, v) : juce::MidiMessage::noteOff (1, n);
            m.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
            proc.uiMidi.addMessageToQueue (m);
            done ({});
        })

        // Presets. Every call answers with the fresh {current, list} so the page never has to guess.
        .withNativeFunction ("presetLoad", [this] (Args a, Done done) { proc.presets.load (a[0].toString()); done (presetPayload()); })
        .withNativeFunction ("presetStep", [this] (Args a, Done done) { proc.presets.step ((int) a[0]); done (presetPayload()); })
        .withNativeFunction ("presetInit", [this] (Args, Done done) { proc.presets.initPatch(); done (presetPayload()); })
        .withNativeFunction ("presetSave", [this] (Args a, Done done)
        {
            const auto id = proc.presets.save (a[0], (bool) a[1]);
            auto r = presetPayload();
            r.getDynamicObject()->setProperty ("ok", id.isNotEmpty());
            done (r);
        })
        .withNativeFunction ("presetDelete", [this] (Args a, Done done) { proc.presets.remove (a[0].toString()); done (presetPayload()); })
        .withNativeFunction ("presetFavorite", [this] (Args a, Done done) { proc.presets.setFavorite (a[0].toString(), (bool) a[1]); done (presetPayload()); })
        .withNativeFunction ("presetUnlock", [this] (Args, Done done) { proc.presets.unlock(); done (presetPayload()); })
        .withNativeFunction ("presetReveal", [] (Args, Done done) { Presets::userDir().revealToUser(); done ({}); })
        .withNativeFunction ("presetImport", [this] (Args, Done done)
        {
            chooser = std::make_unique<juce::FileChooser> ("Import a WRECKAGE preset", Presets::userDir(), "*.wreck;*.json");
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this, done] (const juce::FileChooser& fc)
                                  {
                                      if (fc.getResult().existsAsFile()) proc.presets.importFile (fc.getResult());
                                      done (presetPayload());
                                  });
        })
        .withNativeFunction ("presetExport", [this] (Args a, Done done)
        {
            const auto id = a[0].toString();
            auto name = juce::File::createLegalFileName (proc.presets.current()["name"].toString());
            chooser = std::make_unique<juce::FileChooser> ("Export preset",
                juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile (name + ".wreck"), "*.wreck");
            chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                                  [this, id, done] (const juce::FileChooser& fc)
                                  {
                                      const bool ok = fc.getResult() != juce::File() && proc.presets.exportTo (id, fc.getResult().withFileExtension ("wreck"));
                                      done (ok);
                                  });
        });
}

juce::var WreckageEditor::presetPayload()
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("current", proc.presets.current());
    o->setProperty ("list", proc.presets.list());
    o->setProperty ("unlocked", proc.presets.unlocked());
    return juce::var (o);
}

juce::var WreckageEditor::initData()
{
    juce::Array<juce::var> ps;
    for (auto& d : params::all())
    {
        auto* p = proc.apvts.getParameter (d.id);
        auto* o = new juce::DynamicObject();
        o->setProperty ("id", d.id);
        o->setProperty ("name", d.name);
        o->setProperty ("min", d.min);
        o->setProperty ("max", d.max);
        o->setProperty ("def", p->convertTo0to1 (d.def));
        o->setProperty ("centre", d.skewCentre);
        o->setProperty ("value", p->getValue());
        o->setProperty ("text", p->getCurrentValueAsText());
        o->setProperty ("steps", d.toggle ? 2 : (params::isInt (d) ? (int) (d.max - d.min) + 1 : 0));
        if (d.choices)
        {
            juce::Array<juce::var> c;
            for (auto& s : *d.choices) c.add (s);
            o->setProperty ("choices", c);
        }
        ps.add (juce::var (o));
    }
    auto r = presetPayload();
    r.getDynamicObject()->setProperty ("params", ps);
    r.getDynamicObject()->setProperty ("version", JucePlugin_VersionString);
    r.getDynamicObject()->setProperty ("standalone", proc.wrapperType == juce::AudioProcessor::wrapperType_Standalone);
    std::fill (lastSent.begin(), lastSent.end(), -1.0f);
    return r;
}

void WreckageEditor::timerCallback()
{
    auto* o = new juce::DynamicObject();

    // Param values that moved since the last tick (host automation, preset loads, our own sets echo back harmlessly).
    auto* changed = new juce::DynamicObject();
    const auto& defs = params::all();
    for (size_t i = 0; i < defs.size(); ++i)
    {
        auto* p = proc.apvts.getParameter (defs[i].id);
        const float v = p->getValue();
        if (std::abs (v - lastSent[i]) > 1.0e-5f)
        {
            lastSent[i] = v;
            changed->setProperty (defs[i].id, juce::Array<juce::var> { v, p->getCurrentValueAsText() });
        }
    }
    o->setProperty ("params", juce::var (changed));

    o->setProperty ("peakL", proc.peakL.load());
    o->setProperty ("peakR", proc.peakR.load());
    o->setProperty ("rust", proc.rustNow.load());

    // Scope: last 1024 samples, aligned to a rising zero crossing so the trace stands still, 256 points.
    {
        const int end = proc.scopePos.load();
        const int N = WreckageProcessor::scopeSize;
        auto at = [&] (int k) { return proc.scope[(size_t) ((end - N + k + 2 * N) % N)]; };
        int start = N / 2;
        for (int k = N / 2; k > 1; --k)
            if (at (k - 1) <= 0.0f && at (k) > 0.0f) { start = k; break; }
        start = juce::jmax (0, juce::jmin (start, N - 1024));
        juce::Array<juce::var> pts;
        for (int k = 0; k < 256; ++k)
            pts.add (std::round (at (start + k * 4) * 1000.0f) / 1000.0f);
        o->setProperty ("scope", pts);
    }

    const auto cur = juce::JSON::toString (proc.presets.current(), true);
    if (cur != lastPresetJson) { lastPresetJson = cur; o->setProperty ("preset", proc.presets.current()); }

    web.emitEventIfBrowserIsVisible ("state", juce::var (o));
}

std::optional<juce::WebBrowserComponent::Resource> WreckageEditor::resource (const juce::String& url)
{
    auto path = url.fromFirstOccurrenceOf ("/", false, false).upToFirstOccurrenceOf ("?", false, false);
    if (path.isEmpty()) path = "index.html";
    const auto file = path.fromLastOccurrenceOf ("/", false, false);

    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        if (file != BinaryData::getNamedResourceOriginalFilename (BinaryData::namedResourceList[i])) continue;
        int size = 0;
        auto* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size);
        const auto ext = file.fromLastOccurrenceOf (".", false, false).toLowerCase();
        const juce::String mime = ext == "html" ? "text/html" : ext == "css" ? "text/css" : ext == "js" ? "text/javascript"
                                : ext == "png" ? "image/png" : ext == "jpg" ? "image/jpeg" : ext == "webp" ? "image/webp"
                                : ext == "woff2" ? "font/woff2" : ext == "ttf" ? "font/ttf" : ext == "svg" ? "image/svg+xml"
                                : "application/octet-stream";
        std::vector<std::byte> bytes ((size_t) size);
        std::memcpy (bytes.data(), data, (size_t) size);
        return juce::WebBrowserComponent::Resource { std::move (bytes), mime };
    }
    return std::nullopt;
}
