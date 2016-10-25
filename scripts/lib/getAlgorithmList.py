def getAlgorithmList():
  import os, sys
  from lib.debugMessage import debugMessage
  from lib.errorMessage import errorMessage
  # Build an initial list of possible algorithms, found in the relevant scripts/src/ directory
  algorithm_list = []
  path = os.path.basename(sys.argv[0])
  if not path[0].isalpha():
    path = '_' + path
  src_file_dir = os.path.join(os.path.dirname(os.path.abspath(sys.argv[0])), 'src', path)
  if not os.path.isdir (src_file_dir):
    errorMessage('Unable to query script source directory ' + src_file_dir + ' (have you moved the script?)')
  src_file_list = os.listdir(src_file_dir)
  for filename in src_file_list:
    filename = filename.split('.')
    if len(filename) == 2 and filename[1] == 'py' and not filename[0] == '__init__':
      algorithm_list.append(filename[0])
  algorithm_list = sorted(algorithm_list)
  debugMessage('Found algorithms: ' + str(algorithm_list))
  return algorithm_list

