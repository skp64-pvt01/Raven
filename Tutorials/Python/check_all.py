"""
Runs every step's --test mode and reports the result.

    python3 check_all.py

The environment must already reach Raven: either `pip install raven-gui`, or
PYTHONPATH pointing at Raven/bindings/python and RAVEN_LIBRARY at the shared
library (see README.md).
"""

import pathlib
import subprocess
import sys

root = pathlib.Path(__file__).resolve().parent
failed = 0

for script in sorted(root.glob("0*/main.py")):
    print("=== %s" % script.parent.name)
    result = subprocess.run([sys.executable, str(script), "--test"])
    if result.returncode != 0:
        failed += 1

print()
print("%d step(s) failed" % failed if failed else "All steps passed.")
sys.exit(1 if failed else 0)
