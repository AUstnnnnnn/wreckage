# Dev only: writes ui/dev/mock.js so the UI runs in a plain browser (no JUCE) for offscreen screenshots.
# URL flags: ?rust=0.7&level=0.6&crates=1&bday=1&tab=dirt&permit=1
import json, re
src = open("Source/Params.h").read()
lists = {m.group(1): re.findall(r'"([^"]+)"', m.group(2)) for m in re.finditer(r"StringArray (\w+)\s*\{([^}]*)\}", src)}
defs = []
for m in re.finditer(r'\{ "(\w+)",\s*"([^"]+)",\s*([-\d.f]+),\s*([-\d.f]+),\s*([-\d.f]+),\s*([-\d.f]+)(?:,\s*(&\w+|nullptr))?(?:,\s*(true))?\s*\}', src):
    i, name, mn, mx, df, sk, ch, tg = m.groups()
    f = lambda x: float(x.rstrip("f"))
    d = dict(id=i, name=name, min=f(mn), max=f(mx), defReal=f(df), centre=f(sk))
    if ch and ch != "nullptr": d["choices"] = lists[ch[1:]]
    d["toggle"] = bool(tg)
    isint = "choices" in d or d["toggle"] or i.endswith(("_oct", "_semi")) or i in ("uni_voices", "crush_bits", "crush_rate", "bend")
    d["steps"] = int(d["max"] - d["min"]) + 1 if isint else 0
    defs.append(d)
factory = json.load(open("presets/factory.json"))
js = """window.WRECKAGE_MOCK = (() => {
const DEFS = %s, FACTORY = %s;
const q = new URLSearchParams(location.search);
const listeners = {};
const norm = (d, r) => {
  if (d.steps) return (r - d.min) / (d.max - d.min);
  if (d.centre > 0) { const skew = Math.log(0.5) / Math.log((d.centre - d.min) / (d.max - d.min)); return Math.pow((r - d.min) / (d.max - d.min), skew); }
  return (r - d.min) / (d.max - d.min);
};
const vals = {}; DEFS.forEach(d => vals[d.id] = norm(d, d.defReal));
let unlocked = q.has('bday'), user = [], favs = new Set(['factory:0']);
let cur = { id: 'factory:0', name: FACTORY[0].name, tags: FACTORY[0].tags, author: 'WRECKAGE', notes: FACTORY[0].notes, factory: true, dirty: false };
const list = () => [...FACTORY.map((p, i) => ({ ...p, id: 'factory:' + i, factory: true })).filter(p => p.bank !== 'OCT 29' || unlocked), ...user]
  .map(p => ({ id: p.id, name: p.name, author: p.author, tags: p.tags, notes: p.notes, bank: p.bank || '', factory: !!p.factory, favorite: favs.has(p.id) }));
const payload = () => ({ current: { ...cur }, list: list(), unlocked });
const changed = {};
function load(id) {
  const p = id.startsWith('factory:') ? FACTORY[+id.split(':')[1]] : user.find(u => u.id === id);
  if (!p) return;
  DEFS.forEach(d => { vals[d.id] = norm(d, p.params[d.id] ?? d.defReal); changed[d.id] = [vals[d.id], '']; });
  cur = { id, name: p.name, tags: p.tags, author: p.author, notes: p.notes, factory: id.startsWith('factory'), dirty: false };
}
load('factory:0');
if (q.has('rust')) { vals.rust = +q.get('rust'); changed.rust = [vals.rust, '']; }
const fns = {
  init: () => ({ ...payload(), version: '1.0.0', params: DEFS.map(d => ({ id: d.id, name: d.name, min: d.min, max: d.max, centre: d.centre, def: norm(d, d.defReal), value: vals[d.id], steps: d.steps, choices: d.choices })) }),
  set: (id, v) => { vals[id] = v; cur.dirty = true; },
  gesture: () => {}, note: () => {},
  presetLoad: id => (load(id), payload()),
  presetStep: dlt => { const l = list(); const i = l.findIndex(p => p.id === cur.id); load(l[(i + dlt + l.length) %% l.length].id); return payload(); },
  presetInit: () => { DEFS.forEach(d => { vals[d.id] = norm(d, d.defReal); changed[d.id] = [vals[d.id], '']; }); cur = { id: '', name: 'INIT', tags: [], dirty: false }; return payload(); },
  presetSave: (meta, over) => { const id = over && cur.id.startsWith('user:') ? cur.id : 'user:' + meta.name + '.wreck';
    user = user.filter(u => u.id !== id).concat({ id, name: (meta.name || cur.name).toUpperCase(), tags: meta.tags || cur.tags, author: meta.author, notes: meta.notes, params: {} });
    cur = { ...cur, id, name: (meta.name || cur.name).toUpperCase(), tags: meta.tags || cur.tags, factory: false, dirty: false }; return { ...payload(), ok: true }; },
  presetDelete: id => { user = user.filter(u => u.id !== id); return payload(); },
  presetFavorite: (id, on) => { on ? favs.add(id) : favs.delete(id); return payload(); },
  presetUnlock: () => { unlocked = true; return payload(); },
  presetReveal: () => {}, presetImport: () => payload(), presetExport: () => true,
};
const call = (name, ...a) => Promise.resolve(fns[name] ? fns[name](...a) : null);
const listen = (ev, fn) => { (listeners[ev] = listeners[ev] || []).push(fn); };
let t = 0;
setInterval(() => {
  t += 1 / 30;
  const lvl = q.has('level') ? +q.get('level') : 0.35 + 0.3 * Math.abs(Math.sin(t * 1.3));
  const scope = Array.from({ length: 256 }, (_, i) => { const x = i / 256 * 4.3 + t; return Math.tanh(2.5 * (((x %% 1) * 2 - 1) * 0.5 + 0.35 * Math.sin(x * 9.4))) * 0.45 * (0.6 + lvl); });
  const s = { params: { ...changed }, peakL: lvl, peakR: lvl * 0.95, rust: vals.rust, scope };
  for (const k in changed) delete changed[k];
  (listeners.state || []).forEach(f => f(s));
}, 33);
setTimeout(() => {
  if (q.has('crates')) document.getElementById('cratesBtn').click();
  if (q.has('permit')) document.getElementById('wordmark').click();
  if (q.has('tab')) document.querySelector(`[data-tab="${q.get('tab')}"]`).click();
  if (q.has('lights')) document.getElementById('lightsOut').classList.remove('hidden');
}, 200);
return { call, listen };
})();
""" % (json.dumps(defs), json.dumps(factory))
import os
os.makedirs("ui/dev", exist_ok=True)
open("ui/dev/mock.js", "w").write(js)
print(len(defs), "params")
