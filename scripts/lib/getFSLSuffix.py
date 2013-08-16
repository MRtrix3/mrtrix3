def getFSLSuffix():
  import os, sys
  fsl_output_type = os.environ.get('FSLOUTPUTTYPE', '')
  if not fsl_output_type:
	  return ''
  if fsl_output_type == 'NIFTI':
    return 'nii'
  if fsl_output_type == 'NIFTI_GZ':
    return 'nii.gz'
  if fsl_output_type == 'NIFTI_PAIR':
    return 'img'
  if fsl_output_type == 'NIFTI_PAIR_GZ':
    sys.stderr.write('MRtrix does not support compressed NIFTI pairs; please set FSLOUTPUTTYPE to something else\n')
    exit(1)
  sys.stderr.write('Environment variable FSLOUTPUTTYPE not set; FSL commands may fail\n')
  return 'nii.gz'

