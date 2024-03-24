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

import threading

class Shared: # pylint: disable=unused-variable
  def __init__(self):
    self._job_index = 0
    self.lock = threading.Lock()
    self.stop = False
  def next(self, jobs):
    job = None
    with self.lock:
      if self._job_index < len(jobs):
        job = jobs[self._job_index]
        self._job_index += 1
        self.stop = self._job_index == len(jobs)
    return job
