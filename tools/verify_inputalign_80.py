"""Static package checks and isolated x64 execution. Requires unicorn==2.1.4.
Does NOT test Geometry Dash gameplay or Geode integration.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys
import zipfile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_64, UC_HOOK_CODE
from unicorn.x86_const import *
from patch_inputalign_80 import PATCHES, PACKAGE_SHA

BASE = 0x180000000
MEM = 0x2000000
STACK = MEM + 0x10000


def emulator(dll):
    uc = Uc(UC_ARCH_X86, UC_MODE_64)
    uc.mem_map(BASE, 0xc1000)
    pe = struct.unpack_from('<I', dll, 0x3c)[0]
    count = struct.unpack_from('<H', dll, pe+6)[0]
    size = struct.unpack_from('<H', dll, pe+20)[0]
    for i in range(count):
        header = pe+24+size+i*40
        _, rva, length, offset = struct.unpack_from('<IIII', dll, header+8)
        uc.mem_write(BASE+rva, dll[offset:offset+length])
    uc.mem_map(MEM, 0x20000)
    uc.reg_write(UC_X86_REG_RSP, STACK)
    return uc


def verify(original_path, patched_path):
    if not __debug__:
        raise RuntimeError(
            'Verification cannot run with Python optimization; remove -O/-OO and PYTHONOPTIMIZE'
        )
    with zipfile.ZipFile(original_path) as old, zipfile.ZipFile(patched_path) as new:
        assert hashlib.sha256(Path(original_path).read_bytes()).hexdigest() == PACKAGE_SHA
        assert new.testzip() is None
        assert old.namelist() == new.namelist()
        changed = {'mod.json', 'ai.predictivebuffer.dll', 'binary-patch-provenance.json'}
        for name in old.namelist():
            if name not in changed:
                assert old.read(name) == new.read(name), name
        a, b = old.read('ai.predictivebuffer.dll'), new.read('ai.predictivebuffer.dll')
        assert len(a) == len(b)
        permitted = set()
        for rva, before, after, _ in PATCHES:
            pos = rva - 0xc00
            assert a[pos:pos+len(bytes.fromhex(before))] == bytes.fromhex(before)
            assert b[pos:pos+len(bytes.fromhex(after))] == bytes.fromhex(after)
            permitted.update(range(pos,pos+len(bytes.fromhex(after))))
        for phrase in (b'up to 40 ticks', b'hasta 40 ticks'):
            pos = a.index(phrase)
            assert b[pos:pos+len(phrase)] == phrase.replace(b'40', b'80')
            permitted.update(range(pos,pos+len(phrase)))
        assert {i for i,(x,y) in enumerate(zip(a,b)) if x != y} <= permitted
        meta = json.loads(new.read('mod.json'))
        for name in ('normal-ticks','wave-ticks','spam-interval'):
            assert meta['settings'][name]['max'] == 80
        assert meta['settings']['normal-ticks']['default'] == 80
        assert meta['settings']['wave-ticks']['default'] == 80
        assert meta['settings']['spam-guard']['default'] is False
        assert json.loads(new.read('binary-patch-provenance.json'))['patchedDllSha256'] == hashlib.sha256(b).hexdigest()
    cases = 0
    # Execute actual UI callbacks until the shared setting-adjustment function.
    for start, name, minimum in [(0x49770,'normal-ticks',0),(0x498e0,'wave-ticks',0),(0x49950,'spam-interval',4)]:
        uc = emulator(b)
        uc.mem_write(MEM+8, struct.pack('<Q',MEM+0x1000))
        uc.mem_write(MEM+0x12f0,struct.pack('<I',1))
        uc.mem_write(MEM+0x2000,struct.pack('<i',1))
        uc.reg_write(UC_X86_REG_RCX,MEM)
        uc.reg_write(UC_X86_REG_RDX,MEM+0x2000)
        uc.emu_start(BASE+start, BASE+0x497c0, count=100)
        sp = uc.reg_read(UC_X86_REG_RSP)
        assert struct.unpack('<Q',uc.mem_read(sp+0x28,8))[0] == 80
        assert uc.reg_read(UC_X86_REG_R9) == minimum
        addr = uc.reg_read(UC_X86_REG_RDX)
        assert bytes(uc.mem_read(addr,len(name))).decode() == name
        cases += 1
    # Execute the original adjustment/clamp instructions with boundary values.
    for minimum in (0,4):
        for current in (0,4,39,40,41,79,80):
            for delta in (-100,-1,0,1,100):
                uc = emulator(b)
                uc.reg_write(UC_X86_REG_RAX,current)
                uc.reg_write(UC_X86_REG_EBP,delta & 0xffffffff)
                uc.reg_write(UC_X86_REG_RBX,minimum)
                uc.reg_write(UC_X86_REG_R12,80)
                uc.mem_write(STACK+0x28,struct.pack('<Q',minimum))
                uc.mem_write(STACK+0xb0,struct.pack('<Q',80))
                uc.emu_start(BASE+0x49854,BASE+0x4987f,count=30)
                ptr = uc.reg_read(UC_X86_REG_RBX)
                result = struct.unpack('<q',uc.mem_read(ptr,8))[0]
                assert result == max(minimum,min(80,current+delta)), (current,delta,result)
                cases += 1
    for candidate in (0,2,39,40,41,79,80,81,100):
        uc = emulator(b)
        uc.reg_write(UC_X86_REG_R12,MEM)
        uc.reg_write(UC_X86_REG_EBX,candidate)
        uc.reg_write(UC_X86_REG_EBP,2)
        uc.emu_start(BASE+0x4bbc3,BASE+0x4bbd9,count=20)
        assert struct.unpack('<I',uc.mem_read(MEM,4))[0] == min(80,max(2,candidate))
        cases += 1
    for current in (40,80):
        for p90 in (0,39,40,41,76,78,79,80,100):
            uc = emulator(b)
            uc.reg_write(UC_X86_REG_ESI,current)
            uc.reg_write(UC_X86_REG_EBP,p90)
            uc.emu_start(BASE+0x4ba31,BASE+0x4ba5f,count=30)
            expected = min(80,p90+2) if p90 < current-1 else min(76,current)+4
            assert uc.reg_read(UC_X86_REG_ESI) == expected
            cases += 1
    for distance in (0,39,40,41,79,80,81,100):
        uc = emulator(b)
        uc.reg_write(UC_X86_REG_R9D,distance)
        uc.reg_write(UC_X86_REG_R10D,distance)
        uc.emu_start(BASE+0x2c00e,BASE+0x2c01d,count=10)
        assert bool(uc.reg_read(UC_X86_REG_R9D)&0xff) == (distance<=80)
        cases += 1
    print(f'PASS: archive, metadata, exact binary diff, preserved resources; {cases} isolated x64 cases')
    print('Gameplay and Geode integration: NOT TESTED')


if __name__ == '__main__':
    verify(*sys.argv[1:])
