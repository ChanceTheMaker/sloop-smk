#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read Sloop CPU1 counters; never writes parameters, storage or firmware."""
import argparse,json,time
from sloop_smk_storage_diag import MIDIReader,u7
PREFIX=[0x7d,ord('S'),ord('C'),ord('1'),1,0]
NAMES=['online','jobs','pairs','timeouts','rejected','faults','max_job_ticks','max_wait_ticks','barred']
def main():
    import mido
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--input',required=True);ap.add_argument('--output',required=True)
    ap.add_argument('--seconds',type=int,default=0)
    a=ap.parse_args()
    with mido.open_input(a.input) as inp,mido.open_output(a.output) as out:
        c=MIDIReader(inp,out,mido)
        info=c.exchange([125,70,76,1],[])
        firmware=bytes(info[:info.index(0)]).decode('ascii')
        if not firmware.startswith('SLOOP SMK '): raise ValueError('Expected Sloop SMK, got '+firmware)
        names=NAMES.copy()
        if firmware.startswith('SLOOP SMK 2.8.'):
            names[2]='render_blocks'  # 2.8 submits whole render units, not FM6 pairs.
        print(firmware,flush=True)
        end=time.monotonic()+a.seconds
        while True:
            out.send(mido.Message('sysex',data=PREFIX))
            deadline=time.monotonic()+3;raw=None
            while time.monotonic()<deadline:
                msg=inp.poll()
                if msg and msg.type=='sysex' and list(msg.data[:len(PREFIX)])==PREFIX:
                    raw=list(msg.data[len(PREFIX):]);break
                time.sleep(.003)
            if raw is None: raise TimeoutError('No CPU1 diagnostic reply; close other MIDI apps')
            if len(raw)!=45: raise ValueError('Invalid CPU1 diagnostics length')
            print(json.dumps(dict(zip(names,[u7(raw[i:i+5]) for i in range(0,45,5)]))),flush=True)
            if time.monotonic()>=end: break
            time.sleep(2)
if __name__=='__main__': main()
