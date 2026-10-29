'use strict';

const BASE_W = 1200, BASE_H = 750;
const $ = (s, el = document) => el.querySelector(s);
const $$ = (s, el = document) => [...el.querySelectorAll(s)];
const stage = $('#stage');
const clamp = (x, a = 0, b = 1) => Math.min(b, Math.max(a, x));

function fit() {
  const s = Math.min(innerWidth / BASE_W, innerHeight / BASE_H);
  stage.style.transform = `scale(${s})`;
  stage.style.left = `${(innerWidth - BASE_W * s) / 2}px`;
  stage.style.top = `${(innerHeight - BASE_H * s) / 2}px`;
}
addEventListener('resize', fit);
fit();

/* ------------------------------------------------------------------ bridge */
// JUCE's native-function protocol, without pulling in its module file.
let call, listen;
function juceBridge(J) {
  let next = 0;
  const pending = new Map();
  J.backend.addEventListener('__juce__complete', ({ promiseId, result }) => {
    pending.get(promiseId)?.(result);
    pending.delete(promiseId);
  });
  call = (name, ...params) => new Promise(res => {
    const id = next++;
    pending.set(id, res);
    J.backend.emitEvent('__juce__invoke', { name, params, resultId: id });
  });
  listen = (ev, fn) => J.backend.addEventListener(ev, fn);
}

/* ------------------------------------------------------------------ params */
const P = {};          // id -> {def, v (0..1), text, el: [render fns]}
const dragging = new Set();

function toReal(p, v) {
  if (p.steps) return p.min + Math.round(v * (p.max - p.min));
  if (p.centre > 0) {
    const skew = Math.log(0.5) / Math.log((p.centre - p.min) / (p.max - p.min));
    return p.min + (p.max - p.min) * Math.exp(Math.log(Math.max(v, 1e-9)) / skew);
  }
  return p.min + v * (p.max - p.min);
}

const pct = r => `${Math.round(r * 100)}%`;
const secs = r => (r < 1 ? `${Math.round(r * 1000)}MS` : `${r.toFixed(2)}S`);
const signed = (r, u) => `${r > 0 ? '+' : ''}${Math.round(r)}${u}`;
function fmt(id, v) {
  const p = P[id]; if (!p) return '';
  const r = toReal(p, v);
  if (p.choices) return p.choices[Math.round(r)] ?? '';
  if (p.steps === 2 && p.min === 0 && p.max === 1) return r ? 'ON' : 'OFF';
  if (id === 'flt_cutoff') return r >= 1000 ? `${(r / 1000).toFixed(r >= 10000 ? 1 : 2)}K` : `${Math.round(r)}HZ`;
  if (/_(a|d|r)$/.test(id)) return secs(r);
  if (id.endsWith('_semi')) return signed(r, 'ST');
  if (id.endsWith('_fine')) return signed(r, 'CT');
  if (id.endsWith('_oct')) return signed(r, '');
  if (id === 'flt_env') return signed(r * 100, '%');
  if (id === 'out_gain') return `${r > 0 ? '+' : ''}${r.toFixed(1)}DB`;
  if (id === 'glide') return r < 0.001 ? 'OFF' : secs(r);
  if (id === 'lfo_rate') return `${r < 10 ? r.toFixed(2) : r.toFixed(1)}HZ`;
  if (id === 'crush_bits') return r >= 16 ? 'CLEAN' : `${r} BIT`;
  if (id === 'crush_rate') return r <= 1 ? 'CLEAN' : `÷${r}`;
  if (id === 'uni_voices') return r === 1 ? 'OFF' : `${r} VOX`;
  if (id === 'bend') return `±${r}ST`;
  if (id === 'oscA_pw' || id === 'oscB_pw') return pct(r);
  return pct((r - p.min) / (p.max - p.min));
}

function setParam(id, v, fromUser = true) {
  const p = P[id]; if (!p) return;
  if (p.steps) v = Math.round(v * (p.steps - 1)) / (p.steps - 1);
  v = clamp(v);
  if (Math.abs(v - p.v) < 1e-6) return;
  p.v = v;
  p.views.forEach(f => f());
  if (fromUser) { call('set', id, v); markDirty(); }
}
const gesture = (id, on) => call('gesture', id, on);
const bind = (id, fn) => P[id] && P[id].views.push(fn);

