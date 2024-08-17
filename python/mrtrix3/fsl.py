# Copyright (c) 2008-2024 the MRtrix3 contributors.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Covered Software is provided under this License on an "as is"
# basis, without warranty of any kind, either expressed, implied, or
# statutory, including, without limitation, warranties that the
# Covered Software is free of defects, merchantable, fit for a
# particular purpose or non-infringing.
# See the Mozilla Public License v. 2.0 for more details.
#
# For more details, see http://www.mrtrix.org/.

import os, pathlib, shutil
from mrtrix3 import MRtrixError




_SUFFIX = ''

IMAGETYPE2SUFFIX = {'NIFTI': '.nii',
                    'NIFTI_GZ': '.nii.gz',
                    'NIFTI_PAIR': '.img',
                    'NIFTI_PAIR_GZ': None}



# Functions that may be useful for scripts that interface with FMRIB FSL tools

# FSL's run_first_all script can be difficult to wrap, since it does not provide
#   a meaningful return code, and may run via SGE, which then requires waiting for
#   the output files to appear.
def check_first(prefix, structures): #pylint: disable=unused-variable
  from mrtrix3 import app, path #pylint: disable=import-outside-toplevel
  vtk_files = [ pathlib.Path(f'{prefix}-{struct}_first.vtk') for struct in structures ]
  existing_file_count = sum(vtk_file.exists() for vtk_file in vtk_files)
  if existing_file_count != len(vtk_files):
    if 'SGE_ROOT' in os.environ and os.environ['SGE_ROOT']:
      app.console('FSL FIRST job may have been run via SGE; '
                  'awaiting completion')
      app.console('(note however that FIRST may fail silently, '
                  'and hence this script may hang indefinitely)')
      path.wait_for(vtk_files)
    else:
      app.DO_CLEANUP = False
      raise MRtrixError('FSL FIRST has failed; '
                        f'{"only " if existing_file_count else ""}{existing_file_count} of {len(vtk_files)} structures were segmented successfully '
                        f'(check {app.SCRATCH_DIR / "first.logs"})')



# Get the name of the binary file that should be invoked to run eddy;
#   this depends on both whether or not the user has requested that the CUDA
#   version of eddy be used, and the various names that this command could
#   conceivably be installed as.
def eddy_binary(cuda): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  if cuda:
    if shutil.which('eddy_cuda'):
      app.debug('Selected soft-linked CUDA version ("eddy_cuda")')
      return 'eddy_cuda'
    # Cuda versions are now provided with a CUDA trailing version number
    # Users may not necessarily create a softlink to one of these and
    #   call it "eddy_cuda"
    # Therefore, hunt through PATH looking for them; if more than one,
    #   select the one with the highest version number
    binaries = [ ]
    for directory in os.environ['PATH'].split(os.pathsep):
      directory = pathlib.Path(directory)
      try:
        for entry in directory.glob('eddy_cuda*'):
          binaries.append(entry)
      except OSError:
        pass
    max_version = 0.0
    exe_path = None
    for entry in binaries:
      try:
        version = float(entry.stem.lstrip('eddy_cuda'))
        if version > max_version:
          max_version = version
          exe_path = entry
      except ValueError:
        pass
    if exe_path:
      app.debug(f'CUDA version {max_version}: {exe_path}')
      return exe_path
    app.debug('No CUDA version of eddy found')
    return ''
  for candidate in [ 'eddy_openmp', 'eddy_cpu', 'eddy', 'fsl5.0-eddy' ]:
    if shutil.which(candidate):
      app.debug(candidate)
      return candidate
  app.debug('No CPU version of eddy found')
  return ''



# In some FSL installations, all binaries get prepended with "fsl5.0-". This function
#   makes it more convenient to locate these commands.
# Note that if FSL 4 and 5 are installed side-by-side, the approach taken in this
#   function will select the version 5 executable.
def exe_name(name): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  if shutil.which(name):
    output = name
  elif shutil.which(f'fsl5.0-{name}'):
    output = f'fsl5.0-{name}'
    app.warn(f'Using FSL binary "{output}" rather than "{name}"; '
             'suggest checking FSL installation')
  else:
    raise MRtrixError(f'Could not find FSL program "{name}"; '
                      'please verify FSL install')
  app.debug(output)
  return output



# In some versions of FSL, even though we try to predict the names of image files that
#   FSL commands will generate based on the suffix() function,
#   the FSL binaries themselves ignore the FSLOUTPUTTYPE environment variable.
# Therefore, the safest approach is:
#   Whenever receiving an output image from an FSL command,
#   explicitly search for the path
def find_image(name): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  prefix = pathlib.PurePath(name)
  prefix = prefix.parent / prefix.name.split('.')[0]
  expected = prefix.with_suffix(suffix())
  if expected.is_file():
    app.debug(f'Image at expected location: {expected}')
    return expected
  for suf in ['.nii', '.nii.gz', '.img']:
    candidate = prefix.with_suffix(suf)
    if candidate.is_file():
      app.debug(f'Expected image at {expected}, '
                f'but found at {candidate}')
      return candidate
  raise MRtrixError(f'Unable to find FSL output image for path {name}')



# For many FSL commands, the format of any output images will depend on the string
#   stored in 'FSLOUTPUTTYPE'. This may even override a filename extension provided
#   to the relevant command. Therefore use this function to 'guess' what the names
#   of images provided by FSL commands will be.
def suffix(): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  global _SUFFIX
  if _SUFFIX:
    return _SUFFIX
  fsl_output_type = os.environ.get('FSLOUTPUTTYPE', '')
  if fsl_output_type in IMAGETYPE2SUFFIX:
    _SUFFIX = IMAGETYPE2SUFFIX[fsl_output_type]
    if _SUFFIX is None:
      raise MRtrixError(f'MRtrix3 does not support FSL output image type "{fsl_output_type}; '
                        'please change FSLOUTPUTTYPE environment variable')
    app.debug(f'{fsl_output_type} -> {_SUFFIX}')
  else:
    _SUFFIX = '.nii.gz'
    if fsl_output_type:
      app.warn('Unrecognised value for environment variable FSLOUTPUTTYPE '
              f'("{fsl_output_type}"): '
              'executed FSL commands may fail')
    else:
      app.warn('Environment variable FSLOUTPUTTYPE not set; '
               'FSL commands may fail, '
               'or this script may fail to locate FSL command outputs')
  return _SUFFIX
