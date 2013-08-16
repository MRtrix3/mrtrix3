def loadOptions(num_args):
  import sys
  verbose = False
  quiet = ' -quiet'
  for option in sys.argv[num_args+1:]:
    if '-verbose'.startswith(option):
      verbose = True
      quiet = ''
    else:
      sys.stderr.write('Unknown option: ' + option + '\n')
      exit(1)
  return (quiet,verbose)