/* ------------------------------------------------------------------ knobs */
const SVGNS = 'http://www.w3.org/2000/svg';
// Controls ease toward their value instead of snapping (preset loads sweep, stepped knobs click over).
const reduceMotion = matchMedia('(prefers-reduced-motion: reduce)').matches;
const tweens = new Set();
function tweenTo(t, target, snap) {
  t.target = target;
  if (snap || reduceMotion) { t.cur = target; tweens.delete(t); t.draw(target); return; }
  tweens.add(t);
}
(function tick() {
  for (const t of tweens) {
    t.cur += (t.target - t.cur) * 0.3;
    if (Math.abs(t.target - t.cur) < 0.001) { t.cur = t.target; tweens.delete(t); }
    t.draw(t.cur);
  }
  requestAnimationFrame(tick);
})();
function arcPath(a0, a1, r = 40) {
  const p = a => [Math.sin(a * Math.PI / 180) * r, -Math.cos(a * Math.PI / 180) * r];
  const [x0, y0] = p(a0), [x1, y1] = p(a1);
  const large = Math.abs(a1 - a0) > 180 ? 1 : 0;
  return `M${x0} ${y0}A${r} ${r} 0 ${large} ${a1 > a0 ? 1 : 0} ${x1} ${y1}`;
}

function dragValue(el, id, { sensitivity = 220, onTap } = {}) {
  let startY = 0, startV = 0, moved = false;
  el.addEventListener('pointerdown', e => {
    if (e.button !== 0) return;
    e.preventDefault();
    el.setPointerCapture(e.pointerId);
    el.focus({ preventScroll: true });
    startY = e.clientY; startV = P[id].v; moved = false;
    dragging.add(id); gesture(id, true);
    el.classList.add('active');
  });
  el.addEventListener('pointermove', e => {
    if (!dragging.has(id) || !el.hasPointerCapture(e.pointerId)) return;
    const scale = stage.getBoundingClientRect().width / BASE_W;
    const dy = (startY - e.clientY) / scale;
    if (Math.abs(dy) > 2) moved = true;
    const steps = P[id].steps;
    const range = steps ? Math.max(60, steps * 14) : (e.shiftKey ? sensitivity * 5 : sensitivity);
    setParam(id, startV + dy / range);
  });
  const end = e => {
    if (!dragging.has(id)) return;
    dragging.delete(id); gesture(id, false);
    el.classList.remove('active');
    if (!moved && onTap) onTap(e);
  };
  el.addEventListener('pointerup', end);
  el.addEventListener('pointercancel', end);
  el.addEventListener('dblclick', () => { gesture(id, true); setParam(id, P[id].def); gesture(id, false); });
  el.addEventListener('wheel', e => {
    e.preventDefault();
    const steps = P[id].steps;
    const d = Math.sign(-e.deltaY) * (steps ? 1 / (steps - 1) : (e.shiftKey ? 0.002 : 0.01));
    setParam(id, P[id].v + d);
  }, { passive: false });
  el.addEventListener('keydown', e => {
    const steps = P[id].steps;
    const d = steps ? 1 / (steps - 1) : (e.shiftKey ? 0.002 : 0.02);
    if (e.key === 'ArrowUp' || e.key === 'ArrowRight') setParam(id, P[id].v + d);
    else if (e.key === 'ArrowDown' || e.key === 'ArrowLeft') setParam(id, P[id].v - d);
    else if (e.key === 'Home') setParam(id, 0);
    else if (e.key === 'End') setParam(id, 1);
    else if (e.key === 'Delete' || e.key === 'Backspace') setParam(id, P[id].def);
    else return;
    e.preventDefault(); e.stopPropagation();
  });
}

function engraveScale(svg) {
  // 0..10 cut into the concrete around RUST: a dark groove with a lit lower lip.
  let g = '';
  for (let i = 0; i <= 20; i++) {
    const a = (-135 + i * 13.5) * Math.PI / 180, major = i % 2 === 0;
    const r0 = 43.5, r1 = major ? 47.5 : 45.5;
    const pt = r => [Math.sin(a) * r, -Math.cos(a) * r];
    const [x0, y0] = pt(r0), [x1, y1] = pt(r1);
    g += `<line class="cut-lip" x1="${x0}" y1="${y0 + 0.35}" x2="${x1}" y2="${y1 + 0.35}"/><line class="cut" x1="${x0}" y1="${y0}" x2="${x1}" y2="${y1}"/>`;
    if (major && i % 4 === 0) {
      const [tx, ty] = pt(51.5);
      g += `<text class="cut-num" x="${tx}" y="${ty}">${i / 2}</text>`;
    }
  }
  svg.insertAdjacentHTML('afterbegin', `<g class="scale">${g}</g>`);
}

