def getFSLSuffix():
  import os, sys
  from lib.debugMessage import debugMessage
  from lib.warnMessage  import warnMessage
  fsl_output_type = os.environ.get('FSLOUTPUTTYPE', '')
  if fsl_output_type == 'NIFTI':
    debugMessage('NIFTI -> .nii')
    return '.nii'
  if fsl_output_type == 'NIFTI_GZ':
    debugMessage('NIFTI_GZ -> .nii.gz')
    return '.nii.gz'
  if fsl_output_type == 'NIFTI_PAIR':
    debugMessage('NIFTI_PAIR -> .img')
    return '.img'
  if fsl_output_type == 'NIFTI_PAIR_GZ':
    sys.stderr.write('MRtrix does not support compressed NIFTI pairs; please set FSLOUTPUTTYPE to something else\n')
    sys.stderr.flush()
    exit(1)
  warnMessage('Environment variable FSLOUTPUTTYPE not set; FSL commands may fail\n')
  return '.nii.gz'

