"""Scoped archive and x64 execution checks for the 400-tick UI variant."""
import contextlib
import hashlib
import io
import json
from pathlib import Path
import struct
import tempfile
import unittest
import zipfile
from patch_inputalign_400 import patch
from verify_inputalign_80 import emulator, BASE, MEM, STACK
from unicorn.x86_const import (
    UC_X86_REG_RAX, UC_X86_REG_RBX, UC_X86_REG_RCX, UC_X86_REG_RDX,
    UC_X86_REG_RSP, UC_X86_REG_R9, UC_X86_REG_R12, UC_X86_REG_EBP,
)
ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'InputAlign-v2.7.1-alpha.1-windows.geode'
BASE80 = ROOT / 'InputAlign-v2.7.1-alpha.1-80ticks-windows.geode'


class FourHundredTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.scratch = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.scratch.cleanup)
        cls.output = Path(cls.scratch.name) / '400.geode'
        with contextlib.redirect_stdout(io.StringIO()):
            patch(SOURCE, cls.output)
        with zipfile.ZipFile(cls.output) as archive:
            cls.dll = archive.read('ai.predictivebuffer.dll')

    def test_archive_metadata_and_exact_delta(self):
        with zipfile.ZipFile(BASE80) as old, zipfile.ZipFile(self.output) as new:
            self.assertIsNone(new.testzip())
            self.assertEqual(old.namelist(), new.namelist())
            for name in old.namelist():
                if name not in ('mod.json', 'ai.predictivebuffer.dll', 'binary-patch-provenance.json'):
                    self.assertEqual(old.read(name), new.read(name), name)
            meta = json.loads(new.read('mod.json'))
            for key in ('normal-ticks', 'wave-ticks', 'spam-interval'):
                self.assertEqual(meta['settings'][key]['max'], 400)
            for key in ('normal-ticks', 'wave-ticks'):
                self.assertEqual(meta['settings'][key]['default'], 400)
            self.assertIs(meta['settings']['spam-guard']['default'], False)
            normalized = bytearray(self.dll)
            for rva in (0x49783, 0x498f3, 0x49963):
                offset = rva - 0xc00
                self.assertEqual(normalized[offset:offset+9], bytes.fromhex('48c744242090010000'))
                normalized[offset:offset+9] = bytes.fromhex('48c744242050000000')
            self.assertEqual(normalized, old.read('ai.predictivebuffer.dll'))
            provenance = json.loads(new.read('binary-patch-provenance.json'))
            self.assertEqual(provenance['patchedDllSha256'], hashlib.sha256(self.dll).hexdigest())
            self.assertEqual(provenance['previousPatch'], json.loads(old.read('binary-patch-provenance.json')))
            self.assertIs(provenance['gameplayTested'], False)
            # All other settings, including macro analysis and time budget, stay intact.
            old_meta = json.loads(old.read('mod.json'))
            for key in ('normal-ticks', 'wave-ticks'):
                meta['settings'][key]['default'] = 80
            for key in ('normal-ticks', 'wave-ticks', 'spam-interval'):
                meta['settings'][key]['max'] = 80
            meta['name'] = old_meta['name']
            self.assertEqual(meta, old_meta)

    def test_actual_menu_callbacks(self):
        for start, name, minimum in [(0x49770,'normal-ticks',0), (0x498e0,'wave-ticks',0), (0x49950,'spam-interval',4)]:
            with self.subTest(setting=name):
                uc = emulator(self.dll)
                uc.mem_write(MEM+8, struct.pack('<Q',MEM+0x1000))
                uc.mem_write(MEM+0x12f0, struct.pack('<I',1))
                uc.mem_write(MEM+0x2000, struct.pack('<i',1))
                uc.reg_write(UC_X86_REG_RCX,MEM)
                uc.reg_write(UC_X86_REG_RDX,MEM+0x2000)
                uc.emu_start(BASE+start, BASE+0x497c0, count=100)
                sp = uc.reg_read(UC_X86_REG_RSP)
                self.assertEqual(struct.unpack('<Q',uc.mem_read(sp+0x28,8))[0],400)
                self.assertEqual(uc.reg_read(UC_X86_REG_R9), minimum)
                addr = uc.reg_read(UC_X86_REG_RDX)
                self.assertEqual(bytes(uc.mem_read(addr,len(name))).decode(),name)

    def test_actual_clamp_boundaries(self):
        for minimum in (0,4):
            for current in (0,4,79,80,399,400,401):
                for delta in (-1000,-1,0,1,1000):
                    with self.subTest(minimum=minimum,current=current,delta=delta):
                        uc=emulator(self.dll)
                        uc.reg_write(UC_X86_REG_RAX,current)
                        uc.reg_write(UC_X86_REG_EBP,delta & 0xffffffff)
                        uc.reg_write(UC_X86_REG_RBX,minimum)
                        uc.reg_write(UC_X86_REG_R12,400)
                        uc.mem_write(STACK+0x28,struct.pack('<Q',minimum))
                        uc.mem_write(STACK+0xb0,struct.pack('<Q',400))
                        uc.emu_start(BASE+0x49854,BASE+0x4987f,count=30)
                        ptr=uc.reg_read(UC_X86_REG_RBX)
                        self.assertEqual(struct.unpack('<q',uc.mem_read(ptr,8))[0],max(minimum,min(400,current+delta)))

    def test_reproducible_output(self):
        other = Path(self.scratch.name) / 'again.geode'
        with contextlib.redirect_stdout(io.StringIO()):
            patch(SOURCE,other)
        self.assertEqual(other.read_bytes(),self.output.read_bytes())

    def test_reject_invalid_source_without_touching_destination(self):
        invalid=Path(self.scratch.name)/'invalid.geode'
        invalid.write_bytes(b'not a package')
        before=self.output.read_bytes()
        with self.assertRaises(ValueError), contextlib.redirect_stdout(io.StringIO()):
            patch(invalid,self.output)
        self.assertEqual(before,self.output.read_bytes())
        with self.assertRaises(ValueError):
            patch(invalid,invalid)
        self.assertEqual(invalid.read_bytes(),b'not a package')


if __name__ == '__main__':
    unittest.main()
