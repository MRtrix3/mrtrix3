_suffix = ''

# Functions that may be useful for scripts that interface with FMRIB FSL tools

# Get the name of the binary file that should be invoked to run eddy;
#   this depends on both whether or not the user has requested that the CUDA
#   version of eddy be used, and the various names that this command could
#   conceivably be installed as.
def eddyBinary(cuda):
  import os
  from mrtrix3 import app
  from distutils.spawn import find_executable
  if cuda:
    if find_executable('eddy_cuda'):
      app.debug('Selecting CUDA version of eddy')
      return 'eddy_cuda'
    else:
      app.warn('CUDA version of eddy not found; running standard version')
  if find_executable('eddy_openmp'):
    path = 'eddy_openmp'
  elif find_executable('eddy'):
    path = 'eddy'
  elif find_executable('fsl5.0-eddy'):
    path = 'fsl5.0-eddy'
  else:
    app.error('Could not find FSL program eddy; please verify FSL install')
  app.debug(path)
  return path



# In some versions of FSL, even though we try to predict the names of image files that
#   FSL commands will generate based on the suffix() function, the FSL binaries themselves
#   ignore the FSLOUTPUTTYPE environment variable. Therefore, the safest approach is:
# Whenever receiving an output image from an FSL command, explicitly search for the path
def findImage(name):
  import os
  from mrtrix3 import app
  basename = name.split('.')[0]
  if os.path.isfile(basename + suffix()):
    app.debug('Image at expected location: \"' + basename + suffix() + '\"')
    return basename + suffix()
  for suf in ['.nii', '.nii.gz', '.img']:
    if os.path.isfile(basename + suf):
      app.debug('Expected image at \"' + basename + suffix() + '\", but found at \"' + basename + suf + '\"')
      return basename + suf
  app.error('Unable to find FSL output file for path \"' + name + '\"')



# For many FSL commands, the format of any output images will depend on the string
#   stored in 'FSLOUTPUTTYPE'. This may even override a filename extension provided
#   to the relevant command. Therefore use this function to 'guess' what the names
#   of images provided by FSL commands will be.
def suffix():
  import os
  from mrtrix3 import app
  global _suffix
  if _suffix:
    return _suffix
  fsl_output_type = os.environ.get('FSLOUTPUTTYPE', '')
  if fsl_output_type == 'NIFTI':
    app.debug('NIFTI -> .nii')
    _suffix = '.nii'
  elif fsl_output_type == 'NIFTI_GZ':
    app.debug('NIFTI_GZ -> .nii.gz')
    _suffix = '.nii.gz'
  elif fsl_output_type == 'NIFTI_PAIR':
    app.debug('NIFTI_PAIR -> .img')
    _suffix = '.img'
  elif fsl_output_type == 'NIFTI_PAIR_GZ':
    app.error('MRtrix3 does not support compressed NIFTI pairs; please change FSLOUTPUTTYPE environment variable')
  elif fsl_output_type:
    app.warn('Unrecognised value for environment variable FSLOUTPUTTYPE (\"' + fsl_output_type + '\"): Expecting compressed NIfTIs, but FSL commands may fail')
    _suffix = '.nii.gz'
  else:
    app.warn('Environment variable FSLOUTPUTTYPE not set; FSL commands may fail, or script may fail to locate FSL command outputs')
    _suffix = '.nii.gz'
  return _suffix

