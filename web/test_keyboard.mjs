// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import vm from 'node:vm';
const html = readFileSync(new URL('editor.html', import.meta.url), 'utf8');
const proto = readFileSync(new URL('keyboard.js', import.meta.url), 'utf8');
const Keyboard = vm.runInNewContext(proto + ';SloopKeyboard', { console, setTimeout, clearTimeout });
const sent = [], output = { state: 'connected', send: b => sent.push(Array.from(b)) };
const k = new Keyboard();
k.setChannel(0);
assert.equal(k.press('A', 60, 100), false);
k.connect(output);
k.press('A', 60, 100); k.press('A', 60, 100); k.press('touch1', 60, 100);
k.press('D', 64, 96); k.press('G', 67, 110);
assert.deepEqual(sent.splice(0), [[0x90, 60, 100], [0x90, 64, 96], [0x90, 67, 110]]);
k.release('A'); assert.equal(sent.length, 0);
k.release('touch1'); assert.deepEqual(sent.splice(0), [[0x80, 60, 0]]);
k.setSustain(true); k.releaseAll();
assert.deepEqual(sent.splice(0), [[0x80, 64, 0], [0x80, 67, 0]]);
k.press('A', 60, 100); k.setChannel(9); k.release('A'); k.press('A', 60, 100);
assert.deepEqual(sent.splice(0), [[0x90, 60, 100], [0x80, 60, 0], [0x99, 60, 100]]);
k.press('A', 62, 100); // pointer glissando releases its previous note
assert.deepEqual(sent.splice(0), [[0x89, 60, 0], [0x99, 62, 100]]);
k.connect(null);
assert.deepEqual(sent.splice(0), [[0x89, 62, 0]]);
k.connect(output); assert.equal(sent.length, 0); assert.equal(k.held.size, 0);
k.press('A', 128, 100); k.press('A', -1, 100); k.press('A', 60, 0); k.press('A', 60, NaN);
assert.equal(sent.length, 0);
k.connect({ send: () => { throw Error('unplugged'); } });
assert.equal(k.press('A', 60, 100), false); assert.equal(k.held.size, 0);
k.connect(output); k.press('A', 60, 100); k.setSustain(true); sent.length = 0;
k.panic();
assert.deepEqual(sent, [[0x89, 60, 0]]);
assert.equal(k.sustain, false); assert.equal(k.held.size, 0);
// Parse the complete script too, including UI wiring, without requiring a DOM.
new vm.Script(html.match(/<script>([\s\S]*?)<\/script>/)[1]);
console.log('Browser keyboard: chords, shared notes, repeats, release, sustain, channel changes, disconnect, glissando, bounds and send errors passed.');

// Sloop sustain owns deferred releases locally; firmware does not implement CC64.
k.connect(output); sent.length=0; k.setSustain(true); k.press('held',64,96); k.release('held');
assert.deepEqual(sent.splice(0), [[0x99,64,96]]); k.setSustain(false);
assert.deepEqual(sent.splice(0), [[0x89,64,0]]);
