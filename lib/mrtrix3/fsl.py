# Copyright (c) 2008-2023 the MRtrix3 contributors.
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

import os, psutil
from distutils.spawn import find_executable
from mrtrix3 import MRtrixError




_SUFFIX = ''



# Functions that may be useful for scripts that interface with FMRIB FSL tools

# Check the results of having executed "run_first_all"
# - Script may return zero even though the processing jobs have not yet completed,
#   as execution may have been deferred to SGE
# - A subset of the structures to be segmented may have failed, resulting in missing .vtk files
# - run_first_all may quote the final PID on stdout, allowing us to wait for completion of that process
def check_first(prefix, structures=[], first_stdout=None): #pylint: disable=unused-variable
  from mrtrix3 import app, path #pylint: disable=import-outside-toplevel
  pid = None
  if first_stdout:
    try:
      pid = int(first_stdout.rstrip().splitlines()[-1])
    except ValueError:
      pass
  if pid is not None:
    app.debug('FIRST PID reported as ' + str(pid))
    try:
      process = psutil.Process(pid)
      processes = [process.children(recursive=True), process]
      app.debug('Awaiting completion of ' + str(len(processes)) + ' processes')
      psutil.wait_procs(processes)
    except psutil.NoSuchProcess:
      app.debug('FIRST PID no longer exists')
    except psutil.AccessDenied:
      app.debug('Inadequate credentials to query status of FIRST PID')
      pid = None
  else:
    app.debug('No FIRST PID found')
  if not structures:
    app.warn('No way to verify up-front whether FSL FIRST was successful')
    app.warn('Execution will continue, but script may subsequently fail if an expected structure was not segmented')
    return
  vtk_files = [ prefix + '-' + struct + '_first.vtk' for struct in structures ]
  existing_file_count = sum([ os.path.exists(filename) for filename in vtk_files ])
  if existing_file_count == len(vtk_files):
    app.debug('All ' + str(existing_file_count) + ' expected FIRST .vtk files found')
    return
  if pid is None and 'SGE_ROOT' in os.environ and os.environ['SGE_ROOT']:
    app.console('FSL FIRST job PID not found, but job may nevertheless complete later via SGE')
    app.console('Script will wait to see if the expected .vtk files are generated later')
    app.console('(note however that FIRST may fail silently, and hence this script may hang indefinitely)')
    path.wait_for(vtk_files)
  else:
    app.DO_CLEANUP = False
    raise MRtrixError('FSL FIRST has failed; ' + ('only ' if existing_file_count else '') + str(existing_file_count) + ' of ' + str(len(vtk_files)) + ' structures were segmented successfully (check ' + path.to_scratch('first.logs', False) + ')')



# Get the name of the binary file that should be invoked to run eddy;
#   this depends on both whether or not the user has requested that the CUDA
#   version of eddy be used, and the various names that this command could
#   conceivably be installed as.
def eddy_binary(cuda): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  if cuda:
    if find_executable('eddy_cuda'):
      app.debug('Selected soft-linked CUDA version (\'eddy_cuda\')')
      return 'eddy_cuda'
    # Cuda versions are now provided with a CUDA trailing version number
    # Users may not necessarily create a softlink to one of these and
    #   call it "eddy_cuda"
    # Therefore, hunt through PATH looking for them; if more than one,
    #   select the one with the highest version number
    binaries = [ ]
    for directory in os.environ['PATH'].split(os.pathsep):
      if os.path.isdir(directory):
        for entry in os.listdir(directory):
          if entry.startswith('eddy_cuda'):
            binaries.append(entry)
    max_version = 0.0
    exe_path = ''
    for entry in binaries:
      try:
        version = float(entry.lstrip('eddy_cuda'))
        if version > max_version:
          max_version = version
          exe_path = entry
      except:
        pass
    if exe_path:
      app.debug('CUDA version ' + str(max_version) + ': ' + exe_path)
      return exe_path
    app.debug('No CUDA version of eddy found')
    return ''
  for candidate in [ 'eddy_openmp', 'eddy_cpu', 'eddy', 'fsl5.0-eddy' ]:
    if find_executable(candidate):
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
  if find_executable(name):
    output = name
  elif find_executable('fsl5.0-' + name):
    output = 'fsl5.0-' + name
    app.warn('Using FSL binary \"' + output + '\" rather than \"' + name + '\"; suggest checking FSL installation')
  else:
    raise MRtrixError('Could not find FSL program \"' + name + '\"; please verify FSL install')
  app.debug(output)
  return output



# In some versions of FSL, even though we try to predict the names of image files that
#   FSL commands will generate based on the suffix() function, the FSL binaries themselves
#   ignore the FSLOUTPUTTYPE environment variable. Therefore, the safest approach is:
# Whenever receiving an output image from an FSL command, explicitly search for the path
def find_image(name): #pylint: disable=unused-variable
  from mrtrix3 import app #pylint: disable=import-outside-toplevel
  prefix = os.path.join(os.path.dirname(name), os.path.basename(name).split('.')[0])
  if os.path.isfile(prefix + suffix()):
    app.debug('Image at expected location: \"' + prefix + suffix() + '\"')
    return prefix + suffix()
  for suf in ['.nii', '.nii.gz', '.img']:
    if os.path.isfile(prefix + suf):
      app.debug('Expected image at \"' + prefix + suffix() + '\", but found at \"' + prefix + suf + '\"')
      return prefix + suf
  raise MRtrixError('Unable to find FSL output file for path \"' + name + '\"')



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
  if fsl_output_type == 'NIFTI':
    app.debug('NIFTI -> .nii')
    _SUFFIX = '.nii'
  elif fsl_output_type == 'NIFTI_GZ':
    app.debug('NIFTI_GZ -> .nii.gz')
    _SUFFIX = '.nii.gz'
  elif fsl_output_type == 'NIFTI_PAIR':
    app.debug('NIFTI_PAIR -> .img')
    _SUFFIX = '.img'
  elif fsl_output_type == 'NIFTI_PAIR_GZ':
    raise MRtrixError('MRtrix3 does not support compressed NIFTI pairs; please change FSLOUTPUTTYPE environment variable')
  elif fsl_output_type:
    app.warn('Unrecognised value for environment variable FSLOUTPUTTYPE (\"' + fsl_output_type + '\"): Expecting compressed NIfTIs, but FSL commands may fail')
    _SUFFIX = '.nii.gz'
  else:
    app.warn('Environment variable FSLOUTPUTTYPE not set; FSL commands may fail, or script may fail to locate FSL command outputs')
    _SUFFIX = '.nii.gz'
  return _SUFFIX
