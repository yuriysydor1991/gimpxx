#!/usr/bin/env python3
import os
import subprocess
import sys
from pathlib import Path

# The GIMP tree root is given by the build, since it may be a subdirectory
# of the top-level project.
os.chdir(Path(sys.argv[1] if len(sys.argv) > 1 else os.getenv("MESON_SOURCE_ROOT",".")))
result = subprocess.run(['git', 'status', '--untracked-files=no', '--short', '--ignore-submodules=dirty', 'gimp-data'], capture_output=True, text=True, check=True)
exit(len(result.stdout.splitlines()))
