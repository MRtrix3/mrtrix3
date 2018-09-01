def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('gif', parents=[base_parser])
  parser.setAuthor('Matteo Mancini (m.mancini@ucl.ac.uk)')
  parser.setSynopsis('Generate the 5TT image based on a Geodesic Information Flow (GIF) segmentation image')
  parser.add_argument('input',  help='The input Geodesic Information Flow (GIF) segmentation image')
  parser.add_argument('output', help='The output 5TT image')


def checkOutputPaths(): #pylint: disable=unused-variable
  from mrtrix3 import app
  app.checkOutputPath(app.args.output)


def checkGIFinput(image_path):
  from mrtrix3 import image, MRtrixException
  dim = image.Header(image_path).size()
  if len(dim) < 4:
    raise MRtrixException('Image \'' + image_path + '\' does not look like GIF segmentation (less than 4 spatial dimensions)')
  if min(dim[:4]) == 1:
    raise MRtrixException('Image \'' + image_path + '\' does not look like GIF segmentation (axis with size 1)')


def getInputs(): #pylint: disable=unused-variable
  from mrtrix3 import app, path, run
  checkGIFinput(path.fromUser(app.args.input, True))
  run.command('mrconvert ' + path.fromUser(app.args.input, True) + ' ' + path.toTemp('input.mif', True))


def execute(): #pylint: disable=unused-variable
  import os
  from mrtrix3 import app, path, run

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
    run.function(os.rename, '5tt.mif', 'result.mif')
  else:
    run.command('mrmath 5tt.mif sum - -axis 3 | mrthreshold - - -abs 0.5 | mrcrop 5tt.mif result.mif -mask -')

  run.command('mrconvert result.mif ' + path.fromUser(app.args.output, True) + app.mrconvertOutputOption(path.fromUser(app.args.input, True)))
