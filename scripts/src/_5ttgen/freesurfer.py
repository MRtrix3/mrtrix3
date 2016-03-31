def initParser(subparsers, base_parser):
  import argparse  
  parser = subparsers.add_parser('freesurfer', parents=[base_parser], help='Generate the 5TT image based on a FreeSurfer parcellation image (any image containing \'aseg\' in its name)')
  options = parser.add_argument_group('options specific to the \'freesurfer\' algorithm')
  options.add_argument('-lut', help='Manually provide path to file FreeSurferColorLUT.txt')
  parser.set_defaults(algorithm='freesurfer')
  
  
  
def checkOutputFiles():
  pass



def getInputFiles():
  import os, shutil
  import lib.app
  if hasattr(lib.app.args, 'lut') and lib.app.args.lut:
    shutil.copyfile(lib.app.args.lut, os.path.join(lib.app.tempDir, 'LUT.txt'))



def execute():
  import os, sys
  import lib.app
  from lib.errorMessage import errorMessage
  from lib.runCommand import runCommand

  lut_path = 'LUT.txt'
  if not os.path.exists('LUT.txt'):
    freesurfer_home = os.environ.get('FREESURFER_HOME', '')
    if not freesurfer_home:
      errorMessage('Environment variable FREESURFER_HOME is not set; please run appropriate FreeSurfer configuration script, set this variable manually, or provide script with path to file FreeSurferColorLUT.txt using -lut option')
    lut_path = os.path.join(freesurfer_home, 'FreeSurferColorLUT.txt')
    if not os.path.isfile(lut_path):
      errorMessage('Could not find FreeSurfer lookup table file\n(Expected location: ' + freesurfer_lut + ')')

  if lib.app.args.sgm_amyg_hipp:
    config_file_name = 'FreeSurfer2ACT_sgm_amyg_hipp.txt'
  else:
    config_file_name = 'FreeSurfer2ACT.txt'
  config_path = os.path.join(os.path.dirname(os.path.abspath(sys.argv[0])), 'data', config_file_name);
  if not os.path.isfile(config_path):
    errorMessage('Could not find config file for converting FreeSurfer parcellation output to tissues\n(Expected location: ' + config_path + ')')

  # Initial conversion from FreeSurfer parcellation to five principal tissue types
  runCommand('labelconfig input.mif ' + config_path + ' indices.mif -lut_freesurfer ' + lut_path)

  # Use mrcrop to reduce file size
  if lib.app.args.nocrop:
    image = 'indices.mif'
  else:
    image = 'indices_cropped.mif'
    runCommand('mrthreshold indices.mif - -abs 0.5 | mrcrop indices.mif ' + image + ' -mask -')

  # Convert into the 5TT format for ACT
  runCommand('mrcalc ' + image + ' 1 -eq cgm.mif')
  runCommand('mrcalc ' + image + ' 2 -eq sgm.mif')
  runCommand('mrcalc ' + image + ' 3 -eq  wm.mif')
  runCommand('mrcalc ' + image + ' 4 -eq csf.mif')
  runCommand('mrcalc ' + image + ' 5 -eq path.mif')

  runCommand('mrcat cgm.mif sgm.mif wm.mif csf.mif path.mif - -axis 3 | mrconvert - result.mif -datatype float32')

