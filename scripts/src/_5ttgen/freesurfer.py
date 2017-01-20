def initParser(subparsers, base_parser):
  import argparse  
  parser = subparsers.add_parser('freesurfer', parents=[base_parser], add_help=False, description='Generate the 5TT image based on a FreeSurfer parcellation image')
  parser.add_argument('input',  help='The input FreeSurfer parcellation image (any image containing \'aseg\' in its name)')
  parser.add_argument('output', help='The output 5TT image')
  options = parser.add_argument_group('Options specific to the \'freesurfer\' algorithm')
  options.add_argument('-lut', help='Manually provide path to the lookup table on which the input parcellation image is based (e.g. FreeSurferColorLUT.txt)')
  parser.set_defaults(algorithm='freesurfer')



def checkOutputFiles():
  pass



def getInputFiles():
  import os, shutil
  import lib.app
  from lib.getUserPath import getUserPath
  from lib.runFunction import runFunction
  if hasattr(lib.app.args, 'lut') and lib.app.args.lut:
    runFunction(shutil.copyfile, getUserPath(lib.app.args.lut, False), os.path.join(lib.app.tempDir, 'LUT.txt'))



def execute():
  import os, sys
  import lib.app
  from lib.errorMessage import errorMessage
  from lib.getUserPath  import getUserPath
  from lib.runCommand   import runCommand

  lut_input_path = 'LUT.txt'
  if not os.path.exists('LUT.txt'):
    freesurfer_home = os.environ.get('FREESURFER_HOME', '')
    if not freesurfer_home:
      errorMessage('Environment variable FREESURFER_HOME is not set; please run appropriate FreeSurfer configuration script, set this variable manually, or provide script with path to file FreeSurferColorLUT.txt using -lut option')
    lut_input_path = os.path.join(freesurfer_home, 'FreeSurferColorLUT.txt')
    if not os.path.isfile(lut_input_path):
      errorMessage('Could not find FreeSurfer lookup table file (expected location: ' + freesurfer_lut + '), and none provided using -lut')

  if lib.app.args.sgm_amyg_hipp:
    lut_output_file_name = 'FreeSurfer2ACT_sgm_amyg_hipp.txt'
  else:
    lut_output_file_name = 'FreeSurfer2ACT.txt'
  lut_output_path = os.path.join(os.path.dirname(os.path.abspath(sys.argv[0])), 'data', lut_output_file_name);
  if not os.path.isfile(lut_output_path):
    errorMessage('Could not find lookup table file for converting FreeSurfer parcellation output to tissues (expected location: ' + lut_output_path + ')')

  # Initial conversion from FreeSurfer parcellation to five principal tissue types
  runCommand('labelconvert input.mif ' + lut_input_path + ' ' + lut_output_path + ' indices.mif')

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

