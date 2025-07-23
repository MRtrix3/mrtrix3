# Copyright (c) 2008-2025 the MRtrix3 contributors.
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

import os
import subprocess
try:
  from shutil import which as find_executable
except ImportError:
  from distutils.spawn import find_executable # pylint: disable=deprecated-module
from mrtrix3 import MRtrixError




_SUFFIX = ''



# Functions that may be useful for scripts that interface with FMRIB FSL tools

# FSL's run_first_all script can be difficult to wrap, since:
#   - It may or may not run via SGE or SLURM, and therefore execution control will
#     return to Python even though those jobs have not yet executed / completed
#   - The return code of run_first_all may be a job ID that can possibly be used
#     to query whether or not jobs have completed
#   - Sometimes a subset of the segmentation jobs may fail; when this happens,
#     ideally the script should report an outright failure and raise an MRtrixError;
#     but in some circumstances, it's possible that the requisite files may appear
#     later because eg. they are being executed via SGE
# This function attempts to provide a unified interface for querying whether or not
#   FIRST was successful, taking all of these into account
def check_first(prefix, structures=None, first_stdout=None): #pylint: disable=unused-variable
  from mrtrix3 import app, path #pylint: disable=import-outside-toplevel
  job_id = None
  if first_stdout:
    try:
      job_id = int(first_stdout.rstrip().splitlines()[-1])
    except ValueError:
      app.debug('Unable to convert FIRST stdout contents to integer job ID')
  execution_verified = False
  if job_id:
    # Eventually modify on dev to reflect Python3 prerequisite
    # Create dummy fsl_sub job, use to monitor for completion
    flag_file = path.name_temporary('txt')
    try:
      with subprocess.Popen(['fsl_sub',
                             '-j', str(job_id),
                             '-T', '1',
                             'touch', flag_file],
                            stdout=subprocess.PIPE) as proc:
        (fslsub_stdout, _) = proc.communicate()
        returncode = proc.returncode
      if returncode:
        app.debug('fsl_sub executed successfully, but returned error code ' + str(returncode))
      else:
        app.debug('fsl_sub successfully executed; awaiting completion flag')
        path.wait_for(flag_file)
        execution_verified = True
        app.debug('Flag file identified indicating completion of all run_first_all tasks')
      try:
        flag_jobid = int(fslsub_stdout.rstrip().splitlines()[-1])
        app.cleanup(['touch.' + stream + str(flag_jobid) for stream in ['o', 'e']])
      except ValueError:
        app.debug('Unable to parse Job ID for fsl_sub "touch" job; could not erase stream files')
    except OSError:
      app.debug('Unable to execute fsl_sub to check status of FIRST execution')
    finally:
      app.cleanup(flag_file)
  if not structures:
    app.warn('No way to verify up-front whether FSL FIRST was successful due to no knowledge of set of structures to be segmented')
    app.warn('Execution will continue, but script may subsequently fail if an expected structure was not segmented successfully')
    return
  vtk_files = [ prefix + '-' + struct + '_first.vtk' for struct in structures ]
  existing_file_count = sum(os.path.exists(filename) for filename in vtk_files)
  if existing_file_count == len(vtk_files):
    app.debug('All ' + str(existing_file_count) + ' expected FIRST .vtk files found')
    return
  if not execution_verified and 'SGE_ROOT' in os.environ and os.environ['SGE_ROOT']:
    app.console('FSL FIRST job PID not found, but job may nevertheless complete later via SGE')
    app.console('Script will wait to see if the expected .vtk files are generated later')
    app.console('(note however that FIRST may fail silently, and hence this script may hang indefinitely)')
    path.wait_for(vtk_files)
    return
  app.DO_CLEANUP = False
  raise MRtrixError('FSL FIRST has failed; '
                    + ('only ' if existing_file_count else '') + str(existing_file_count) + ' of ' + str(len(vtk_files)) + ' structures were segmented successfully '
                    + '(check ' + path.to_scratch('first.logs', False) + ')')



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
