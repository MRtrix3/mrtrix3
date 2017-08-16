_suffix = ''

# Functions that may be useful for scripts that interface with FMRIB FSL tools

# Get the name of the binary file that should be invoked to run eddy;
#   this depends on both whether or not the user has requested that the CUDA
#   version of eddy be used, and the various names that this command could
#   conceivably be installed as.
def eddyBinary(cuda): #pylint: disable=unused-variable
  import os
  from mrtrix3 import app
  from distutils.spawn import find_executable
  if cuda:
    if find_executable('eddy_cuda'):
      app.debug('Selected CUDA version (eddy_cuda)')
      return 'eddy_cuda'
    else:
      # Cuda versions are now provided with a CUDA trailing version number
      # Users may not necessarily create a softlink to one of these and
      #   call it "eddy_cuda"
      # Therefore, hunt through PATH looking for them; if more than one,
      #   select the one with the highest version number
      binaries = [ ]
      for directory in os.environ['PATH'].split(os.pathsep):
        if os.path.isdir(directory):
          for file in os.listdir(directory):
            if file.startswith('eddy_cuda'):
              binaries.append(file)
      max_version = 0.0
      path = ''
      for file in binaries:
        try:
          version = float(file.lstrip('eddy_cuda'))
          if version > max_version:
            max_version = version
            path = file
        except:
          pass
      if path:
        app.debug('CUDA version ' + str(max_version) + ': ' + path)
        return path
      app.warn('CUDA version of eddy not found; running standard version')
  if find_executable('eddy_openmp'):
    path = 'eddy_openmp'
  else:
    path = exeName('eddy')
  app.debug(path)
  return path



# In some FSL installations, all binaries get prepended with "fsl5.0-". This function
#   makes it more convenient to locate these commands.
# Note that if FSL 4 and 5 are installed side-by-side, the approach taken in this
#   function will select the version 5 executable.
def exeName(name): #pylint: disable=unused-variable
  from mrtrix3 import app
  from distutils.spawn import find_executable
  if find_executable('fsl5.0-' + name):
    output = 'fsl5.0-' + name
  elif find_executable(name):
    output = name
  else:
    app.error('Could not find FSL program \"' + name + '\"; please verify FSL install')
  app.debug(output)
  return output



# In some versions of FSL, even though we try to predict the names of image files that
#   FSL commands will generate based on the suffix() function, the FSL binaries themselves
#   ignore the FSLOUTPUTTYPE environment variable. Therefore, the safest approach is:
# Whenever receiving an output image from an FSL command, explicitly search for the path
def findImage(name): #pylint: disable=unused-variable
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
def suffix(): #pylint: disable=unused-variable
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
