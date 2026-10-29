#include "Presets.h"
#include "Params.h"
#include <BinaryData.h>

static const juce::String birthdayBank = "OCT 29";

Presets::Presets (juce::AudioProcessorValueTreeState& s) : apvts (s)
{
    auto parsed = juce::JSON::parse (juce::String::createStringFromData (BinaryData::factory_json, BinaryData::factory_jsonSize));
    if (auto* arr = parsed.getArray())
        factory = *arr;
    loadSettings();
}

juce::File Presets::userDir()
{
    auto d = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("WRECKAGE").getChildFile ("Presets");
    d.createDirectory();
    return d;
}

static juce::File settingsFile() { return Presets::userDir().getParentDirectory().getChildFile ("settings.json"); }

void Presets::loadSettings()
{
    auto v = juce::JSON::parse (settingsFile());
    if (auto* fav = v["favorites"].getArray())
        for (auto& f : *fav) favorites.addIfNotAlreadyThere (f.toString());
    birthdayUnlocked = (bool) v["unlocked"];
}

void Presets::saveSettings() const
{
    auto* o = new juce::DynamicObject();
    juce::Array<juce::var> fav;
    for (auto& f : favorites) fav.add (f);
    o->setProperty ("favorites", fav);
    o->setProperty ("unlocked", birthdayUnlocked);
    settingsFile().replaceWithText (juce::JSON::toString (juce::var (o)));
}

void Presets::unlock()
{
    birthdayUnlocked = true;
    saveSettings();
}

juce::var Presets::paramsToVar (const juce::AudioProcessorValueTreeState& s)
{
    auto* o = new juce::DynamicObject();
    for (auto& d : params::all())
        if (auto* p = s.getParameter (d.id))
            o->setProperty (d.id, p->convertFrom0to1 (p->getValue()));
    return juce::var (o);
}

void Presets::varToParams (const juce::var& v, juce::AudioProcessorValueTreeState& s)
{
    // Missing keys fall back to the default so old or hand-written presets always load cleanly.
    for (auto& d : params::all())
        if (auto* p = s.getParameter (d.id))
        {
            const float real = v.hasProperty (d.id) ? (float) v[d.id] : d.def;
            p->setValueNotifyingHost (p->convertTo0to1 (real));
        }
}

static juce::Array<juce::var> toArray (const juce::StringArray& a)
{
    juce::Array<juce::var> r;
    for (auto& s : a) r.add (s);
    return r;
}

static juce::StringArray toStrings (const juce::var& v)
{
    juce::StringArray r;
    if (auto* a = v.getArray())
        for (auto& x : *a)
            if (auto t = x.toString().trim().toLowerCase(); t.isNotEmpty()) r.addIfNotAlreadyThere (t);
    return r;
}

static juce::String safeName (juce::String n)
{
    n = juce::File::createLegalFileName (n.trim());
    return n.isEmpty() ? "UNTITLED" : n;
}

juce::File Presets::fileFor (const juce::String& id) const
{
    return id.startsWith ("user:") ? userDir().getChildFile (id.fromFirstOccurrenceOf ("user:", false, false)) : juce::File();
}

juce::var Presets::read (const juce::String& id) const
{
    if (id.startsWith ("factory:"))
    {
        const int i = id.fromFirstOccurrenceOf (":", false, false).getIntValue();
        return juce::isPositiveAndBelow (i, factory.size()) ? factory[i] : juce::var();
    }
    auto f = fileFor (id);
    return f.existsAsFile() ? juce::JSON::parse (f) : juce::var();
}

juce::var Presets::list() const
{
    juce::Array<juce::var> out;
    auto add = [&] (const juce::String& id, const juce::var& p, bool isFactory)
    {
        const auto bank = p["bank"].toString();
        if (bank == birthdayBank && ! birthdayUnlocked) return;
        auto* o = new juce::DynamicObject();
        o->setProperty ("id", id);
        o->setProperty ("name", p["name"].toString().isEmpty() ? juce::String ("UNTITLED") : p["name"].toString());
        o->setProperty ("author", p["author"]);
        o->setProperty ("tags", toArray (toStrings (p["tags"])));
        o->setProperty ("notes", p["notes"]);
        o->setProperty ("bank", bank);
        o->setProperty ("factory", isFactory);
        o->setProperty ("favorite", favorites.contains (id));
        out.add (juce::var (o));
    };
    for (int i = 0; i < factory.size(); ++i)
        add ("factory:" + juce::String (i), factory[i], true);
    for (auto& f : userDir().findChildFiles (juce::File::findFiles, false, "*.wreck"))
        add ("user:" + f.getFileName(), juce::JSON::parse (f), false);
    return out;
}

