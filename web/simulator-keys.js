// SPDX-License-Identifier: GPL-3.0-only
export const pianoCodes=['KeyA','KeyW','KeyS','KeyE','KeyD','KeyF','KeyT','KeyG','KeyY','KeyH','KeyU','KeyJ','KeyK','KeyO','KeyL','KeyP','Semicolon'];
export function padShortcut(code,shift=false){
  const m=/^(?:Digit|Numpad)([1-8])$/.exec(code);
  return m?Number(m[1])-1+(shift?8:0):-1;
}
export function bindSimulatorKeyboard(target,{enabled,press,release,keys,pads}){
  const down=e=>{
    if(!enabled()||e.ctrlKey||e.metaKey||e.altKey||e.target.closest('input,select,textarea,[contenteditable=true],.sim-tour,.sim-finish-picker'))return;
    const pad=padShortcut(e.code,e.shiftKey),base=pianoCodes.indexOf(e.code),key=base<0?-1:base+(e.shiftKey?12:0);
    if(pad<0&&key<0)return;
    e.preventDefault();if(e.repeat)return;
    press('q'+e.code,pad>=0?3:2,pad>=0?pad:key,100,pad>=0?pads[pad]:keys[key]);
  };
  // Use the physical key as owner: releasing Shift before the number still ends the right pad.
  const up=e=>release('q'+e.code);
  target.addEventListener('keydown',down);target.addEventListener('keyup',up);
  return ()=>{target.removeEventListener('keydown',down);target.removeEventListener('keyup',up);};
}
