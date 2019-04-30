import inspect, os, sys, imp
LIB_FOLDER = os.path.realpath(os.path.join(os.path.dirname(os.path.realpath(inspect.getfile(inspect.currentframe()))), os.pardir, 'lib'))
fp, pathname, description = imp.find_module("mrtrix3", [ LIB_FOLDER ])
try:
  imp.load_module("mrtrix3", fp, pathname, description)
finally:
  if fp:
    fp.close()
