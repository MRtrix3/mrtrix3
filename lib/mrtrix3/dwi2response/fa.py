#pylint: disable=unused-variable
def initialise(base_parser, subparsers):
  parser = subparsers.add_parser('fa', author='Robert E. Smith (robert.smith@florey.edu.au)', synopsis='Use the old FA-threshold heuristic for single-fibre voxel selection and response function estimation', parents=[base_parser])
  parser.addCitation('', 'Tournier, J.-D.; Calamante, F.; Gadian, D. G. & Connelly, A. Direct estimation of the fiber orientation density function from diffusion-weighted MRI data using spherical deconvolution. NeuroImage, 2004, 23, 1176-1185', False)
  parser.add_argument('input', help='The input DWI')
  parser.add_argument('output', help='The output response function text file')
  options = parser.add_argument_group('Options specific to the \'fa\' algorithm')
  options.add_argument('-erode', type=int, default=3, help='Number of brain mask erosion steps to apply prior to threshold (not used if mask is provided manually)')
  options.add_argument('-number', type=int, default=300, help='The number of highest-FA voxels to use')
  options.add_argument('-threshold', type=float, help='Apply a hard FA threshold, rather than selecting the top voxels')
  parser.flagMutuallyExclusiveOptions( [ 'number', 'threshold' ] )



#pylint: disable=unused-variable
def checkOutputPaths():
  from mrtrix3 import app
  app.checkOutputPath(app.args.output)



#pylint: disable=unused-variable
def getInputs():
  pass



#pylint: disable=unused-variable
def needsSingleShell():
  return False



#pylint: disable=unused-variable
def execute():
  import shutil
  from mrtrix3 import app, image, path, run
  bvalues = [ int(round(float(x))) for x in image.mrinfo('dwi.mif', 'shells').split() ]
  if len(bvalues) < 2:
    app.error('Need at least 2 unique b-values (including b=0).')
  lmax_option = ''
  if app.args.lmax:
    lmax_option = ' -lmax ' + app.args.lmax
  if not app.args.mask:
    run.command('maskfilter mask.mif erode mask_eroded.mif -npass ' + str(app.args.erode))
    mask_path = 'mask_eroded.mif'
  else:
    mask_path = 'mask.mif'
  run.command('dwi2tensor dwi.mif -mask ' + mask_path + ' tensor.mif')
  run.command('tensor2metric tensor.mif -fa fa.mif -vector vector.mif -mask ' + mask_path)
  if app.args.threshold:
    run.command('mrthreshold fa.mif voxels.mif -abs ' + str(app.args.threshold))
  else:
    run.command('mrthreshold fa.mif voxels.mif -top ' + str(app.args.number))
  run.command('dwiextract dwi.mif - -singleshell -no_bzero | amp2response - voxels.mif vector.mif response.txt' + lmax_option)

  run.function(shutil.copyfile, 'response.txt', path.fromUser(app.args.output, False))
