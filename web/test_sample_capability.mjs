// SPDX-License-Identifier: GPL-3.0-only
// Connect with the physical b2 zero-slot reply, without touching USB or flash.
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const html = readFileSync(new URL('./editor.html', import.meta.url), 'utf8');
const section = (start, end) => html.slice(html.indexOf(start), html.indexOf(end, html.indexOf(start)));
const proto = section('/*PROTO-BEGIN*/', '/*PROTO-END*/');
const noop = () => {};
let cleared = 0, watches = 0;
const nodes = new Map();
const ctx = vm.createContext({console, setTimeout, clearTimeout,
  $: id => { if (!nodes.has(id)) nodes.set(id, {replaceChildren: () => cleared++}); return nodes.get(id); },
  chopUI: noop, chopRender: noop, getDescOrNull: noop, fm6Names: noop,
  libAdopt: noop, getDump: async () => ({}), renderPanels: noop, loadSteps: noop,
  readMixer: noop, watchReached: () => { watches++; },
  renderLive: noop, renderLib: noop, renderBank: noop, renderSysinfo: noop, sayK: noop,
});
vm.runInContext(proto + `
  startWatch = async () => { globalThis.watchReached(); return false; };
  let dev, busy = 0, lang = 'en';
  // Include a stale sample UI from a previous browser-synth connection.
  const smpPage = {slot: 3, ui: {lang, n: 0, slots: {children: []}, erase: {}}};
  const smpBusy = false;
  function setLabel() {}
  function t(x) { return x; }
  ${section('function renderSamples()', 'async function smpSendPage()')}
  ${section('async function load(d)', 'async function getDescOrNull')}
  ${section('async function smpInfo()', 'async function smpUpload(')}
  function buildUI() { renderSamples(); }
  globalThis.api = {parse, CMD, load, smpInfo, smpButtons,
    device: d => { dev = d; }, busy: () => busy};
`, ctx);
const {api} = ctx;
// firmware/src/smk37_editor.c command 15 explicitly replies with these two bytes.
const noSlots = [0, 0];
assert.equal(api.parse[api.CMD.SMP_INFO](noSlots), null);
for (const count of [3, 4]) {
  const reply = [count, 80, ...Array.from({length: count}, () => [0, 0, 0]).flat()];
  const info = api.parse[api.CMD.SMP_INFO](reply);
  assert.equal(info.nslots, count);
  assert.equal(info.slots.length, count);
  assert.equal(info.slots[0].zones, 0);
}
assert.throws(() => api.parse[api.CMD.SMP_INFO]([3, 80]));
const originalInfo = api.parse[api.CMD.INFO];
api.parse[api.CMD.INFO] = () => ({version:'SLOOP SMK 2.6.0-b2', pcount:0, gcount:0, pe0:0, nengines:0, proto:0, ntrk:0});
const requests = [];
const d = {pdesc:[], gdesc:[], names:[], slotUsed:[], link:{request: async ([cmd]) => {
  requests.push(cmd);
  if (cmd === api.CMD.SMP_INFO) return noSlots;
  if (cmd === api.CMD.PROJECT) return [2, 0, 0];
  if (cmd === api.CMD.INFO) return [];
  throw new Error(`Unexpected command ${cmd}`);
}}};
api.device(d);
await api.load(d);
assert.equal(d.smp, null);
assert.equal(watches, 1, 'connection reaches live-watch initialization');
assert.equal(cleared, 1, 'sample controls cleared instead of rendering nonexistent slot');
assert.equal(api.busy(), 0);
api.smpButtons(); // A previous browser-mode sample UI must not re-enable upload/play.
ctx.rq = r => d.link.request(r);
await api.smpInfo();
assert.equal(d.smp, null, 'sample refresh preserves unavailable capability');
api.parse[api.CMD.INFO] = originalInfo;
console.log('PASS: b2 zero-slot connection completes; refresh and stale UI safe; 3/4-slot and malformed replies checked');
