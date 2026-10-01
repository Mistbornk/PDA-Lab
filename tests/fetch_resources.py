#!/usr/bin/env python3
"""Restore pinned course resources and check their hashes (Python stdlib only)."""
import argparse
import hashlib
import json
from pathlib import Path
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]


def verify(data, entry):
    if len(data) != entry['bytes']:
        raise ValueError(f"Size mismatch: {entry['path']}")
    if 'sha256' in entry and hashlib.sha256(data).hexdigest() != entry['sha256']:
        raise ValueError(f"SHA256 mismatch: {entry['path']}")
    if 'git_blob_sha1' in entry:
        digest = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
        if digest != entry['git_blob_sha1']:
            raise ValueError(f"Git blob hash mismatch: {entry['path']}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Verify files without downloading')
    args = parser.parse_args()
    entries = json.loads((ROOT / 'tests/resources.json').read_text())['resources']
    for entry in entries:
        path = ROOT / entry['path']
        if path.exists():
            verify(path.read_bytes(), entry)
        elif args.check:
            raise FileNotFoundError(path)
        else:
            if 'url' in entry:
                print('Download', entry['path'], flush=True)
                with urllib.request.urlopen(entry['url'], timeout=120) as response:
                    data = response.read()
            else:
                data = (ROOT / entry['source']).read_bytes()
            verify(data, entry)
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)

    # Extract regular files only; do not follow links or allow archive path traversal.
    archive = ROOT / 'Lab04/tests/tools/upstream/Evaluator.tar'
    destination = ROOT / 'Lab04/tests/tools/upstream'
    with tarfile.open(archive) as tar:
        for member in tar.getmembers():
            if member.isdir():
                continue
            path = destination / member.name
            if not member.isfile() or not path.resolve().is_relative_to(destination.resolve()):
                raise ValueError(f'Unsafe archive member: {member.name}')
            with tar.extractfile(member) as source:
                data = source.read()
            if args.check or path.exists():
                if path.read_bytes() != data:
                    raise ValueError(f'Extracted file mismatch: {path}')
            else:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
            if not args.check:
                path.chmod(member.mode & 0o777)
    for name in ['Lab02/tests/tools/verifier', 'Lab03/tests/tools/upstream/Evaluator',
                 'Lab04/tests/tools/upstream/Evaluator/Evaluator']:
        path = ROOT / name
        if path.exists() and not args.check:
            path.chmod(0o755)
    print(f'Verified {len(entries)} resources and Lab04 archive contents.')


if __name__ == '__main__':
    main()
