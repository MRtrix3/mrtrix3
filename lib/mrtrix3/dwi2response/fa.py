def initParser(subparsers, base_parser):
  import argparse
  from mrtrix3 import app
  app.addCitation('If using \'fa\' algorithm', 'Tournier, J.-D.; Calamante, F.; Gadian, D. G. & Connelly, A. Direct estimation of the fiber orientation density function from diffusion-weighted MRI data using spherical deconvolution. NeuroImage, 2004, 23, 1176-1185', False)
  parser = subparsers.add_parser('fa', parents=[base_parser], description='Use the old FA-threshold heuristic for single-fibre voxel selection and response function estimation')
  parser.add_argument('input', help='The input DWI')
  parser.add_argument('output', help='The output response function text file')
  options = parser.add_argument_group('Options specific to the \'fa\' algorithm')
  options.add_argument('-erode', type=int, default=3, help='Number of brain mask erosion steps to apply prior to threshold (not used if mask is provided manually)')
  options.add_argument('-number', type=int, default=300, help='The number of highest-FA voxels to use')
  options.add_argument('-threshold', type=float, help='Apply a hard FA threshold, rather than selecting the top voxels')
  parser.flagMutuallyExclusiveOptions( [ 'number', 'threshold' ] )
  parser.set_defaults(algorithm='fa')
  parser.set_defaults(single_shell=True)
  parser.set_defaults(needs_bzero=True)
  
  
  
def checkOutputFiles():
  from mrtrix3 import app
  app.checkOutputFile(app.args.output)



def getInputFiles():
  pass



def execute():
  import os, shutil
  from mrtrix3 import app, path, run
  lmax_option = ''
  if app.args.lmax:
    lmax_option = ' -lmax ' + app.args.lmax
  if not app.args.mask:
    run.command('maskfilter mask.mif erode mask_eroded.mif -npass ' + str(app.args.erode))
    mask_path = 'mask_eroded.mif'
  else:
    mask_path = 'mask.mif'
  run.command('mrcat bzero.mif dwi.mif - -axis 3 | dwi2tensor - -mask ' + mask_path + ' tensor.mif')
  run.command('tensor2metric tensor.mif -fa fa.mif -vector vector.mif -mask ' + mask_path)
  if app.args.threshold:
    run.command('mrthreshold fa.mif voxels.mif -abs ' + str(app.args.threshold))
  else:
    run.command('mrthreshold fa.mif voxels.mif -top ' + str(app.args.number))
  run.command('amp2response dwi.mif voxels.mif vector.mif response.txt' + lmax_option)

  run.function(shutil.copyfile, 'response.txt', path.fromUser(app.args.output, False))

