"""Experimental 400-tick UI/setting limits; macro analysis remains capped at 80."""
import argparse
import contextlib
import io
import json
import os
from pathlib import Path
import tempfile
import zipfile
from patch_inputalign_80 import patch as patch80, sha

BASE80_SHA = 'cee33be4b42ad683b672ec4bb99b3123f063afb14382242891326619ed56634a'
UI_RVAS = (0x49783, 0x498f3, 0x49963)


def patch(source, destination):
    source, destination = Path(source), Path(destination)
    if source.resolve() == destination.resolve() or (destination.exists() and source.samefile(destination)):
        raise ValueError('Use a separate destination; preserve the original package')
    with tempfile.TemporaryDirectory() as scratch:
        base = Path(scratch) / '80.geode'
        with contextlib.redirect_stdout(io.StringIO()):
            patch80(source, base)
        if sha(base.read_bytes()) != BASE80_SHA:
            raise ValueError('Unexpected intermediate 80-tick package')
        with zipfile.ZipFile(base) as archive:
            dll = bytearray(archive.read('ai.predictivebuffer.dll'))
            edits = []
            for rva in UI_RVAS:
                offset = rva - 0xc00
                before = bytes.fromhex('48c744242050000000')
                after = bytes.fromhex('48c744242090010000')
                if dll[offset:offset+len(before)] != before:
                    raise ValueError(f'Unexpected UI instruction at {rva:#x}')
                dll[offset:offset+len(after)] = after
                edits.append(dict(rva=hex(rva), before=before.hex(), after=after.hex()))
            meta = json.loads(archive.read('mod.json'))
            meta['name'] = 'InputAlign (experimental 400 tick windows)'
            for key in ('normal-ticks', 'wave-ticks', 'spam-interval'):
                meta['settings'][key]['max'] = 400
            for key in ('normal-ticks', 'wave-ticks'):
                meta['settings'][key]['default'] = 400
            provenance = dict(generatedBy='CodeRabbit Coding Agent',
                              originalDeveloper=meta['developer'], gameplayTested=False,
                              kind='experimental-400-tick-ui-window-patch',
                              base80PackageSha256=BASE80_SHA, patchedDllSha256=sha(dll),
                              scope='Normal/Wave and spam interval UI/setting maxima 400; Normal/Wave defaults 400. Recommendation and macro matching limits remain 80. Simulation budget unchanged; real gameplay not verified.',
                              patches=edits,
                              previousPatch=json.loads(archive.read('binary-patch-provenance.json')))
            replacements = {'mod.json': json.dumps(meta, ensure_ascii=False, indent=2).encode(),
                            'ai.predictivebuffer.dll': bytes(dll),
                            'binary-patch-provenance.json': json.dumps(provenance, indent=2).encode()}
            destination.parent.mkdir(parents=True, exist_ok=True)
            # Build completely before replacing an existing destination.
            with tempfile.TemporaryDirectory(dir=destination.parent) as staging:
                output_path = Path(staging) / 'package.geode'
                with zipfile.ZipFile(output_path, 'w') as output:
                    for entry in archive.infolist():
                        output.writestr(entry, replacements.get(entry.filename, archive.read(entry.filename)))
                os.replace(output_path, destination)
    print(json.dumps(dict(output=str(destination), sha256=sha(destination.read_bytes()),
                          maxTicks=400, macroAnalysisMaxTicks=80, gameplayTested=False), indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    patch(args.source, args.destination)
