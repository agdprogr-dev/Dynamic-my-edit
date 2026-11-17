"""Reproducible, hash-locked InputAlign 80-tick experimental binary patch."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile

PACKAGE_SHA = '278fe2013f597fa2b8faed6d73d178b3fcf5048d8e561a5d24a2b0b390ae07de'
DLL_SHA = '29fc5227414bb5f3aa65e27b6cec8dda8092f380663638937495c7cfe98c68d4'
# Virtual addresses established from objdump of this exact DLL. Same-size edits only.
PATCHES = [
    (0x49783, '48c744242028000000', '48c744242050000000', 'Normal menu upper bound'),
    (0x498f3, '48c744242028000000', '48c744242050000000', 'Wave menu upper bound'),
    (0x49963, '48c74424203c000000', '48c744242050000000', 'Spam interval menu upper bound'),
    (0x4ba42, '83fd27', '83fd4f', 'Delay recommendation saturation boundary'),
    (0x4ba45, 'be28000000', 'be50000000', 'Delay recommendation upper bound'),
    (0x4ba4f, '83fe24', '83fe4c', 'Limit-hit recommendation boundary'),
    (0x4ba52, 'b824000000', 'b84c000000', 'Limit-hit recommendation plus-four ceiling'),
    (0x4bbcc, '83fd28', '83fd50', 'Macro lead recommendation comparison'),
    (0x4bbcf, 'b828000000', 'b850000000', 'Macro lead recommendation upper bound'),
    (0x2c011, '4183f929', '4183f951', 'Reference macro matching distance (exclusive)'),
    (0x2c036, '83fa28', '83fa50', 'Reference macro matching distance (inclusive)'),
]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def patch(source, destination):
    if source.resolve() == destination.resolve() or (destination.exists() and source.samefile(destination)):
        raise ValueError('Use a separate destination; preserve the original package')
    if sha(source.read_bytes()) != PACKAGE_SHA:
        raise ValueError('Unsupported package SHA-256')
    with zipfile.ZipFile(source) as original:
        if original.testzip() is not None:
            raise ValueError('Corrupt source archive')
        dll = bytearray(original.read('ai.predictivebuffer.dll'))
        if sha(dll) != DLL_SHA:
            raise ValueError('Unsupported DLL SHA-256')
        edits = []
        for rva, before_hex, after_hex, reason in PATCHES:
            # .text: RVA 0x1000, raw file offset 0x400.
            offset = rva - 0xc00
            before, after = bytes.fromhex(before_hex), bytes.fromhex(after_hex)
            if len(before) != len(after) or dll[offset:offset + len(before)] != before:
                raise ValueError(f'Unexpected instruction at RVA {rva:#x}')
            dll[offset:offset + len(after)] = after
            edits.append(dict(rva=hex(rva), fileOffset=hex(offset), before=before_hex,
                              after=after_hex, reason=reason))
        mod = json.loads(original.read('mod.json'))
        mod['name'] = 'InputAlign (experimental 80 ticks)'
        for key in ('normal-ticks', 'wave-ticks', 'spam-interval'):
            mod['settings'][key]['max'] = 80
        for key in ('normal-ticks', 'wave-ticks'):
            mod['settings'][key]['default'] = 80
        mod['settings']['spam-guard']['default'] = False
        mod['settings']['macro-compare']['description'] = mod['settings']['macro-compare']['description'].replace('40 ticks', '80 ticks')
        # Preserve string lengths and all PE addresses when refreshing embedded help.
        old = b'up to 40 ticks'
        new = b'up to 80 ticks'
        if dll.count(old) != 1 or dll.count(b'hasta 40 ticks') != 1:
            raise ValueError('Unexpected embedded macro help')
        dll[:] = dll.replace(old, new).replace(b'hasta 40 ticks', b'hasta 80 ticks')
        previous_provenance = json.loads(original.read('binary-patch-provenance.json'))
        provenance = dict(generatedBy='CodeRabbit Coding Agent', kind='experimental-binary-tick-patch',
                          originalDeveloper=mod['developer'], inputPackageSha256=PACKAGE_SHA,
                          inputDllSha256=DLL_SHA, patchedDllSha256=sha(dll), gameplayTested=False,
                          scope='80-tick Normal/Wave and spam interval limits; recommendation and macro matching bounds; Normal/Wave defaults 80; spam guard default off. Existing saved settings persist.',
                          patches=edits, embeddedHelpReplacements=['up to 40 ticks -> up to 80 ticks', 'hasta 40 ticks -> hasta 80 ticks'],
                          previousPatch=previous_provenance)
        replacements = {'mod.json': json.dumps(mod, ensure_ascii=False, indent=2).encode(),
                        'ai.predictivebuffer.dll': bytes(dll),
                        'binary-patch-provenance.json': json.dumps(provenance, indent=2).encode()}
        destination.parent.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(destination, 'w') as output:
            for entry in original.infolist():
                output.writestr(entry, replacements.get(entry.filename, original.read(entry.filename)))
    print(json.dumps(dict(output=str(destination), sha256=sha(destination.read_bytes()),
                          dllSha256=sha(dll), instructionPatches=len(edits)), indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    patch(args.source, args.destination)
