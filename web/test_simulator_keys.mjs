import assert from 'node:assert/strict';
import {padShortcut,bindSimulatorKeyboard} from './simulator-keys.js';
for(let n=1;n<=8;n++)for(const prefix of ['Digit','Numpad']){
 assert.equal(padShortcut(prefix+n),n-1);assert.equal(padShortcut(prefix+n,true),n+7);
}
assert.equal(padShortcut('Digit0'),-1);assert.equal(padShortcut('KeyA'),-1);
const handlers={},active=new Map(),messages=[];let enabled=true;
const target={addEventListener:(k,fn)=>handlers[k]=fn,removeEventListener:k=>delete handlers[k]};
const dispose=bindSimulatorKeyboard(target,{enabled:()=>enabled,keys:[],pads:[],
 press:(owner,kind,id)=>{if(!active.has(owner)){active.set(owner,[kind,id]);messages.push([kind,id,1]);}},
 release:owner=>{const v=active.get(owner);if(v){messages.push([...v,0]);active.delete(owner);}}});
const event=(code,extra={})=>({code,shiftKey:false,repeat:false,target:{closest:()=>null},preventDefault(){},...extra});
handlers.keydown(event('Digit2',{shiftKey:true}));handlers.keydown(event('Digit2',{repeat:true}));handlers.keyup(event('Digit2'));
assert.deepEqual(messages,[[3,9,1],[3,9,0]],'Shift release and auto-repeat must not produce stuck or duplicate pads');
for(const extra of [{ctrlKey:true},{altKey:true},{metaKey:true},{target:{closest:()=>true}}])handlers.keydown(event('Digit1',extra));
assert.equal(messages.length,2);enabled=false;handlers.keydown(event('Digit1'));assert.equal(messages.length,2);
enabled=true;handlers.keydown(event('KeyA'));handlers.keyup(event('KeyA'));assert.deepEqual(messages.slice(-2),[[2,0,1],[2,0,0]]);
handlers.keydown(event('KeyA',{shiftKey:true}));handlers.keyup(event('KeyA'));assert.deepEqual(messages.slice(-2),[[2,12,1],[2,12,0]],'Shift plays an octave higher and releases even after Shift comes up');
handlers.keydown(event('Semicolon',{shiftKey:true}));handlers.keyup(event('Semicolon'));assert.deepEqual(messages.slice(-2),[[2,28,1],[2,28,0]]);
dispose();assert.deepEqual(handlers,{});
console.log('PASS: all 16 pads, number-pad aliases, Shift release, repeat suppression, typing/modifier guards, inactive view and piano coexistence.');
