# SPDX-License-Identifier: GPL-3.0-only
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).parents[1] / 'tools'))
import sloop_smk_storage_reset as reset
from test_sloop_smk_storage_diag import legacy, BackupTest

class ResetTest(unittest.TestCase):
    def setUp(self):
        self.data = {(s,c): legacy(s) if s<3 else bytes([c])*4096 for s in range(4) for c in range(2)}

    def test_plan_and_exact_postcondition(self):
        steps=reset.plan(self.data,dict(self.data),[0,1,2])
        sent=[]
        reset.execute(steps,lambda op,args: (sent.append((op,args)) or [0,args[0]]),lambda _:None)
        self.assertEqual([args[0] for _,args in sent],[0,1,2])
        self.assertTrue(all(op==1 and args[-5:]==list(b'RESET') for op,args in sent))
        after=dict(self.data)
        for s in range(3):after[s,1]=reset.tombstone(s)
        reset.verify_after(self.data,after,[0,1,2])
        after[3,0]=bytes(4096-1)+b'\x01'
        with self.assertRaises(ValueError):reset.verify_after(self.data,after,[0,1,2])

    def test_refuses_changed_device_and_other_data(self):
        for key in self.data:
            changed=dict(self.data);value=bytearray(changed[key]);value[0]^=1;changed[key]=bytes(value)
            with self.assertRaises(ValueError):reset.plan(self.data,changed,[0,1,2])
        for key in ((0,0),(0,1)):
            wrong=dict(self.data);wrong[key]=bytes(4096)
            with self.assertRaises(ValueError):reset.plan(wrong,wrong,[0])
        for slots in ([],[3],[0,0],[-1]):
            with self.assertRaises(ValueError):reset.plan(self.data,self.data,slots)
        with self.assertRaises(ValueError):reset.reset_args(3,0,0)

    def test_failure_stops_without_retry(self):
        calls=[]
        def fail(op,args):calls.append(args);return [7,args[0]]
        with self.assertRaises(ValueError):reset.execute(reset.plan(self.data,self.data,[0,1,2]),fail,lambda _:None)
        self.assertEqual(len(calls),1)
        calls.clear()
        def lost(op,args):calls.append(args);raise TimeoutError()
        with self.assertRaises(TimeoutError):reset.execute(reset.plan(self.data,self.data,[0,1,2]),lost,lambda _:None)
        self.assertEqual(len(calls),1)

    def test_transport_timeout_sends_once(self):
        class Midi:
            @staticmethod
            def Message(*args,**kwargs):return kwargs
        class Out:
            def __init__(self):self.sent=[]
            def send(self,msg):self.sent.append(msg)
        out=Out();link=reset.ResetLink(None,out,Midi)
        with patch.object(reset.time,'monotonic',side_effect=[0,9]):
            with self.assertRaises(TimeoutError):link.request(1,reset.reset_args(0,1,2))
        self.assertEqual(len(out.sent),1)
        with self.assertRaises(ValueError):link.request(2,[])
        self.assertEqual(len(out.sent),1)

    def test_verified_archive_required(self):
        fixture=BackupTest()
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'capture'
            reset.capture(fixture.request,path,'test')
            data=reset.checked_archive(path)
            self.assertEqual(data[0,0],legacy(0))
            (path/'slot-1-copy-0-pass-1.bin').write_bytes(bytes(4096))
            with self.assertRaises(ValueError):reset.checked_archive(path)

if __name__=='__main__':unittest.main()
