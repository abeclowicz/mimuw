import runpy
import sys
from pathlib import Path

SRC_DIR = Path(__file__).parent / "src/"

sys.path.insert(0, str(SRC_DIR))

if __name__ == "__main__":
    runpy.run_path(str(SRC_DIR / "integration_test.py"), run_name="__main__")
