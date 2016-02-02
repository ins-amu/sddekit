# copyright 2016 Apache 2 sddekit authors

"""
This scripts runs ctypesgen against sddekit.h

"""

import os
import os.path as op
import glob
import sys
import subprocess
import logging

logging.basicConfig(level=logging.DEBUG)
log = logging.getLogger(__file__)

# python ctypesgen/ctypesgen.py -L ../ -lSDDEKit ../src/sddekit.h -o _sddekit.py
here = op.dirname(op.abspath(__file__))
if not op.exists(op.join(here, 'ctypesgen')):
		log.error('ctypesgen submodule not found')
cmd = [sys.executable, 'ctypesgen/ctypesgen.py']

up = op.dirname(here)
lib = glob.glob(op.join(up, 'libSDDEKit.*'))
if len(lib) == 0:
		log.warning('shared library not found, not used for gen')
else:
		cmd += ['-L', up, '-lSDDEKit']

cmd += [op.join(up, 'src', 'sddekit.h'), '-o', op.join(here, '_sddekit.py')]

log.info (cmd)

proc = subprocess.Popen(cmd, cwd=here)
proc.wait()

log.info('done!')

