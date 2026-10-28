#!/usr/bin/env python3
"""Build all arcade MAME drivers in a clean, separate build tree."""
from pathlib import Path
import runpy,sys
sys.argv=[sys.argv[0],'--arcade-all',*sys.argv[1:]]
runpy.run_path(str(Path(__file__).with_name('build_mame_subset.py')),run_name='__main__')
