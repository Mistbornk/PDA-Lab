#!/usr/bin/env python3
"""Run Clang Static Analyzer on project translation units from a compile database."""
import argparse
import json
from pathlib import Path
import plistlib
import shlex
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, default=Path('build'))
    parser.add_argument('--compiler', default='clang++-14')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    rows = []
    with tempfile.TemporaryDirectory(prefix='pda-analysis-') as directory:
        for entry in json.loads((args.build / 'compile_commands.json').read_text()):
            source = Path(entry['file'])
            if source.parent.name == 'tests' or 'tests' in source.parts:
                continue
            tokens = shlex.split(entry['command'])[1:]
            flags = []
            skip = False
            for token in tokens:
                if skip:
                    skip = False
                elif token == '-o':
                    skip = True
                elif token != '-c' and not token.startswith('-fsanitize'):
                    flags.append(token)
            output = Path(directory) / f'{len(rows)}.plist'
            command = [args.compiler, *flags, '--analyze', '-Xanalyzer',
                       '-analyzer-output=plist', '-o', str(output)]
            run = subprocess.run(command, cwd=entry['directory'], capture_output=True,
                                 text=True, timeout=120)
            diagnostics = plistlib.loads(output.read_bytes()).get('diagnostics', []) if output.exists() else []
            rows.append(dict(source=str(source), returncode=run.returncode,
                             diagnostics=diagnostics, output=run.stdout+run.stderr))
    report = dict(compiler=subprocess.check_output([args.compiler, '--version'], text=True).splitlines()[0],
                  results=rows, complete=True)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(f'Analyzed {len(rows)} files; {sum(len(r["diagnostics"]) for r in rows)} diagnostics')
    raise SystemExit(0 if rows and all(r['returncode'] == 0 and not r['diagnostics'] for r in rows) else 1)


if __name__ == '__main__':
    main()
