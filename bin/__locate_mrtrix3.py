import inspect, os, sys
from distutils.spawn import find_executable
try:
  # Import module that is already present in PYTHONPATH
  import mrtrix3
except ImportError:
  # Look for the libraries based on the relative path to:
  # - this file
  # - location of mrconvert binary
  for filepath in [ inspect.getfile(inspect.currentframe()),
                    find_executable('mrconvert') ]:
    if filepath:
      lib_folder = os.path.realpath(os.path.join(os.path.dirname(os.path.realpath(filepath)), os.pardir, 'lib'))
      if os.path.isdir(lib_folder):
        try:
          sys.path.insert(0, lib_folder)
          import mrtrix3
          break
        except ImportError:
          sys.path = sys.path[1:]
  if 'mrtrix3' not in sys.modules:
    sys.stderr.write('Unable to locate MRtrix3 Python libraries\n')
    sys.stderr.write('Add MRtrix3 lib/ directory to PYTHONPATH environment variable\n')
    sys.exit(1)