function buildKnob(el) {
  const id = el.dataset.p, p = P[id]; if (!p) return;
  const label = el.dataset.label || p.name;
  const bipolar = el.hasAttribute('data-bipolar') || (p.min < 0 && p.max > 0 && !p.steps);
  el.tabIndex = 0;
  el.setAttribute('role', 'slider');
  el.setAttribute('aria-label', p.name);
  el.innerHTML = `<div class="dial"><img src="assets/plates/rust-knob.png" alt="" draggable="false">
    <svg viewBox="-50 -50 100 100"><path class="arc" stroke-width="3.6"/><line class="needle" x1="0" y1="-11" x2="0" y2="-34" stroke-width="2.6"/></svg></div>
    <div class="label"></div>`;
  const arc = $('.arc', el), needle = $('.needle', el), lab = $('.label', el);
  if (el.classList.contains('rust')) engraveScale($('svg', el));
  let hover = false;
  const t = { cur: p.v, draw: v => {
    const a = -135 + v * 270;
    const a0 = bipolar ? 0 : -135;
    arc.setAttribute('d', Math.abs(a - a0) < 0.5 ? '' : arcPath(Math.min(a0, a), Math.max(a0, a)));
    needle.setAttribute('transform', `rotate(${a})`);
  } };
  t.draw(p.v);
  const render = () => {
    tweenTo(t, p.v, dragging.has(id) && !p.steps);
    const text = fmt(id, p.v);
    el.setAttribute('aria-valuetext', text);
    if (p.choices) lab.innerHTML = `${label} <span class="val">${text}</span>`;
    else lab.innerHTML = hover || dragging.has(id) ? `<span class="val">${text}</span>` : label;
  };
  el.addEventListener('pointerenter', () => { hover = true; render(); });
  el.addEventListener('pointerleave', () => { hover = false; render(); });
  el.addEventListener('focus', () => { hover = true; render(); });
  el.addEventListener('blur', () => { hover = false; render(); });
  dragValue(el, id, { sensitivity: el.classList.contains('rust') ? 320 : 220 });
  bind(id, render); render();
}

function buildFader(el) {
  const id = el.dataset.p, p = P[id]; if (!p) return;
  el.tabIndex = 0; el.setAttribute('role', 'slider'); el.setAttribute('aria-label', p.name);
  el.innerHTML = `<div class="slot"><div class="fill"></div><div class="cap"></div></div><div class="label"></div>`;
  const fill = $('.fill', el), cap = $('.cap', el), lab = $('.label', el);
  const H = 214;
  let hover = false;
  const t = { cur: p.v, draw: v => { fill.style.height = `${v * (H - 4)}px`; cap.style.bottom = `${v * H}px`; } };
  t.draw(p.v);
  const render = () => {
    tweenTo(t, p.v, dragging.has(id));
    const text = fmt(id, p.v);
    el.setAttribute('aria-valuetext', text);
    lab.innerHTML = hover || dragging.has(id) ? `<span class="val">${text}</span>` : el.dataset.label;
  };
  el.addEventListener('pointerenter', () => { hover = true; render(); });
  el.addEventListener('pointerleave', () => { hover = false; render(); });
  dragValue(el, id, { sensitivity: H });
  bind(id, render); render();
}

function buildSeg(el) {
  const id = el.dataset.p, p = P[id]; if (!p) return;
  const labels = el.dataset.labels ? el.dataset.labels.split(',') : p.choices;
  el.setAttribute('role', 'group'); el.setAttribute('aria-label', p.name);
  el.innerHTML = labels.map((l, i) => `<button data-i="${i}">${l}</button>`).join('');
  const n = labels.length;
  $$('button', el).forEach(b => b.addEventListener('click', () => {
    gesture(id, true); setParam(id, +b.dataset.i / (n - 1)); gesture(id, false);
  }));
  const render = () => $$('button', el).forEach(b =>
    b.setAttribute('aria-pressed', String(Math.round(p.v * (n - 1)) === +b.dataset.i)));
  bind(id, render); render();
}

