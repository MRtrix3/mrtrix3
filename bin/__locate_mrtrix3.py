# Make the corresponding MRtrix3 Python libraries available
import inspect, os, sys
LIB_FOLDER = os.path.realpath(os.path.join(os.path.dirname(os.path.realpath(inspect.getfile(inspect.currentframe()))), os.pardir, 'lib'))
if not os.path.isdir(LIB_FOLDER):
  sys.stderr.write('Unable to locate MRtrix3 Python libraries')
  sys.exit(1)
sys.path.insert(0, LIB_FOLDER)

