def initParser(subparsers, base_parser):
  import argparse
  parser = subparsers.add_parser('manual', parents=[base_parser], description='Derive a response function using an input mask image alone (i.e. pre-selected voxels)')
  parser.add_argument('input', help='The input DWI')
  parser.add_argument('in_voxels', help='Input voxel selection mask')
  parser.add_argument('output', help='Output response function text file')
  options = parser.add_argument_group('Options specific to the \'manual\' algorithm')
  options.add_argument('-dirs', help='Manually provide the fibre direction in each voxel (a tensor fit will be used otherwise)')
  parser.set_defaults(algorithm='manual')



def checkOutputFiles():
  from mrtrix3 import app
  app.checkOutputFile(app.args.output)



def getInputFiles():
  import os
  from mrtrix3 import app, path, run
  mask_path = path.toTemp('mask.mif', False)
  if os.path.exists(mask_path):
    app.warn('-mask option is ignored by algorithm \'manual\'')
    os.remove(mask_path)
  run.command('mrconvert ' + path.fromUser(app.args.in_voxels, True) + ' ' + path.toTemp('in_voxels.mif', True))
  if hasattr(app.args, 'dirs') and app.args.dirs:
    run.command('mrconvert ' + path.fromUser(app.args.dirs, True) + ' ' + path.toTemp('dirs.mif', True) + ' -stride 0,0,0,1')



def execute():
  import os, shutil
  from mrtrix3 import app, image, path, run

  shells = [ int(round(float(x))) for x in image.headerField('dwi.mif', 'shells').split() ]

  # Get lmax information (if provided)
  lmax = [ ]
  if hasattr(app.args, 'lmax') and app.args.lmax:
    lmax = [ int(x.strip()) for x in app.args.lmax.split(',') ]
    if not len(lmax) == len(shells):
      app.error('Number of manually-defined lmax\'s (' + str(len(lmax)) + ') does not match number of b-value shells (' + str(len(shells)) + ')')
    for l in lmax:
      if l%2:
        app.error('Values for lmax must be even')
      if l<0:
        app.error('Values for lmax must be non-negative')

  # Do we have directions, or do we need to calculate them?
  if not os.path.exists('dirs.mif'):
    run.command('dwi2tensor dwi.mif - -mask in_voxels.mif | tensor2metric - -vector dirs.mif')

  # Get response function
  bvalues_option = ' -shell ' + ','.join(map(str,shells))
  lmax_option = ''
  if lmax:
    lmax_option = ' -lmax ' + ','.join(map(str,lmax))
  run.command('amp2response dwi.mif in_voxels.mif dirs.mif response.txt' + bvalues_option + lmax_option)

  run.function(shutil.copyfile, 'response.txt', path.fromUser(app.args.output, False))
  run.function(shutil.copyfile, 'in_voxels.mif', 'voxels.mif')