function buildStepper(el) {
  const id = el.dataset.p, p = P[id]; if (!p) return;
  el.innerHTML = `<span>${el.dataset.label}</span><button aria-label="${p.name} down">−</button><span class="v"></span><button aria-label="${p.name} up">+</button>`;
  const [down, up] = $$('button', el), v = $('.v', el);
  const step = 1 / (p.steps - 1);
  down.onclick = () => { gesture(id, true); setParam(id, p.v - step); gesture(id, false); };
  up.onclick = () => { gesture(id, true); setParam(id, p.v + step); gesture(id, false); };
  const render = () => { v.textContent = fmt(id, p.v); };
  bind(id, render); render();
}

function buildToggle(el) {
  const id = el.dataset.p, p = P[id]; if (!p) return;
  el.onclick = () => { gesture(id, true); setParam(id, p.v > 0.5 ? 0 : 1); gesture(id, false); };
  const render = () => el.setAttribute('aria-pressed', String(p.v > 0.5));
  bind(id, render); render();
}

function buildCycle(el) {
  const id = el.dataset.p, p = P[id]; if (!p) return;
  el.title = p.name;
  el.onclick = () => {
    const n = p.steps, i = (Math.round(p.v * (n - 1)) + 1) % n;
    gesture(id, true); setParam(id, i / (n - 1)); gesture(id, false);
  };
  const render = () => { el.textContent = fmt(id, p.v); };
  bind(id, render); render();
}

function buildSelector(el) {
  const id = el.dataset.p, p = P[id]; if (!p) return;
  const n = p.choices.length, angles = [-90, 0, 90, 180];
  el.innerHTML = p.choices.map((c, i) => `<button class="pos p${i}" data-i="${i}">${c}</button>`).join('')
    + `<div class="knob" tabindex="0" role="slider" aria-label="${p.name}"><div class="dial"><img src="assets/plates/rust-knob.png" alt="" draggable="false"></div></div><div class="grip"></div>`;
  const pick = i => { gesture(id, true); setParam(id, i / (n - 1)); gesture(id, false); };
  $$('.pos', el).forEach(b => b.onclick = () => pick(+b.dataset.i));
  const knob = $('.knob', el);
  dragValue(knob, id, { onTap: () => pick((Math.round(p.v * (n - 1)) + 1) % n) });
  const grip = $('.grip', el);
  let shown = angles[Math.round(p.v * (n - 1))];
  const render = () => {
    const i = Math.round(p.v * (n - 1));
    let target = angles[i];
    while (target - shown > 180) target -= 360;
    while (shown - target > 180) target += 360;
    shown = target;
    grip.style.setProperty('--a', `${shown}deg`);
    $$('.pos', el).forEach(b => b.setAttribute('aria-pressed', String(+b.dataset.i === i)));
    knob.setAttribute('aria-valuetext', p.choices[i]);
  };
  bind(id, render); render();
}

/* ------------------------------------------------------------------ tabs */
$$('.tabs button').forEach(b => b.addEventListener('click', () => {
  $$('.tabs button').forEach(x => x.setAttribute('aria-selected', String(x === b)));
  $$('.screen .pane').forEach(x => x.classList.toggle('hidden', x.dataset.pane !== b.dataset.tab));
}));

/* ------------------------------------------------------------------ meters, tube, scope */
const scope = $('#scope');
const sctx = scope.getContext('2d');
scope.width = 916; scope.height = 426;
let lastScope = null;
const meter = { shown: 0, slow: 0, flash: 0, flicker: 0 };
const tubeEl = $('#tube'), spillEl = $('#spill');

