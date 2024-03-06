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


import os
from mrtrix3 import app
from . import CMDSPLIT

class Entry:
  def __init__(self, input_text, common_prefix, common_suffix):
    self.input_text = input_text
    self.sub_in = input_text
    self.sub_name = os.path.basename(input_text.rstrip('/'))
    self.sub_pre = os.path.splitext(self.sub_name.rstrip('.gz'))[0]
    if common_suffix:
      self.sub_uni = input_text[len(common_prefix):-len(common_suffix)]
    else:
      self.sub_uni = input_text[len(common_prefix):]

    self.substitutions = { 'IN': self.sub_in, 'NAME': self.sub_name, 'PRE': self.sub_pre, 'UNI': self.sub_uni }
    app.debug('Input text: ' + input_text)
    app.debug('Substitutions: ' + str(self.substitutions))

    self.cmd = [ ]
    for entry in CMDSPLIT:
      for (key, value) in self.substitutions.items():
        entry = entry.replace(key, value)
      if ' ' in entry:
        entry = '"' + entry + '"'
      self.cmd.append(entry)
    app.debug('Resulting command: ' + str(self.cmd))

    self.outputtext = None
    self.returncode = None
