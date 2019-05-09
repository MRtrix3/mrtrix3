import imp, inspect, os, sys
from distutils.spawn import find_executable

def imported(lib_path):
  success = False
  fp = None
  try:
    fp, pathname, description = imp.find_module('mrtrix3', [ lib_path ])
    imp.load_module('mrtrix3', fp, pathname, description)
    success = True
  except ImportError:
    pass
  finally:
    if fp:
      fp.close()
  return success

# Approach 1:
# Can the MRtrix3 Python modules be found based on their relative location to this file?
# Note that this includes the case where this file is a softlink within an external module,
#   which provides a direct link to the core installation
if not imported(os.path.normpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), \
                                              os.pardir, \
                                              'lib'))):

  # Approach 2:
  # If this file is a duplicate, which has been stored in an external module, we may be able to use
  #   a softlink to the build script if such has been created
  if not imported(os.path.join(os.path.dirname(os.path.realpath(os.path.join(os.path.dirname(__file__), \
                                                                             os.pardir, \
                                                                             'build'))), \
                               'lib'))):

    sys.stderr.write('Unable to locate MRtrix3 Python modules\n')
    sys.stderr.write('https://mrtrix.readthedocs.io/en/latest/tips_and_tricks/external_modules.html\n')
    sys.stderr.flush()
    sys.exit(1)

# See if the origin importing this file has the requisite member functions to be executed using the API
MODULE = inspect.getmodule(inspect.stack()[-1][0])
try:
  getattr(MODULE, 'usage')
  getattr(MODULE, 'execute')
  from mrtrix3 import app
  app.execute(MODULE)
except AttributeError:
  pass
