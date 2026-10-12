# SPDX-License-Identifier: GPL-3.0-only
"""Linked-image checks for the opt-in SMK CPU1 renderer (not a hardware test)."""
import re

def audit_multicore(symbols, disassembly, enabled):
    rows = {}
    for line in symbols.splitlines():
        fields = line.split()
        if fields and re.fullmatch(r'[0-9a-f]+', fields[0]):
            rows[fields[-1]] = fields
    def addr(name):
        if name not in rows:
            raise ValueError('Missing CPU1 audit symbol: ' + name)
        return int(rows[name][0], 16)
    stacks = addr('_cpu1_stacks_hi') - addr('_cpu1_stacks_lo')
    worker_names = ('fm1_core1_start', 'fm1_core1_main', 'audio_worker_loop')
    if not enabled:
        if stacks or any(name in rows for name in worker_names):
            raise ValueError('Serial build unexpectedly contains CPU1 code or stacks')
        return {'enabled': False, 'stack_bytes': 0}
    addr('fm1_core1_start')
    lo, hi = addr('_dsp_start'), addr('_dsp_end')
    for name in worker_names[1:]:
        row = rows.get(name, [])
        if '.dsp_text' not in row or not lo <= addr(name) < addr(name) + int(row[-2], 16) <= hi:
            raise ValueError('CPU1 polling must reside entirely in copied SRAM: ' + name)
    if stacks != 8192 or addr('_cpu1_stacks_lo') % 32:
        raise ValueError('CPU1 needs aligned private 8 KiB stacks')
    if not addr('_pool_end') <= addr('_cpu1_stacks_lo') < addr('_cpu1_stacks_hi') <= 0x01C74000:
        raise ValueError('CPU1 stacks overlap shared pool/working arena')
    begin = addr('audio_worker_loop')
    end = begin + int(rows['audio_worker_loop'][-2], 16)
    checked = 0
    callbacks = 0
    for line in disassembly.splitlines():
        match = re.match(r'^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2}\s+)+(.+)$', line)
        if not match or not begin <= int(match[1], 16) < end:
            continue
        checked += 1
        asm = match[2]
        if re.search(r'\b(?:call|icall)\b', asm):
            # The job callback is the only permitted call. At rest CPU1 must
            # never fetch XIP while CPU0 services NOR reads/erase/program.
            if not re.search(r'\b(?:call|icall)\s+r\d+\s*$', asm):
                raise ValueError('CPU1 polling has a non-register call: ' + line)
            callbacks += 1
        if 'goto' in asm:
            target = re.search(r':\s*([0-9a-f]+)\s*>', asm)
            if not target or not begin <= int(target[1], 16) < end:
                raise ValueError('CPU1 polling branches outside its RAM body: ' + line)
    if checked < 20 or callbacks != 1:
        raise ValueError('Incomplete/unexpected CPU1 polling disassembly')
    return {'enabled': True, 'stack_bytes': stacks, 'idle_in_sram': True,
            'poll_instructions_checked': checked, 'callback_calls': callbacks,
            'pool_headroom_bytes': 0x01C74000 - addr('_cpu1_stacks_hi')}
