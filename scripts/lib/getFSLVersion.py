def getFSLVersion():
  import os
  from lib.errorMessage import errorMessage
  fsl_path = os.environ.get('FSLDIR', '')
  if not fsl_path:
    errorMessage('Environment variable FSLDIR is not set; please run appropriate FSL configuration script')
  path = os.path.join(fsl_path, 'etc', 'fslversion')
  if not os.path.isfile(path):
    errorMessage('Could not query FSL version; please check FSL install')
  with open(path, 'r') as f:
    fsl_version = f.readline().strip()
  return fsl_version

