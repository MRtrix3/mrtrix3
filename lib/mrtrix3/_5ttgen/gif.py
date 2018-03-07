def initialise(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('gif', author='Matteo Mancini (m.mancini@ucl.ac.uk)', synopsis='Generate the 5TT image based on a Geodesic Information Flow (GIF) segmentation image', parents=[base_parser])
  parser.add_argument('input',  help='The input Geodesic Information Flow (GIF) segmentation image')
  parser.add_argument('output', help='The output 5TT image')


def checkOutputPaths(): #pylint: disable=unused-variable
  pass


def checkGIFinput(image_path):
  from mrtrix3 import app, image
  dim = [ int(i) for i in image.mrinfo(image_path, 'size').strip().split() ]
  if len(dim) < 4:
    app.error('Image \'' + image_path + '\' does not look like GIF segmentation (less than 4 spatial dimensions)')
  if min(dim[:4]) == 1:
    app.error('Image \'' + image_path + '\' does not look like GIF segmentation (axis with size 1)')


def getInputs(): #pylint: disable=unused-variable
  import os, shutil #pylint: disable=unused-variable
  from mrtrix3 import app, path, run
  checkGIFinput(path.fromUser(app.args.input, True))
  run.command('mrconvert ' + path.fromUser(app.args.input, True) + ' ' + path.toTemp('input.mif', True))


def execute(): #pylint: disable=unused-variable
  import os, sys #pylint: disable=unused-variable
  from mrtrix3 import app, path, run #pylint: disable=unused-variable

  # Generate the images related to each tissue
  run.command('mrconvert input.mif -coord 3 1 CSF.mif')
  run.command('mrconvert input.mif -coord 3 2 cGM.mif')
  run.command('mrconvert input.mif -coord 3 3 cWM.mif')
  run.command('mrconvert input.mif -coord 3 4 sGM.mif')

  # Combine WM and subcortical WM into a unique WM image
  run.command('mrconvert input.mif - -coord 3 3,5 | mrmath - sum WM.mif -axis 3')

  # Create an empty lesion image
  run.command('mrcalc WM.mif 0 -mul lsn.mif')

  # Convert into the 5tt format
  run.command('mrcat cGM.mif sGM.mif WM.mif CSF.mif lsn.mif 5tt.mif -axis 3')

  if app.args.nocrop:
    run.command('mrconvert 5tt.mif result.mif')
  else:
    run.command('mrmath 5tt.mif sum - -axis 3 | mrthreshold - - -abs 0.5 | mrcrop 5tt.mif result.mif -mask -')
