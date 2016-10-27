def getAlgorithmList():
  import os, sys
  # Build an initial list of possible algorithms, found in the relevant scripts/src/ directory
  algorithm_list = []
  execpath=sys.argv[0]
  while os.path.islink (execpath): execpath=os.readlink (execpath)
  path = os.path.basename(execpath)
  if not path[0].isalpha():
    path = '_' + path
  src_file_list = os.listdir(os.path.join(os.path.dirname(os.path.abspath(execpath)), 'src', path))
  for filename in src_file_list:
    filename = filename.split('.')
    if len(filename) == 2 and filename[1] == 'py' and not filename[0] == '__init__':
      algorithm_list.append(filename[0])
  return sorted(algorithm_list)

