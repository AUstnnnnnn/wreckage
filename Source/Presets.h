#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Factory bank (embedded JSON, read-only) + user presets as plain .wreck JSON files in Documents/WRECKAGE/Presets.
// Ids: "factory:<index>" or "user:<file name>".
class Presets
{
public:
    explicit Presets (juce::AudioProcessorValueTreeState&);

    juce::var list() const;                 // [{id, name, author, tags, notes, factory, favorite, bank}]
    bool load (const juce::String& id);
    juce::String save (const juce::var& meta, bool overwriteCurrent); // returns new id, empty on failure
    bool remove (const juce::String& id);
    void setFavorite (const juce::String& id, bool);
    juce::String importFile (const juce::File&);
    bool exportTo (const juce::String& id, const juce::File&) const;
    void initPatch();
    juce::String step (int delta);          // prev/next through the visible (unlocked) list

    bool unlocked() const { return birthdayUnlocked; }
    void unlock();                          // the hidden OCT 29 bank

    juce::var current() const;              // {id, name, author, tags, notes, factory, dirty}
    void markDirty() { dirty = true; }

    juce::ValueTree toState() const;        // stored with the DAW project
    void fromState (const juce::ValueTree&);

    static juce::File userDir();
    static juce::var paramsToVar (const juce::AudioProcessorValueTreeState&);
    static void varToParams (const juce::var&, juce::AudioProcessorValueTreeState&);

    int numFactory() const { return factory.size(); }
    juce::var factoryAt (int i) const { return factory[i]; }

private:
    juce::var read (const juce::String& id) const;
    juce::File fileFor (const juce::String& id) const;
    void loadSettings();
    void saveSettings() const;

    juce::AudioProcessorValueTreeState& apvts;
    juce::Array<juce::var> factory;
    juce::StringArray favorites;
    bool birthdayUnlocked = false;

    juce::String curId, curName { "INIT" }, curAuthor, curNotes;
    juce::StringArray curTags;
    bool dirty = false;
};
