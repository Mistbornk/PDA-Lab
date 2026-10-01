#!/usr/bin/env python3
"""Rebuild a tracked CMake revision in an isolated ignored directory."""
import argparse
import io
import json
from pathlib import Path
import subprocess
import tarfile
from run import ROOT, build_metadata


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('revision')
    parser.add_argument('--jobs', type=int, default=4)
    args = parser.parse_args()
    commit = subprocess.check_output(['git', 'rev-parse', '--verify', args.revision+'^{commit}'], cwd=ROOT, text=True).strip()
    work = ROOT/'benchmarks/work'/('revision-'+commit[:12])
    source = work/'source'; source.mkdir(parents=True, exist_ok=True)
    # git archive contains only tracked repository files; this is our local revision.
    archive = subprocess.check_output(['git', 'archive', commit], cwd=ROOT)
    with tarfile.open(fileobj=io.BytesIO(archive)) as data:
        data.extractall(source)
    build = work/'build'
    subprocess.run(['cmake', '-S', str(source), '-B', str(build), '-DCMAKE_BUILD_TYPE=Release', '-DBUILD_TESTING=OFF'], check=True)
    subprocess.run(['cmake', '--build', str(build), '--parallel', str(args.jobs)], check=True)
    metadata = build_metadata(build); metadata['source_commit'] = commit
    (build/'build-info.json').write_text(json.dumps(metadata, indent=2)+'\n')
    print(build)


if __name__ == '__main__': main()
