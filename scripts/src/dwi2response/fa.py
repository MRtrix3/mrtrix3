def initParser(subparsers, base_parser):
  import argparse  
  parser_fa = subparsers.add_parser('fa', parents=[base_parser], help='Use the old FA-threshold heuristic')
  parser_fa_argument = parser_fa.add_argument_group('Positional argument specific to the \'fa\' algorithm')
  parser_fa_argument.add_argument('output', help='The output response function text file')
  parser_fa_options = parser_fa.add_argument_group('Options specific to the \'fa\' algorithm')
  parser_fa_options.add_argument('-erode', type=int, default=3, help='Number of brain mask erosion steps to apply prior to threshold (not used if mask is provided manually)')
  parser_fa_options.add_argument('-threshold', type=float, default=0.7, help='Threshold to apply to the FA image')
  parser_fa.set_defaults(algorithm='fa')
  parser_fa.set_defaults(single_shell=True)
  
  
  
def checkOutputFiles():
  import lib.app
  lib.app.checkOutputFile(lib.app.args.output)



def getInputFiles():
  pass



def execute():
  import os, shutil
  import lib.app
  from lib.runCommand import runCommand
  lmax_option = ''
  if lib.app.args.lmax:
    lmax_option = ' -lmax ' + lib.app.args.lmax
  if not lib.app.args.mask:
    runCommand('maskfilter mask.mif erode mask_eroded.mif -npass ' + str(lib.app.args.erode))
    mask_path = 'mask_eroded.mif'
  else:
    mask_path = 'mask.mif'
  runCommand('dwi2tensor dwi.mif -mask ' + mask_path + ' tensor.mif')
  runCommand('tensor2metric tensor.mif -fa fa.mif -vector vector.mif -mask ' + mask_path)
  runCommand('mrthreshold fa.mif voxels.mif -abs ' + str(lib.app.args.threshold))
  runCommand('amp2sh dwi.mif dwiSH.mif' + lmax_option)
  runCommand('sh2response dwiSH.mif voxels.mif vector.mif response.txt' + lmax_option)

  shutil.copyfile('response.txt', os.path.join(lib.app.workingDir, lib.app.args.output))

