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

import os, re, sys, threading
from mrtrix3 import ANSI, MRtrixError
from mrtrix3 import app, run
from . import CMDSPLIT, KEYLIST
from .entry import Entry
from .shared import Shared

def execute(): #pylint: disable=unused-variable

  inputs = app.ARGS.inputs
  app.debug('All inputs: ' + str(inputs))
  app.debug('Command: ' + str(app.ARGS.command))
  app.debug('CMDSPLIT: ' + str(CMDSPLIT))

  if app.ARGS.exclude:
    app.ARGS.exclude = [ exclude[0] for exclude in app.ARGS.exclude ] # To deal with argparse's action=append. Always guaranteed to be only one argument since nargs=1
    app.debug('To exclude: ' + str(app.ARGS.exclude))
    exclude_unmatched = [ ]
    to_exclude = [ ]
    for exclude in app.ARGS.exclude:
      if exclude in inputs:
        to_exclude.append(exclude)
      else:
        try:
          re_object = re.compile(exclude)
          regex_hits = [ ]
          for arg in inputs:
            search_result = re_object.search(arg)
            if search_result and search_result.group():
              regex_hits.append(arg)
          if regex_hits:
            app.debug('Inputs excluded via regex "' + exclude + '": ' + str(regex_hits))
            to_exclude.extend(regex_hits)
          else:
            app.debug('Compiled exclude regex "' + exclude + '" had no hits')
            exclude_unmatched.append(exclude)
        except re.error:
          app.debug('Exclude string "' + exclude + '" did not compile as regex')
          exclude_unmatched.append(exclude)
    if exclude_unmatched:
      app.warn('Item' + ('s' if len(exclude_unmatched) > 1 else '') + ' specified via -exclude did not result in item exclusion, whether by direct match or compilation as regex: ' + str('\'' + exclude_unmatched[0] + '\'' if len(exclude_unmatched) == 1 else exclude_unmatched))
    inputs = [ arg for arg in inputs if arg not in to_exclude ]
    if not inputs:
      raise MRtrixError('No inputs remaining after application of exclusion criteri' + ('on' if len(app.ARGS.exclude) == 1 else 'a'))
    app.debug('Inputs after exclusion: ' + str(inputs))

  common_prefix = os.path.commonprefix(inputs)
  common_suffix = os.path.commonprefix([i[::-1] for i in inputs])[::-1]
  app.debug('Common prefix: ' + common_prefix if common_prefix else 'No common prefix')
  app.debug('Common suffix: ' + common_suffix if common_suffix else 'No common suffix')

  for entry in CMDSPLIT:
    if os.path.exists(entry):
      keys_present = [ key for key in KEYLIST if key in entry ]
      if keys_present:
        app.warn('Performing text substitution of ' + str(keys_present) + ' within command: "' + entry + '"; but the original text exists as a path on the file system... is this a problematic filesystem path?')

  try:
    next(entry for entry in CMDSPLIT if any(key for key in KEYLIST if key in entry))
  except StopIteration as exception:
    raise MRtrixError('None of the unique for_each keys ' + str(KEYLIST) + ' appear in command string "' + app.ARGS.command + '"; no substitution can occur') from exception

  jobs = [ ]
  for i in inputs:
    jobs.append(Entry(i, common_prefix, common_suffix))

  if app.ARGS.test:
    app.console('Command strings for ' + str(len(jobs)) + ' jobs:')
    for job in jobs:
      sys.stderr.write(ANSI.execute + 'Input:' + ANSI.clear + '   "' + job.input_text + '"\n')
      sys.stderr.write(ANSI.execute + 'Command:' + ANSI.clear + ' ' + ' '.join(job.cmd) + '\n')
    return

  parallel = app.NUM_THREADS is not None and app.NUM_THREADS > 1

  def progress_string():
    text = str(sum(1 if job.returncode is not None else 0 for job in jobs)) + \
        '/' + \
        str(len(jobs)) + \
        ' jobs completed ' + \
        ('across ' + str(app.NUM_THREADS) + ' threads' if parallel else 'sequentially')
    fail_count = sum(1 if job.returncode else 0 for job in jobs)
    if fail_count:
      text += ' (' + str(fail_count) + ' errors)'
    return text

  progress = app.ProgressBar(progress_string(), len(jobs))
  shared = Shared()

  def execute_parallel():
    while not shared.stop:
      my_job = shared.next(jobs)
      if not my_job:
        return
      try:
        result = run.command(' '.join(my_job.cmd), shell=True)
        my_job.outputtext = result.stdout + result.stderr
        my_job.returncode = 0
      except run.MRtrixCmdError as exception:
        my_job.outputtext = str(exception)
        my_job.returncode = exception.returncode
      except Exception as exception: # pylint: disable=broad-except
        my_job.outputtext = str(exception)
        my_job.returncode = 1
      with shared.lock:
        progress.increment(progress_string())

  if parallel:
    threads = [ ]
    for i in range (1, app.NUM_THREADS):
      thread = threading.Thread(target=execute_parallel)
      thread.start()
      threads.append(thread)
    execute_parallel()
    for thread in threads:
      thread.join()
  else:
    for job in jobs:
      try:
        result = run.command(' '.join(job.cmd), shell=True)
        job.outputtext = result.stdout + result.stderr
        job.returncode = 0
      except run.MRtrixCmdError as exception:
        job.outputtext = str(exception)
        job.returncode = exception.returncode
      except Exception as exception: # pylint: disable=broad-except
        job.outputtext = str(exception)
        job.returncode = 1
      progress.increment(progress_string())

  progress.done()

  assert all(job.returncode is not None for job in jobs)
  fail_count = sum(1 if job.returncode else 0 for job in jobs)
  if fail_count:
    app.warn(str(fail_count) + ' of ' + str(len(jobs)) + ' jobs did not complete successfully')
    if fail_count > 1:
      app.warn('Outputs from failed commands:')
      sys.stderr.write(app.EXEC_NAME + ':\n')
    else:
      app.warn('Output from failed command:')
    for job in jobs:
      if job.returncode:
        if job.outputtext:
          app.warn('For input "' + job.sub_in + '" (returncode = ' + str(job.returncode) + '):')
          for line in job.outputtext.splitlines():
            sys.stderr.write(' ' * (len(app.EXEC_NAME)+2) + line + '\n')
        else:
          app.warn('No output from command for input "' + job.sub_in + '" (return code = ' + str(job.returncode) + ')')
        if fail_count > 1:
          sys.stderr.write(app.EXEC_NAME + ':\n')
    raise MRtrixError(str(fail_count) + ' of ' + str(len(jobs)) + ' jobs did not complete successfully: ' + str([job.input_text for job in jobs if job.returncode]))

  if app.VERBOSITY > 1:
    if any(job.outputtext for job in jobs):
      sys.stderr.write(app.EXEC_NAME + ':\n')
      for job in jobs:
        if job.outputtext:
          app.console('Output of command for input "' + job.sub_in + '":')
          for line in job.outputtext.splitlines():
            sys.stderr.write(' ' * (len(app.EXEC_NAME)+2) + line + '\n')
        else:
          app.console('No output from command for input "' + job.sub_in + '"')
        sys.stderr.write(app.EXEC_NAME + ':\n')
    else:
      app.console('No output from command for any inputs')

  app.console('Script reported successful completion for all inputs')
