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



# For many FSL commands, the format of any output images will depend on the string
#   stored in 'FSLOUTPUTTYPE'. This may even override a filename extension provided
#   to the relevant command. Therefore use this function to 'guess' what the names
#   of images provided by FSL commands will be.
def suffix():
  import os
  from mrtrix3 import app
  fsl_output_type = os.environ.get('FSLOUTPUTTYPE', '')
  if fsl_output_type == 'NIFTI':
    app.debug('NIFTI -> .nii')
    return '.nii'
  if fsl_output_type == 'NIFTI_GZ':
    app.debug('NIFTI_GZ -> .nii.gz')
    return '.nii.gz'
  if fsl_output_type == 'NIFTI_PAIR':
    app.debug('NIFTI_PAIR -> .img')
    return '.img'
  if fsl_output_type == 'NIFTI_PAIR_GZ':
    app.error('MRtrix3 does not support compressed NIFTI pairs; please change FSLOUTPUTTYPE environment variable')
  app.warn('Environment variable FSLOUTPUTTYPE not set; FSL commands may fail, or script may fail to locate FSL command outputs')
  return '.nii.gz'

