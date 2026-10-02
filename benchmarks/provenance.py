"""Source and environment identity shared by interview experiment reports."""
from datetime import datetime, timezone
import hashlib
from pathlib import Path
import platform
import subprocess


def provenance(root: Path):
    files = [root/'CMakeLists.txt']
    for directory in ['Lab01', 'Lab02', 'Lab03', 'Lab04', 'include']:
        files += [p for p in (root/directory).rglob('*') if p.suffix in {'.cpp', '.hpp', '.h'}
                  and not any(part in {'tests', 'Lab1_Guide', 'Lab2_Guide', 'Lab3_Guide', 'Lab4_Guide'} for part in p.parts)]
    return dict(timestamp=datetime.now(timezone.utc).isoformat(), platform=platform.platform(),
                cpu=next((s.split(':',1)[1].strip() for s in Path('/proc/cpuinfo').read_text().splitlines()
                          if s.startswith('model name')), platform.processor()),
                revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),
                dirty=bool(subprocess.check_output(['git','status','--porcelain'],cwd=root)),
                source_sha256={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)})
