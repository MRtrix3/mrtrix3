def initParser(subparsers, base_parser):
  import argparse
  parser = subparsers.add_parser('manual', parents=[base_parser], add_help=False, description='Derive a response function using an input mask image alone (i.e. pre-selected voxels)')
  parser.add_argument('input', help='The input DWI')
  parser.add_argument('in_voxels', help='Input voxel selection mask')
  parser.add_argument('output', help='Output response function text file')
  options = parser.add_argument_group('Options specific to the \'manual\' algorithm')
  options.add_argument('-dirs', help='Manually provide the fibre direction in each voxel (a tensor fit will be used otherwise)')
  parser.set_defaults(algorithm='manual')
  
  
  
def checkOutputFiles():
  import lib.app
  lib.app.checkOutputFile(lib.app.args.output)



def getInputFiles():
  import os
  import lib.app
  from lib.getUserPath import getUserPath
  from lib.runCommand  import runCommand
  from lib.warnMessage import warnMessage
  mask_path = os.path.join(lib.app.tempDir, 'mask.mif')
  if os.path.exists(mask_path):
    warnMessage('-mask option is ignored by algorithm \'manual\'')
    os.remove(mask_path)
  runCommand('mrconvert ' + getUserPath(lib.app.args.in_voxels, True) + ' ' + os.path.join(lib.app.tempDir, 'in_voxels.mif'))
  if lib.app.args.dirs:
    runCommand('mrconvert ' + getUserPath(lib.app.args.dirs, True) + ' ' + os.path.join(lib.app.tempDir, 'dirs.mif') + ' -stride 0,0,0,1')



def execute():
  import os, shutil
  import lib.app
  from lib.errorMessage  import errorMessage
  from lib.getHeaderInfo import getHeaderInfo
  from lib.getUserPath   import getUserPath
  from lib.runCommand    import runCommand
  from lib.runFunction   import runFunction
  
  shells = [ int(round(float(x))) for x in getHeaderInfo('dwi.mif', 'shells').split() ]
  
  # Get lmax information (if provided)
  lmax = [ ]
  if lib.app.args.lmax:
    lmax = [ int(x.strip()) for x in lib.app.args.lmax.split(',') ]
    if not len(lmax) == len(shells):
      errorMessage('Number of manually-defined lmax\'s (' + str(len(lmax)) + ') does not match number of b-value shells (' + str(len(shells)) + ')')
    for l in lmax:
      if l%2:
        errorMessage('Values for lmax must be even')
      if l<0:
        errorMessage('Values for lmax must be non-negative')
    
  # Do we have directions, or do we need to calculate them?
  if not os.path.exists('dirs.mif'):
    runCommand('dwi2tensor dwi.mif - -mask in_voxels.mif | tensor2metric - -vector dirs.mif')

  # Get response function
  bvalues_option = ' -shell ' + ','.join(map(str,shells))
  lmax_option = ''
  if lmax:
    lmax_option = ' -lmax ' + ','.join(map(str,lmax))
  runCommand('amp2response dwi.mif in_voxels.mif dirs.mif response.txt' + bvalues_option + lmax_option)

  runFunction(shutil.copyfile, 'response.txt', getUserPath(lib.app.args.output, False))
  runFunction(shutil.copyfile, 'in_voxels.mif', 'voxels.mif')

