def initParser(subparsers, base_parser):
  import argparse
  parser_manual = subparsers.add_parser('manual', parents=[base_parser], help='Derive a response function using an input mask image alone (i.e. pre-selected voxels)')
  parser_manual_arguments = parser_manual.add_argument_group('Positional arguments specific to the \'manual\' algorithm')
  parser_manual_arguments.add_argument('in_voxels', help='Input voxel selection mask')
  parser_manual_arguments.add_argument('output', help='Output response function text file')
  parser_manual_options = parser_manual.add_argument_group('Options specific to the \'manual\' algorithm')
  parser_manual_options.add_argument('-dirs', help='Manually provide the fibre direction in each voxel (a tensor fit will be used otherwise)')
  parser_manual.set_defaults(algorithm='manual')
  parser_manual.set_defaults(single_shell=False)
  
  
  
def checkOutputFiles():
  import lib.app
  lib.app.checkOutputFile(lib.app.args.output)



def getInputFiles():
  import os
  import lib.app
  from lib.runCommand  import runCommand
  from lib.warnMessage import warnMessage
  mask_path = os.path.join(lib.app.tempDir, 'mask.mif')
  if os.path.exists(mask_path):
    warnMessage('-mask option is ignored by algorithm \'manual\'')
    os.remove(mask_path)
  runCommand('mrconvert ' + lib.app.args.in_voxels + ' ' + os.path.join(lib.app.tempDir, 'in_voxels.mif'))
  if lib.app.args.dirs:
    runCommand('mrconvert ' + lib.app.args.dirs + ' ' + os.path.join(lib.app.tempDir, 'dirs.mif') + ' -stride 0,0,0,1')



def execute():
  import os, shutil
  import lib.app
  from lib.errorMessage  import errorMessage
  from lib.getHeaderInfo import getHeaderInfo
  from lib.runCommand    import runCommand
  
  lmax_option = ''
  if lib.app.args.lmax:
    lmax_option = ' -lmax ' + lib.app.args.lmax
  
  shells = [ int(x) for x in getHeaderInfo('dwi.mif', 'shells').split() ]

  # Do we have directions, or do we need to calculate them?
  if not os.path.exists('dirs.mif'):
    runCommand('dwi2tensor dwi.mif - -mask in_voxels.mif | tensor2metric - -vector dirs.mif')

  # Loop over shells
  response = [ ]
  max_length = 0
  for index, b in enumerate(shells):
    runCommand('dwiextract dwi.mif - -shell ' + str(b) + ' | amp2sh - - | sh2response - in_voxels.mif dirs.mif response_b' + str(b) + '.txt' + lmax_option)
    shell_response = open('response_b' + str(b) + '.txt', 'r').read().split()
    response.append(shell_response)
    max_length = max(max_length, len(shell_response))

  with open('response.txt', 'w') as f:
    for line in response:
      line += ['0'] * (max_length - len(line))
      f.write(' '.join(line) + '\n')

  shutil.copyfile('response.txt', os.path.join(lib.app.workingDir, lib.app.args.output))
  shutil.copyfile('in_voxels.mif', 'voxels.mif')

