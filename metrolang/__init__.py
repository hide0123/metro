import os
import subprocess
import sys

CMAKE_BIN_DIR = os.path.join(os.path.dirname(__file__), "bin")


def metro():
    raise SystemExit(subprocess.call([os.path.join(CMAKE_BIN_DIR, "metro")] + sys.argv[1:]))