bool Presets::load (const juce::String& id)
{
    auto p = read (id);
    if (! p.isObject()) return false;
    varToParams (p["params"], apvts);
    curId = id;
    curName = p["name"].toString();
    curAuthor = p["author"].toString();
    curNotes = p["notes"].toString();
    curTags = toStrings (p["tags"]);
    dirty = false;
    return true;
}

juce::String Presets::save (const juce::var& meta, bool overwriteCurrent)
{
    const auto name = meta["name"].toString().trim().isEmpty() ? curName : meta["name"].toString().trim().toUpperCase();
    auto* o = new juce::DynamicObject();
    o->setProperty ("name", name);
    o->setProperty ("author", meta.hasProperty ("author") ? meta["author"].toString() : curAuthor);
    o->setProperty ("tags", toArray (meta.hasProperty ("tags") ? toStrings (meta["tags"]) : curTags));
    o->setProperty ("notes", meta.hasProperty ("notes") ? meta["notes"].toString() : curNotes);
    o->setProperty ("version", 1);
    o->setProperty ("params", paramsToVar (apvts));

    juce::File f;
    if (overwriteCurrent && curId.startsWith ("user:"))
    {
        auto old = fileFor (curId);
        f = userDir().getChildFile (safeName (name) + ".wreck");
        if (f != old && ! f.exists()) old.moveFileTo (f);
        else if (f != old) f = old; // keep the old file name rather than clobber a different preset
    }
    else
        f = userDir().getNonexistentChildFile (safeName (name), ".wreck", false);

    if (! f.replaceWithText (juce::JSON::toString (juce::var (o))))
        return {};
    load ("user:" + f.getFileName());
    return curId;
}

bool Presets::remove (const juce::String& id)
{
    auto f = fileFor (id);
    if (! f.existsAsFile() || ! f.moveToTrash()) return false;
    favorites.removeString (id);
    saveSettings();
    if (curId == id) { curId = {}; dirty = true; }
    return true;
}

void Presets::setFavorite (const juce::String& id, bool on)
{
    if (on) favorites.addIfNotAlreadyThere (id);
    else favorites.removeString (id);
    saveSettings();
}

juce::String Presets::importFile (const juce::File& src)
{
    auto p = juce::JSON::parse (src);
    if (! p["params"].isObject()) return {};
    auto f = userDir().getNonexistentChildFile (safeName (p["name"].toString().isEmpty() ? src.getFileNameWithoutExtension() : p["name"].toString()), ".wreck", false);
    if (! src.copyFileTo (f)) return {};
    const auto id = "user:" + f.getFileName();
    load (id);
    return id;
}

bool Presets::exportTo (const juce::String& id, const juce::File& dest) const
{
    auto p = read (id);
    return p.isObject() && dest.replaceWithText (juce::JSON::toString (p));
}

void Presets::initPatch()
{
    varToParams (juce::var (new juce::DynamicObject()), apvts);
    curId = {}; curName = "INIT"; curAuthor = {}; curNotes = {}; curTags.clear();
    dirty = false;
}

juce::String Presets::step (int delta)
{
    auto all = list();
    const int n = all.size();
    if (n == 0) return {};
    int idx = -1;
    for (int i = 0; i < n; ++i)
        if (all[i]["id"].toString() == curId) idx = i;
    idx = idx < 0 ? (delta > 0 ? 0 : n - 1) : (idx + delta + n) % n;
    load (all[idx]["id"].toString());
    return curId;
}

juce::var Presets::current() const
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("id", curId);
    o->setProperty ("name", curName);
    o->setProperty ("author", curAuthor);
    o->setProperty ("notes", curNotes);
    o->setProperty ("tags", toArray (curTags));
    o->setProperty ("factory", curId.startsWith ("factory:"));
    o->setProperty ("dirty", dirty);
    return juce::var (o);
}

juce::ValueTree Presets::toState() const
{
    juce::ValueTree t ("PRESET");
    t.setProperty ("id", curId, nullptr);
    t.setProperty ("name", curName, nullptr);
    t.setProperty ("author", curAuthor, nullptr);
    t.setProperty ("notes", curNotes, nullptr);
    t.setProperty ("tags", curTags.joinIntoString (","), nullptr);
    t.setProperty ("dirty", dirty, nullptr);
    return t;
}

void Presets::fromState (const juce::ValueTree& t)
{
    if (! t.isValid()) return;
    curId = t["id"].toString();
    curName = t["name"].toString();
    curAuthor = t["author"].toString();
    curNotes = t["notes"].toString();
    curTags = juce::StringArray::fromTokens (t["tags"].toString(), ",", "");
    curTags.removeEmptyStrings();
    dirty = (bool) t["dirty"];
}