function drawScope(pts) {
  const w = scope.width, h = scope.height;
  sctx.globalCompositeOperation = 'source-over';
  sctx.fillStyle = 'rgba(22, 16, 13, 0.78)'; // short phosphor persistence
  sctx.fillRect(0, 0, w, h);
  // graticule, 10 x 6, redrawn under the trace every frame
  sctx.strokeStyle = 'rgba(224, 116, 90, 0.14)';
  sctx.lineWidth = 1.5;
  sctx.beginPath();
  for (let i = 1; i < 10; i++) { const x = Math.round(i * w / 10) + 0.5; sctx.moveTo(x, 0); sctx.lineTo(x, h); }
  for (let j = 1; j < 6; j++) { const y = Math.round(j * h / 6) + 0.5; sctx.moveTo(0, y); sctx.lineTo(w, y); }
  sctx.stroke();
  if (!pts || !pts.length) return;
  sctx.globalCompositeOperation = 'source-over';
  const pass = (width, alpha) => {
    sctx.beginPath();
    for (let i = 0; i < pts.length; i++) {
      const x = (i / (pts.length - 1)) * w;
      const y = h / 2 - clamp(pts[i] * 1.6, -1, 1) * (h * 0.42);
      i ? sctx.lineTo(x, y) : sctx.moveTo(x, y);
    }
    sctx.lineWidth = width;
    sctx.strokeStyle = `rgba(224, 116, 90, ${alpha})`;
    sctx.stroke();
  };
  // glow from stacked wide strokes; shadowBlur is a software blur every frame
  pass(14, 0.06); pass(8, 0.12); pass(4.5, 0.3); pass(2.2, 0.95);
}

function onMeters(s) {
  const peak = Math.max(s.peakL || 0, s.peakR || 0);
  const db = 20 * Math.log10(Math.max(peak, 1e-5));
  const target = clamp((db + 54) / 54);
  meter.shown = Math.max(target, meter.shown * 0.86);
  // Transient = fast level jumping well above the slow average, or just very loud.
  if ((peak > meter.slow * 1.7 && peak > 0.06) || peak > 0.8) meter.flash = Math.min(1, meter.flash + 0.55 + (peak - meter.slow));
  meter.slow = meter.slow * 0.9 + peak * 0.1;
  meter.flash *= 0.78;
  // A dead tube still flickers now and then.
  let lvl = meter.shown;
  if (lvl < 0.03) {
    if (meter.flicker > 0) meter.flicker--;
    else if (Math.random() < 0.012) meter.flicker = 2 + (Math.random() * 4 | 0);
    lvl = meter.flicker ? 0.25 + Math.random() * 0.5 : 0;
  }
  const lit = Math.max(lvl, meter.flash > 0.35 ? 1 : 0);
  // Only the tube and the spill move every frame; writing vars on the stage would restyle the whole UI at 30 Hz.
  const l3 = lit.toFixed(2), f3 = meter.flash.toFixed(2);
  if (l3 !== meter.l3 || f3 !== meter.f3) {
    meter.l3 = l3; meter.f3 = f3;
    tubeEl.style.setProperty('--level', l3);
    tubeEl.style.setProperty('--flash', f3);
    spillEl.style.opacity = Math.min(1, lit * 0.6 + meter.flash).toFixed(2);
  }
  const r2 = (s.rust || 0).toFixed(2);
  if (r2 !== meter.r2) { meter.r2 = r2; stage.style.setProperty('--rust', r2); }
  if (s.scope) lastScope = s.scope;
}
const scopePane = $('[data-pane="scope"]');
let drawn = null;
(function loop() {
  if (lastScope !== drawn && !scopePane.classList.contains('hidden')) { drawn = lastScope; drawScope(lastScope); }
  requestAnimationFrame(loop);
})();

/* ------------------------------------------------------------------ presets */
const crates = $('#crates'); crates.inert = true;
const shelf = $('#shelf'), chips = $('#chips'), search = $('#search');
let presetList = [], current = {}, filterTag = 'all', unlocked = false;

function markDirty() {
  if (!current.dirty) { current.dirty = true; renderPresetName(); }
}
function renderPresetName() {
  const el = $('#presetName');
  el.textContent = current.name || 'INIT';
  el.classList.toggle('dirty', !!current.dirty);
}
function applyPayload(r) {
  if (!r) return;
  if (r.current) current = r.current;
  if (r.list) presetList = r.list;
  if (r.unlocked !== undefined) unlocked = r.unlocked;
  renderPresetName();
  renderCrates();
}

