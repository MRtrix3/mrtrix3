def getFSLEddyPath(cuda):
  import os
  from lib.binaryInPath import binaryInPath
  from lib.debugMessage import debugMessage
  from lib.errorMessage import errorMessage
  from lib.warnMessage  import warnMessage
  if cuda:
    if binaryInPath('eddy_cuda'):
      debugMessage('Selecting CUDA version of eddy')
      return 'eddy_cuda'
    else:
      warnMessage('CUDA version of eddy not found; running standard version')
  debugMessage('Selecting non-CUDA version of eddy')
  if binaryInPath('eddy_openmp'):
    return 'eddy_openmp'
  if binaryInPath('eddy'):
    return 'eddy'
  if binaryInPath('fsl5.0-eddy'):
    return 'fsl5.0-eddy'
  errorMessage('Could not find FSL program eddy; please verify FSL install')

