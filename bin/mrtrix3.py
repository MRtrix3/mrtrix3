import imp, os, sys
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
# If this file is within the core MRtrix3 installation, it should be easy to navigate to the lib/ directory
if not imported(os.path.normpath(os.path.join(os.path.dirname(__file__), os.pardir, 'lib'))):

  # Approach 2:
  # If this file is itself a softlink, that should point us to the file in the bin/ directory of the relevant MRtrix3 install
  if not (os.path.islink(__file__) and \
          imported(os.path.normpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, 'lib')))):

    # Approach 3:
    # Find a softlink to build script within an external module
    BUILD_PATH = os.path.normpath(os.path.join(os.path.dirname(__file__), os.pardir, 'build'))
    if not (os.path.islink(BUILD_PATH) and \
            imported(os.path.join(os.path.dirname(os.path.realpath(BUILD_PATH)), 'lib'))):

      # Approach 4:
      # See whether PYTHONPATH has been explicitly set, in which case it should be possible to
      #   find the directory within sys.path
      from_pythonpath = False
      for candidate in sys.path:
        if os.path.normpath(candidate) != os.path.dirname(__file__) and imported(candidate):
          from_pythonpath = True
          break
      if not from_pythonpath:

        # Approach 5:
        # Search for mrconvert in PATH; if it can be found, use that to find the libraries
        MRCONVERT_PATH = find_executable('mrconvert')
        if not MRCONVERT_PATH or not imported(os.path.normpath(os.path.join(os.path.dirname(MRCONVERT_PATH), os.pardir, 'lib'))):
          sys.stderr.write('Unable to locate MRtrix3 Python modules\n')
          sys.stderr.write('https://mrtrix.readthedocs.io/en/latest/tips_and_tricks/external_modules.html\n')
          sys.stderr.flush()
          sys.exit(1)