const tagRot = t => ((t.split('').reduce((a, c) => a + c.charCodeAt(0), 0) % 7) - 3) * 0.7;
function allTags() {
  const count = {};
  presetList.forEach(p => (p.tags || []).forEach(t => { count[t] = (count[t] || 0) + 1; }));
  return Object.keys(count).sort((a, b) => count[b] - count[a] || a.localeCompare(b));
}
function visiblePresets() {
  const q = search.value.trim().toLowerCase();
  return presetList.filter(p => {
    if (filterTag === 'fav' && !p.favorite) return false;
    if (filterTag === 'mine' && p.factory) return false;
    if (filterTag === 'bday' && p.bank !== 'OCT 29') return false;
    if (!['all', 'fav', 'mine', 'bday'].includes(filterTag) && !(p.tags || []).includes(filterTag)) return false;
    if (!q) return true;
    return [p.name, p.author, p.notes, ...(p.tags || [])].join(' ').toLowerCase().includes(q);
  });
}
const STAR = '<svg viewBox="0 0 16 16"><path d="M8 1.2l2 4.6 5 .5-3.8 3.3 1.1 4.9L8 12l-4.3 2.5 1.1-4.9L1 6.3l5-.5z"/></svg>';
function renderCrates() {
  const base = [['all', 'ALL'], ['fav', 'FAVES', true], ['mine', 'MINE']];
  if (unlocked) base.push(['bday', 'OCT 29']);
  chips.innerHTML = [...base, ...allTags().map(t => [t, t.toUpperCase()])]
    .map(([k, l, star]) => `<button class="chip" data-tag="${esc(k)}" aria-pressed="${k === filterTag}" style="--r:${tagRot(k)}deg">${star ? STAR : ''}${esc(l)}</button>`).join('');
  const list = visiblePresets();
  shelf.innerHTML = list.length ? list.map(p => `
    <li data-id="${esc(p.id)}" class="${p.id === current.id ? 'current' : ''}">
      <span class="kind">${p.bank ? '<span class="bank">29</span>' : p.factory ? 'F' : 'U'}</span>
      <span class="pname">${esc(p.name)}</span>
      <span class="ptags">${(p.tags || []).slice(0, 3).map(t => `<span>${esc(t)}</span>`).join('')}</span>
      <button class="fav" aria-label="Favourite ${esc(p.name)}" aria-pressed="${!!p.favorite}">${STAR}</button>
    </li>`).join('') : `<li class="empty">NOTHING IN THESE CRATES. SAVE ONE.</li>`;
  const cur = presetList.find(p => p.id === current.id);
  $('#notes').textContent = cur ? [cur.author && `BY ${cur.author.toUpperCase()}`, cur.notes].filter(Boolean).join(' · ') : '';
  const isUser = current.id && current.id.startsWith('user:');
  $('#actDelete').disabled = !isUser;
  $('#actExport').disabled = !current.id;
}
const esc = s => String(s ?? '').replace(/[&<>"']/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));

chips.addEventListener('click', e => {
  const b = e.target.closest('.chip'); if (!b) return;
  filterTag = b.dataset.tag; renderCrates();
});
search.addEventListener('input', renderCrates);
shelf.addEventListener('click', async e => {
  const li = e.target.closest('li[data-id]'); if (!li) return;
  const id = li.dataset.id;
  if (e.target.closest('.fav')) {
    const p = presetList.find(x => x.id === id);
    applyPayload(await call('presetFavorite', id, !p.favorite));
  } else applyPayload(await call('presetLoad', id));
});

function openCrates(open) {
  crates.classList.toggle('open', open);
  crates.inert = !open;
  $('#cratesBtn').setAttribute('aria-expanded', String(open));
  if (open) { renderCrates(); setTimeout(() => $('li.current', shelf)?.scrollIntoView({ block: 'center' }), 40); }
  else showSaveForm(false);
}
$('#cratesBtn').onclick = () => openCrates(!crates.classList.contains('open'));
$('#presetName').onclick = () => openCrates(true);
$('#cratesClose').onclick = () => openCrates(false);
$('#prev').onclick = async () => applyPayload(await call('presetStep', -1));
$('#next').onclick = async () => applyPayload(await call('presetStep', 1));

const form = $('#saveForm');
function showSaveForm(on) {
  form.classList.toggle('hidden', !on);
  $('#cratesActions').classList.toggle('hidden', on);
  if (!on) return;
  form.name.value = current.id && current.id.startsWith('user:') ? `${current.name} 2` : current.name === 'INIT' ? '' : current.name;
  form.tags.value = (current.tags || []).join(', ');
  form.author.value = current.author && current.author !== 'WRECKAGE' ? current.author : (storage('author') || 'WRECKEM');
  form.notes.value = current.notes || '';
  $('#tagSuggest').innerHTML = allTags().map(t => `<button type="button" class="chip" data-t="${esc(t)}" style="--r:${tagRot(t)}deg">${esc(t.toUpperCase())}</button>`).join('');
  form.name.focus(); form.name.select();
}
$('#tagSuggest').addEventListener('click', e => {
  const b = e.target.closest('.chip'); if (!b) return;
  const tags = form.tags.value.split(',').map(s => s.trim()).filter(Boolean);
  if (!tags.includes(b.dataset.t)) tags.push(b.dataset.t);
  form.tags.value = tags.join(', ');
});
form.addEventListener('submit', async e => {
  e.preventDefault();
  const meta = {
    name: form.name.value.trim(),
    tags: form.tags.value.split(',').map(s => s.trim().toLowerCase()).filter(Boolean),
    author: form.author.value.trim(),
    notes: form.notes.value.trim(),
  };
  if (!meta.name) { form.name.focus(); return; }
  storage('author', meta.author);
  const r = await call('presetSave', meta, false);
  applyPayload(r);
  showSaveForm(false);
  setTimeout(() => $('li.current', shelf)?.scrollIntoView({ block: 'center' }), 40);
});
$('#saveCancel').onclick = () => showSaveForm(false);
$('#actSaveAs').onclick = () => showSaveForm(true);
$('#actSave').onclick = async () => {
  if (current.id && current.id.startsWith('user:')) applyPayload(await call('presetSave', {}, true));
  else showSaveForm(true);
};
let deleteArmed = 0;
$('#actDelete').onclick = async e => {
  const b = e.currentTarget;
  if (Date.now() - deleteArmed > 2500) { deleteArmed = Date.now(); b.textContent = 'SURE?'; setTimeout(() => { b.textContent = 'DELETE'; }, 2500); return; }
  deleteArmed = 0; b.textContent = 'DELETE';
  applyPayload(await call('presetDelete', current.id));
};
$('#actImport').onclick = async () => applyPayload(await call('presetImport'));
$('#actExport').onclick = () => call('presetExport', current.id);
$('#actFolder').onclick = () => call('presetReveal');
$('#actInit').onclick = async () => applyPayload(await call('presetInit'));

function storage(k, v) {
  try { if (v === undefined) return localStorage.getItem(`wreckage.${k}`); localStorage.setItem(`wreckage.${k}`, v); } catch { /* no storage */ }
  return null;
}

/* ------------------------------------------------------------------ keyboard play */
const KEYS = 'awsedftgyhujkolp;\'';
let octave = 4;
const held = new Map();
const typing = e => e.target.closest('input, textarea') || e.metaKey || e.ctrlKey || e.altKey;
let keysPlay = false; // standalone only: in a DAW the host owns the keyboard
addEventListener('keydown', e => {
  if (typing(e) || e.repeat) return;
  const k = e.key.toLowerCase();
  if (k === 'z') { octave = Math.max(0, octave - 1); return; }
  if (k === 'x') { octave = Math.min(8, octave + 1); return; }
  if (k === 'escape') { openCrates(false); hidePermit(); return; }
  if (crates.classList.contains('open') && (k === 'arrowdown' || k === 'arrowup')) {
    const list = visiblePresets(); if (!list.length) return;
    const i = list.findIndex(p => p.id === current.id);
    const n = list[(i + (k === 'arrowdown' ? 1 : -1) + list.length) % list.length];
    call('presetLoad', n.id).then(applyPayload);
    e.preventDefault(); return;
  }
  const i = KEYS.indexOf(k);
  if (!keysPlay || i < 0 || held.has(k) || document.activeElement?.getAttribute('role') === 'slider') return;
  const note = 12 * octave + i;
  held.set(k, note);
  call('note', note, 0.8);
});
addEventListener('keyup', e => {
  const k = e.key.toLowerCase();
  if (held.has(k)) { call('note', held.get(k), 0); held.delete(k); }
});
const releaseKeys = () => { held.forEach(n => call('note', n, 0)); held.clear(); };

// In a DAW, give the keyboard back to the host after every click so its typing keyboard sees key-ups.
// Text fields keep focus while they're in use.
const isField = el => el && el.closest && el.closest('input, textarea');
const yieldFocus = () => { if (!keysPlay && !isField(document.activeElement)) call('yieldFocus'); };
document.addEventListener('pointerup', () => setTimeout(yieldFocus, 0), true);
document.addEventListener('focusout', e => { if (isField(e.target)) setTimeout(yieldFocus, 0); });
addEventListener('blur', releaseKeys);
document.addEventListener('visibilitychange', releaseKeys);

/* ------------------------------------------------------------------ about + birthday */
const permit = $('#permit');
function hidePermit() { permit.classList.add('hidden'); }
$('#wordmark').onclick = e => { e.stopPropagation(); permit.classList.toggle('hidden'); };
permit.onclick = hidePermit;

let tagClicks = [];
$('#tag').onclick = () => {
  const now = Date.now();
  tagClicks = tagClicks.filter(t => now - t < 2500).concat(now);
  if (tagClicks.length >= 5) { tagClicks = []; birthday(); }
};

// Happy Birthday, one octave down, beats of 0.42s.
const SONG = [[55,.75],[55,.25],[57,1],[55,1],[60,1],[59,2],[55,.75],[55,.25],[57,1],[55,1],[62,1],[60,2],
  [55,.75],[55,.25],[67,1],[64,1],[60,1],[59,1],[57,2],[65,.75],[65,.25],[64,1],[60,1],[62,1],[60,3]];
let songTimers = [];
function stopSong() { songTimers.forEach(clearTimeout); songTimers = []; for (let n = 50; n < 72; n++) call('note', n, 0); }
async function birthday() {
  const lo = $('#lightsOut');
  lo.classList.remove('hidden');
  openCrates(false); hidePermit();
  const r = await call('presetUnlock');
  applyPayload(r);
  const bday = presetList.find(p => p.name === 'HAPPY BDAY JACOB');
  if (bday) applyPayload(await call('presetLoad', bday.id));
  stopSong();
  let t = 1300;
  for (const [n, beats] of SONG) {
    const d = beats * 420;
    songTimers.push(setTimeout(() => call('note', n, 0.75), t));
    songTimers.push(setTimeout(() => call('note', n, 0), t + d * 0.85));
    t += d;
  }
  storage('bday', '2026');
}
$('#lightsOut').onclick = () => { stopSong(); $('#lightsOut').classList.add('hidden'); filterTag = 'bday'; openCrates(true); };

/* ------------------------------------------------------------------ boot */
async function boot() {
  if (window.__JUCE__) juceBridge(window.__JUCE__);
  else {
    await new Promise((res, rej) => { const s = document.createElement('script'); s.src = 'dev/mock.js'; s.onload = res; s.onerror = rej; document.head.appendChild(s); });
    ({ call, listen } = window.WRECKAGE_MOCK);
  }
  const init = await call('init');
  init.params.forEach(p => { P[p.id] = { ...p, v: p.value, views: [] }; });
  $('#ver').textContent = init.version || '1.0.0';
  keysPlay = init.standalone !== false;
  yieldFocus();

  $$('.knob[data-p]').forEach(buildKnob);
  $$('.fader').forEach(buildFader);
  $$('.seg').forEach(buildSeg);
  $$('.stepper').forEach(buildStepper);
  $$('.toggle').forEach(buildToggle);
  $$('.cycle').forEach(buildCycle);
  buildSelector($('#modeSel'));
  bind('lfo_sync', () => {
    const on = P.lfo_sync.v > 0.5;
    $('#lfoRate').classList.toggle('hidden', on);
    $('#lfoDiv').classList.toggle('hidden', !on);
  });
  P.lfo_sync.views.at(-1)();
  applyPayload(init);

  listen('state', s => {
    if (s.params) for (const [id, [v]] of Object.entries(s.params)) if (!dragging.has(id)) setParam(id, v, false);
    if (s.preset) { current = s.preset; renderPresetName(); if (crates.classList.contains('open')) renderCrates(); }
    onMeters(s);
  });

  const d = new Date();
  if (d.getMonth() === 9 && d.getDate() === 29 && storage('bday') !== '2026') setTimeout(birthday, 1500);
}
boot();
