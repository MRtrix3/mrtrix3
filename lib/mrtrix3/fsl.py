# Functions that may be useful for scripts that interface with FMRIB FSL tools

# Get the name of the binary file that should be invoked to run eddy;
#   this depends on both whether or not the user has requested that the CUDA
#   version of eddy be used, and the various names that this command could
#   conceivably be installed as.
def eddyBinary(cuda):
  import os
  from mrtrix3 import message, misc
  if cuda:
    if misc.haveBinary('eddy_cuda'):
      message.debug('Selecting CUDA version of eddy')
      return 'eddy_cuda'
    else:
      message.warn('CUDA version of eddy not found; running standard version')
  if misc.haveBinary('eddy_openmp'):
    path = 'eddy_openmp'
  elif misc.haveBinary('eddy'):
    path = 'eddy'
  elif misc.haveBinary('fsl5.0-eddy'):
    path = 'fsl5.0-eddy'
  else:
    message.error('Could not find FSL program eddy; please verify FSL install')
  message.debug(path)
  return path



# For many FSL commands, the format of any output images will depend on the string
#   stored in 'FSLOUTPUTTYPE'. This may even override a filename extension provided
#   to the relevant command. Therefore use this function to 'guess' what the names
#   of images provided by FSL commands will be.
def suffix():
  import os, sys
  from mrtrix3 import message
  fsl_output_type = os.environ.get('FSLOUTPUTTYPE', '')
  if fsl_output_type == 'NIFTI':
    message.debug('NIFTI -> .nii')
    return '.nii'
  if fsl_output_type == 'NIFTI_GZ':
    message.debug('NIFTI_GZ -> .nii.gz')
    return '.nii.gz'
  if fsl_output_type == 'NIFTI_PAIR':
    message.debug('NIFTI_PAIR -> .img')
    return '.img'
  if fsl_output_type == 'NIFTI_PAIR_GZ':
    message.error('MRtrix3 does not support compressed NIFTI pairs; please change FSLOUTPUTTYPE environment variable')
  message.warn('Environment variable FSLOUTPUTTYPE not set; FSL commands may fail, or script may fail to locate FSL command outputs')
  return '.nii.gz'

