import os
from mrtrix3 import MRtrixError
from mrtrix3 import app, image, path, run

def usage(base_parser, subparsers): #pylint: disable=unused-variable
  parser = subparsers.add_parser('deep_atropos', parents=[base_parser])
  parser.set_author('Lucius S. Fekonja (lucius.fekonja[at]charite.de)')
  parser.set_synopsis('Generate the 5TT image based on a Deep Atropos segmentation image')
  parser.add_argument('input',  help='The input Deep Atropos segmentation image')
  parser.add_argument('output', help='The output 5TT image')

def check_output_paths(): #pylint: disable=unused-variable
  app.check_output_path(app.ARGS.output)

def check_deep_atropos_input(image_path):
  dim = image.Header(image_path).size()
  if len(dim) != 3:
    raise MRtrixError('Image \'' + image_path + '\' does not look like Deep Atropos segmentation (number of spatial dimensions is not 3)')

def get_inputs(): #pylint: disable=unused-variable
  check_deep_atropos_input(path.from_user(app.ARGS.input, False))
  run.command('mrconvert ' + path.from_user(app.ARGS.input) + ' ' + path.to_scratch('input.mif'))

def execute(): #pylint: disable=unused-variable
  # Generate the images related to each tissue
  run.command('mrcalc input.mif 1 -eq CSF.mif')
  run.command('mrcalc input.mif 2 -eq cGM.mif')
  run.command('mrcalc input.mif 3 -eq WM1.mif')
  run.command('mrcalc input.mif 5 -eq WM2.mif')
  run.command('mrcalc input.mif 6 -eq WM3.mif')
  run.command('mrmath WM1.mif WM2.mif WM3.mif sum WM.mif')
  run.command('mrcalc input.mif 4 -eq sGM.mif')

  # Create an empty lesion image
  run.command('mrcalc WM.mif 0 -mul lsn.mif')

  # Convert into the 5tt format
  run.command('mrcat cGM.mif sGM.mif WM.mif CSF.mif lsn.mif 5tt.mif -axis 3')

  if app.ARGS.nocrop:
    run.function(os.rename, '5tt.mif', 'result.mif')
  else:
    run.command('mrmath 5tt.mif sum - -axis 3 | mrthreshold - - -abs 0.5 | mrgrid 5tt.mif crop result.mif -mask -')

  run.command('mrconvert result.mif ' + path.from_user(app.ARGS.output), mrconvert_keyval=path.from_user(app.ARGS.input, False), force=app.FORCE_OVERWRITE)
